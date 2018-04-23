//-----------------------------------------------------------------------------
// Copyright (c) 2017-2018 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <disposer/component.hpp>
#include <disposer/module.hpp>

#include <io_tools/range_to_string.hpp>

#include <webservice/server.hpp>
#include <webservice/json_ws_service.hpp>
#include <webservice/ws_service_handler.hpp>
#include <webservice/file_request_handler.hpp>

#include <boost/dll.hpp>

#include <shared_mutex>
#include <thread>
#include <chrono>


namespace disposer_module::http_server_component{


	using namespace disposer::literals;
	namespace hana = boost::hana;


	template < typename T, typename KeyT >
	std::optional< T > get_optional(nlohmann::json const& json, KeyT&& key){
		auto iter = json.find(key);
		if(iter != json.end()){
			return iter->template get< T >();
		}else{
			return {};
		}
	}


	template < typename Module >
	class live_service
		: public webservice::basic_ws_service< bool, std::string >{
	public:
		live_service(Module module)
			: name(module("service_name"_param))
			, module_(module) {}

		void on_server_connect(
			boost::asio::ip::tcp::socket&& socket,
			webservice::http_request&& req
		){
			async_server_connect(std::move(socket), std::move(req), false);
		}

		void on_open(webservice::ws_identifier identifier)override{
			module_.log(
				[this, identifier](logsys::stdlogb& os){
					os << "live service(" << name << ") on_open identifier("
						<< identifier << ")";
				});
		}

		void on_close(webservice::ws_identifier identifier)override{
			module_.log(
				[this, identifier](logsys::stdlogb& os){
					os << "live service(" << name << ") on_close identifier("
						<< identifier << ")";
				});
		}

		void on_text(
			webservice::ws_identifier identifier,
			std::string&& text
		)override{
			module_.log(
				[this, identifier, &text](logsys::stdlogb& os){
					os << "live service(" << name << ") on_text identifier("
						<< identifier << "); message(" << text << ")";
				}, [this, identifier, &text]{
					if(text != "ready"){
						throw std::runtime_error(
							"live_service(" + name +
							") message is not 'ready' but '" + text + "'");
					}

					this->set_value(identifier, true);
				});
		}

		void on_binary(
			webservice::ws_identifier identifier,
			std::string&& text
		)override{
			module_.log(
				[this, identifier](logsys::stdlogb& os){
					os << "live service(" << name << ") on_binary identifier("
						<< identifier << ")";
				}, [this, &text]{
					if(text.size() > 50){
						text = text.substr(0, 47) + "[…]";
					}
					throw std::runtime_error(
						"live_service(" + name
						+ ") received unexpected binary message: '" + text
						+ "' (" + std::to_string(text.size()) + " bytes)");
				});
		}

		void send(std::string data){
			std::set< webservice::ws_identifier > ready_identifiers;
			module_.log(
				[this, &ready_identifiers](logsys::stdlogb& os){
					os << "live service(" << name
						<< ") send binary to sessions("
						<< io_tools::range_to_string(ready_identifiers) << ")";
				}, [this, &data, &ready_identifiers]{
					send_binary_if([&ready_identifiers](
							webservice::ws_identifier identifier,
							bool& ready
						){
							if(ready){
								ready_identifiers.emplace(identifier);
							}
							return std::exchange(ready, false);
						}, std::move(data));
				});
		}

		void on_exception(
			webservice::ws_identifier identifier,
			std::exception_ptr error
		)noexcept override{
			module_.exception_catching_log(
				[this, identifier](logsys::stdlogb& os){
					os << "live service(" << name << ") identifier("
						<< identifier << ")";
				}, [error]{
					std::rethrow_exception(error);
				});
		}

		void on_exception(std::exception_ptr error)noexcept override{
			module_.exception_catching_log(
				[this](logsys::stdlogb& os){
					os << "live service(" << name << ")";
				}, [error]{
					std::rethrow_exception(error);
				});
		}


		std::string const name;

	private:
		Module module_;
	};


	template < typename Component >
	class running_chains
		: public webservice::basic_json_ws_service<
			webservice::none_t, std::string >{
	public:
		running_chains(Component component, webservice::server& server)
			: component_(component)
			, server_(server)
			, locker_([]()noexcept{})
			, timer_(server_.get_io_context())
			, interval_(component_("min_interval_in_ms"_param)) {}


		void add(
			std::string const& chain,
			std::optional< std::size_t > exec_count = {}
		){
			component_.log(
				[&chain](logsys::stdlogb& os){
					os << "add live chain(" << chain << ")";
				}, [this, &chain, exec_count]{
					std::lock_guard lock(mutex_);
					if(shutdown_){
						throw std::logic_error("can not add chain(" + chain
							+ ") while shutdown");
					}

					if(chains_.find(chain) != chains_.end()){
						throw std::logic_error("chain(" + chain
							+ ") is already running");
					}
					chains_.try_emplace(
						chain,
						component_.system().enable_chain(chain),
						exec_count);

					send_text(nlohmann::json::object({{"run-chain", chain}}));

					if(chains_.size() == 1){
						timer_.expires_after(interval_);

						auto lock = locker_.make_first_lock("disposer_module::http_server::first");
						run();
					}
				});
		}

		void erase(std::string const& chain)noexcept{
			std::lock_guard lock(mutex_);
			lockless_erase(chain);
		}

		void run(){
			std::lock_guard lock(mutex_);
			if(shutdown_){
				return;
			}

			if(chains_.empty()) return;

			timer_.async_wait(
				[this, lock = locker_.make_lock("disposer_module::http_server")](
					boost::system::error_code const& error
				){
					lock.enter();

					if(error == boost::asio::error::operation_aborted){
						return;
					}

					timer_.expires_after(interval_);

					{
						std::lock_guard lock(mutex_);
						for(auto& pair: chains_){
							component_.exception_catching_log(
								[&pair](logsys::stdlogb& os){
									os << "server live exec chain("
										<< pair.first << ")";
								}, [this, &pair]{
									auto& [chain, count, counter] = pair.second;
									++counter;
									chain.exec();
									if(count && counter >= *count){
										lockless_erase(pair.first);
									}
								});
						}
					}

					run();
				});
		}


		void on_shutdown()noexcept final{
			std::unique_lock lock(mutex_);
			shutdown_ = true;
			lock.unlock();

			try{
				timer_.cancel();
			}catch(...){}

// 			server_.poll_while([this]()noexcept{
// 					return locker_.count() > 0;
// 				});

			lock.lock();
			for(auto& pair: chains_){
				lockless_erase(pair.first);
			}
		}


	protected:
		Component component_;

		webservice::server& server_;

		bool shutdown_{false};

		webservice::async_locker locker_;


		nlohmann::json running_chains_message(){
			return nlohmann::json::object({{"running-chains",
				[this]{
					auto chains = nlohmann::json::array();
					std::lock_guard lock(mutex_);
					for(auto const& [name, chain]: chains_){
						(void)chain;
						chains.push_back(name);
					}
					return chains;
				}()}});
		}


	private:
		void lockless_erase(std::string const& chain)noexcept{
			component_.exception_catching_log(
				[&chain](logsys::stdlogb& os){
					os << "erase live chain(" << chain << ")";
				}, [this, &chain]{
					if(chains_.erase(chain) == 0){
						throw std::logic_error("chain(" + chain
							+ ") is not running");
					}
					send_text(nlohmann::json::object({{"stop-chain", chain}}));
				});
		}

		struct running_chains_data{
			running_chains_data(
				disposer::enabled_chain&& chain,
				std::optional< std::size_t > exec_count
			)
				: chain(std::move(chain))
				, exec_count(exec_count) {}

			disposer::enabled_chain chain;
			std::optional< std::size_t > exec_count;
			std::size_t exec_counter = 0;
		};

		boost::asio::steady_timer timer_;

		std::chrono::milliseconds const interval_;
		std::chrono::high_resolution_clock::time_point last_exec_;
		std::mutex mutex_;
		std::map< std::string, running_chains_data > chains_;
	};


	template < typename Component >
	class control_service: public running_chains< Component >{
	public:
		using running_chains< Component >::running_chains;


	protected:
		using running_chains< Component >::component_;


	private:
		void on_server_connect(
			boost::asio::ip::tcp::socket&& socket,
			webservice::http_request&& req
		){
			this->async_server_connect(std::move(socket), std::move(req));
		}


		void on_open(webservice::ws_identifier identifier)override{
			component_.log(
				[identifier](logsys::stdlogb& os){
					os << "control service on_open identifier("
						<< identifier << ")";
				}, [this, identifier]{
					this->send_text(identifier, this->running_chains_message());
				});
		}

		void on_close(webservice::ws_identifier identifier)override{
			component_.log(
				[identifier](logsys::stdlogb& os){
					os << "control service on_close identifier("
						<< identifier << ")";
				});
		}

		void on_text(
			webservice::ws_identifier identifier,
			nlohmann::json&& data
		)override{
			auto success = component_.exception_catching_log(
				[identifier, &data](logsys::stdlogb& os){
					os << "control service on_text identifier("
						<< identifier << "); message(";
					try{
						os << data.dump();
					}catch(...){
						os << "ERROR: JSON serialization failed";
					}
					os << ")";
				},
				[this, &data]{
					auto const chain =
						data.at("chain").get< std::string >();
					auto const live = get_optional< bool >(data, "live");
					auto const exec = get_optional< int >(data, "exec");

					if(live && exec){
						throw std::logic_error("json message chain "
							"with both live and exec");
					}

					if(live){
						std::unique_lock lock(mutex_);
						if(*live){
							this->add(chain);
						}else{
							this->erase(chain);
						}
					}else if(exec){
						auto exec_count = *exec;
						if(exec_count < 1){
							throw std::logic_error("json message chain "
								"exec is less than 1 (" +
								std::to_string(exec_count) + ")");
						}

						std::unique_lock lock(mutex_);
						this->add(
							chain, static_cast< std::size_t >(exec_count));
					}else{
						throw std::logic_error("json message chain "
							"without live and exec");
					}
				});

			if(!success){
				component_.exception_catching_log(
					[](logsys::stdlogb& os){
						os << "send error message";
					},
					[this, identifier]{
						this->send_text(identifier, nlohmann::json::object({
								{"error", io_tools::make_string(
									"see server log file session(",
									identifier, ")")}
							}));
						this->send_text(this->running_chains_message());
					});
			}
		}

		void on_binary(
			webservice::ws_identifier identifier,
			std::string&& text
		)override{
			component_.log(
				[identifier](logsys::stdlogb& os){
					os << "control service on_binary identifier("
						<< identifier << ")";
				}, [&text]{
					throw std::runtime_error(
						"received unexpected binary message: '" + text + "'");
				});
		}

		void on_exception(
			webservice::ws_identifier identifier,
			std::exception_ptr error
		)noexcept override{
			component_.exception_catching_log(
				[identifier](logsys::stdlogb& os){
					os << "control service identifier(" << identifier << ")";
				}, [error]{
					std::rethrow_exception(error);
				});
		}

		void on_exception(std::exception_ptr error)noexcept override{
			component_.exception_catching_log(
				[](logsys::stdlogb& os){
					os << "control service";
				}, [error]{
					std::rethrow_exception(error);
				});
		}



		std::shared_mutex mutex_;
	};


	template < typename Component >
	class ws_service_handler: public webservice::ws_service_handler{
	public:
		ws_service_handler(Component component)
			: component_(component) {}


	private:
		void on_exception(std::exception_ptr error)noexcept override{
			component_.exception_catching_log(
				[](logsys::stdlogb& os){
					os << "service handler";
				}, [error]{
					std::rethrow_exception(error);
				});
		}


		Component component_;
	};


	template < typename Component >
	class error_handler: public webservice::error_handler{
	public:
		error_handler(Component component)
			: component_(component) {}


	private:
		void on_exception(std::exception_ptr error)noexcept override{
			component_.exception_catching_log(
				[](logsys::stdlogb& os){
					os << "server thread";
				}, [error]{
					std::rethrow_exception(error);
				});
		}


		Component component_;
	};


	template < typename Component >
	class file_request_handler: public webservice::file_request_handler{
	public:
		file_request_handler(Component component)
			: webservice::file_request_handler(component("root"_param))
			, component_(component) {}

		void operator()(
			webservice::http_request&& req,
			webservice::http_response&& send
		)override{
			component_.log(
				[resource = req.target()](logsys::stdlogb& os){
					os << "http file handler get (" << resource << ")";
				}, [this, &req, &send]{
					webservice::file_request_handler::operator()(
						std::move(req), std::move(send));
				});
		}

	private:
		void on_exception(std::exception_ptr error)noexcept override{
			component_.exception_catching_log(
				[](logsys::stdlogb& os){
					os << "http file handler";
				}, [error]{
					std::rethrow_exception(error);
				});
		}


		Component component_;
	};


	template < typename Component >
	class server{
	public:
		server(Component component)
			: server(
					std::make_unique< ws_service_handler< Component > >(
						component),
					component
				) {}

		server(server&&) = default;


		void shutdown(){
			server_->shutdown();
			server_->block();
		}


		template < typename Module >
		auto add_live_service(Module module){
			std::string service_name = module("service_name"_param);
			return component_.log(
				[&service_name](logsys::stdlogb& os){
					os << "add WebSocket service(" << service_name << ")";
				}, [this, module, &service_name]{
					if(service_name.empty() || service_name[0] != '/'){
						service_name = "/" + service_name;
					}

					class live{
					public:
						live(
							live_service< Module >& service,
							std::string service_name,
							ws_service_handler< Component >& handler,
							Component component
						)
							: service(service)
							, service_name_(std::move(service_name))
							, handler_(handler)
							, component_(component) {}

						live(live const&) = delete;

						~live(){
							component_.exception_catching_log(
								[this](logsys::stdlogb& os){
									os << "erase WebSocket service("
										<< service_name_ << ")";
								}, [this]{
									handler_.erase_service(service_name_);
								});
						}

						live_service< Module >& service;

					private:
						std::string service_name_;
						ws_service_handler< Component >& handler_;
						Component component_;
					};

					auto service =
						std::make_unique< live_service< Module > >(module);

					service->set_ping_time(std::chrono::milliseconds(
						component_("timeout_in_ms"_param)));

					auto result = std::make_unique< live >(
						*service, service_name, ws_handler_, component_);

					ws_handler_.add_service(
						std::move(service_name), std::move(service));

					return result;
				});
		}


	private:
		server(
			std::unique_ptr< ws_service_handler< Component > >&& ws_handler,
			Component component
		)
			: component_(component)
			, ws_handler_(*ws_handler.get())
			, server_(std::make_unique< webservice::server >(
					std::make_unique< file_request_handler< Component > >(
						component),
					std::move(ws_handler),
					std::make_unique< error_handler< Component > >(component),
					boost::asio::ip::make_address(component("address"_param)),
					component("port"_param),
					component("thread_count"_param)
				))
		{
			component_.log(
				[](logsys::stdlogb& os){
					os << "add WebSocket control service on root(/)";
				}, [this]{
					auto control_service =
						std::make_unique< class control_service< Component > >(
							component_, *server_);

					control_service->set_ping_time(std::chrono::milliseconds(
						component_("timeout_in_ms"_param)));

					ws_handler_.add_service("/", std::move(control_service));
				});
		}

		Component component_;
		ws_service_handler< Component >& ws_handler_;
		std::unique_ptr< webservice::server > server_;
	};


	void init(
		std::string const& name,
		disposer::declarant& declarant
	){
		using namespace std::literals::string_literals;
		using namespace disposer;

		auto init = generate_component(
			"the web server has a web socket named 'controller' which can be "
			"used to enable and disable the execution of a chains at regular "
			"intervals",
			component_configure(
				make("root"_param, free_type_c< std::string >,
					"path to the http server root directory"),
				make("address"_param, free_type_c< std::string >,
					"address of the server",
					default_value("0::0")),
				make("port"_param, free_type_c< std::uint16_t >,
					"port of the server",
					default_value(8000)),
				make("thread_count"_param, free_type_c< std::uint8_t >,
					"thread count to process http and websocket requests",
					default_value(2),
					verify_value_fn([](std::uint8_t value){
						if(value > 0) return;
						throw std::logic_error("must be greater or equal 1");
					})),
				make("min_interval_in_ms"_param, free_type_c< std::uint32_t >,
					"time between exec calls on live chains",
					default_value(50)),
				make("timeout_in_ms"_param, free_type_c< std::uint32_t >,
					"time out for websocket connections",
					default_value(15000))
			),
			component_init_fn([](auto component){
				return server< decltype(component) >(component);
			}),
			component_modules(
				make("websocket"_module, generate_module(
					"send data via websocket to all connected clients",
					module_configure(
						make("data"_in, free_type_c< std::string >,
							"data to be send (only last entry is send if "
							"there are more then one per exec)"),
						make("service_name"_param, free_type_c< std::string >,
							"name of the websocket service")
					),
					module_init_fn([](auto module){
						return module.component.state()
							.add_live_service(module);
					}),
					exec_fn([](auto& module){
						auto list = module("data"_in).references();
						if(list.empty()) return;
						auto iter = list.end();
						--iter;
						module.state()->service.send(*iter);
					}),
					no_overtaking
				))
			)
		);

		init(name, declarant);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}

