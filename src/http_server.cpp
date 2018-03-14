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


	template < typename Module >
	class live_service: public webservice::basic_ws_service< std::string >{
	public:
		live_service(Module module)
			: name(module("service_name"_param))
			, module_(module) {}

		void on_open(std::uintptr_t identifier)override{
			module_.log(
				[this, identifier](logsys::stdlogb& os){
					os << "live service " << name << " on_open identifier("
						<< identifier << ")";
				}, [this, identifier]{
					std::unique_lock lock(mutex_);
					ready_.emplace(identifier, false);
				});
		}

		void on_close(std::uintptr_t identifier)override{
			module_.log(
				[this, identifier](logsys::stdlogb& os){
					os << "live service " << name << " on_close identifier("
						<< identifier << ")";
				}, [this, identifier]{
					std::unique_lock lock(mutex_);
					ready_.erase(identifier);
				});
		}

		void on_text(
			std::uintptr_t identifier,
			std::string&& text
		)override{
			module_.log(
				[this, identifier](logsys::stdlogb& os){
					os << "live service " << name << " on_text identifier("
						<< identifier << ")";
				}, [this, identifier, &text]{
					if(text != "ready"){
						throw std::runtime_error(
							"live_service '" + name +
							"' message is not 'ready' but '" + text + "'");
					}
					std::shared_lock lock(mutex_);
					ready_[identifier] = true;
				});
		}

		void on_binary(
			std::uintptr_t identifier,
			std::string&& text
		)override{
			module_.log(
				[this, identifier](logsys::stdlogb& os){
					os << "live service " << name << " on_close identifier("
						<< identifier << ")";
				}, [this, identifier, &text]{
					throw std::runtime_error(
						"live_service '" + name
						+ "' received unexpected binary message: '" + text
						+ "'");
				});
		}

		void send(std::string data){
			module_.log(
				[this](logsys::stdlogb& os){
					os << "live service " << name << " send binary";
				}, [this, &data]{
					std::set< std::uintptr_t > ready_sessions;
					{
						std::shared_lock lock(mutex_);
						for(auto& [identifier, ready]: ready_){
							if(!ready) continue;
							ready = false;
							ready_sessions.insert(
								ready_sessions.end(), identifier);
						}
					}
					send_text(ready_sessions, std::move(data));
				});
		}

		void on_error(
			std::uintptr_t identifier,
			webservice::ws_handler_location location,
			boost::system::error_code ec
		)override{
			module_.log(
				[identifier, location, ec](logsys::stdlogb& os){
					os << "live service identifier(" << identifier
						<< ") location(" << to_string_view(location)
						<< "); error code: " << ec.message() << " (Warning)";
				});
		}

		void on_exception(
			std::uintptr_t identifier,
			std::exception_ptr error
		)noexcept override{
			module_.exception_catching_log(
				[identifier](logsys::stdlogb& os){
					os << "live service identifier(" << identifier << ")";
				}, [error]{
					std::rethrow_exception(error);
				});
		}


		std::string const name;

	private:
		Module module_;
		std::shared_mutex mutex_;
		std::map< std::uintptr_t, std::atomic< bool > > ready_;
	};


	template < typename Component >
	class live_chain{
	public:
		live_chain(Component component, std::string const& chain)
			: component_(component)
			, chain_(chain)
			, active_(true)
			, thread_([this]()noexcept{
				while(active_){
					component_.exception_catching_log(
						[](logsys::stdlogb& os){
							os << "server live exec";
						}, [this]{
							auto const min_interval_in_ms =
								component_("min_interval_in_ms"_param);
							auto const interval =
								std::chrono::milliseconds(min_interval_in_ms);

							auto chain =
								component_.system().enable_chain(chain_);
							while(active_){
								component_.exception_catching_log(
									[](logsys::stdlogb& os){
										os << "server live exec loop";
									}, [this, &chain, interval]{
										while(active_){
											auto const start = std::chrono
												::high_resolution_clock::now();
											chain.exec();
											auto const end = std::chrono
												::high_resolution_clock::now();
											std::chrono::duration< double,
												std::milli > const diff
													= end - start;
											if(diff < interval){
												if(!active_) break;
												std::this_thread::sleep_for(
													interval - diff);
											}
										}
									});
							}
						});
				}
			}) {}

		live_chain(live_chain const&) = delete;
		live_chain(live_chain&&) = delete;

		~live_chain(){
			active_ = false;
			thread_.join();
		}

	private:
		Component component_;
		std::string const chain_;
		std::atomic< bool > active_;
		std::thread thread_;
	};


	template < typename Component >
	class control_service
		: public webservice::basic_json_ws_service< std::string >{
	public:
		control_service(Component component)
			: component_(component) {}


	private:
		void on_open(std::uintptr_t identifier)override{
			component_.log(
				[identifier](logsys::stdlogb& os){
					os << "control service on_open identifier("
						<< identifier << ")";
				}, [this, identifier]{
					send_text(identifier, nlohmann::json{"chains", [this]{
							nlohmann::json chains;
							std::shared_lock lock(mutex_);
							for(auto const& [name, chain]: active_chains_){
								(void)chain;
								chains.push_back(name);
							}
							return chains;
						}()});
				});
		}

		void on_close(std::uintptr_t identifier)override{
			component_.log(
				[identifier](logsys::stdlogb& os){
					os << "control service on_close identifier("
						<< identifier << ")";
				});
		}

		void on_text(
			std::uintptr_t identifier,
			nlohmann::json&& data
		)override{
			auto answer = component_.exception_catching_log(
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
					auto const token = data.at("token").get< std::string >();
					auto const commands = data.at("commands");

					std::unique_lock lock(mutex_);
					for(auto const& data: commands){
						auto const chain =
							data.at("chain").get< std::string >();
						auto const live =
							data.at("live").get< bool >();

						if(live){
							active_chains_.try_emplace(
								chain, component_, chain);
						}else{
							active_chains_.erase(chain);
						}
					}

					return nlohmann::json{"token", token};
				});

			component_.exception_catching_log(
				[identifier, &answer](logsys::stdlogb& os){
					os << "control service on_text identifier("
						<< identifier << "); ";
					if(answer){
						os << "send answer(";
						try{
							os << answer->dump();
						}catch(...){
							os << "ERROR: JSON serialization failed";
						}
						os << ")";
					}else{
						os << "send error message";
					}
				},
				[this, identifier, &answer, &data]{
					if(answer){
						send_text(identifier, *answer);
					}else{
						std::string message = "Execution of command failed";
						try{
							message += " (" + data.dump() + ")";
						}catch(...){
							message += " (ERROR: JSON serialization failed)";
						}
						send_text(identifier, nlohmann::json{"error", message});
					}
				});
		}

		void on_binary(
			std::uintptr_t identifier,
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

		void on_error(
			std::uintptr_t identifier,
			webservice::ws_handler_location location,
			boost::system::error_code ec
		)override{
			component_.log(
				[identifier, location, ec](logsys::stdlogb& os){
					os << "control service identifier(" << identifier
						<< ") location(" << to_string_view(location)
						<< "); error code: " << ec.message() << " (Warning)";
				});
		}

		void on_exception(
			std::uintptr_t identifier,
			std::exception_ptr error
		)noexcept override{
			component_.exception_catching_log(
				[identifier](logsys::stdlogb& os){
					os << "control service identifier(" << identifier << ")";
				}, [error]{
					std::rethrow_exception(error);
				});
		}


		Component component_;

		std::shared_mutex mutex_;

		std::map< std::string, live_chain< Component > > active_chains_;
	};


	template < typename Component >
	class ws_service_handler: public webservice::ws_service_handler{
	public:
		ws_service_handler(Component component)
			: component_(component) {}


	private:
		void on_error(
			webservice::ws_server_session* session,
			std::string const& resource,
			webservice::ws_handler_location location,
			boost::system::error_code ec
		)override{
			auto const identifier = reinterpret_cast< std::intptr_t >(session);
			component_.log(
				[identifier, &resource, location, ec](logsys::stdlogb& os){
					os << "service handler identifier(" << identifier
						<< ") resource(" << resource << ") location("
						<< to_string_view(location) << "); error code: "
						<< ec.message() << "(Warning)";
				});
		}

		void on_exception(
			webservice::ws_server_session* session,
			std::string const& resource,
			std::exception_ptr error
		)noexcept override{
			auto const identifier = reinterpret_cast< std::intptr_t >(session);
			component_.exception_catching_log(
				[identifier, &resource](logsys::stdlogb& os){
					os << "service handler identifier(" << identifier
						<< ") resource(" << resource << ")";
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
		void on_error(boost::system::error_code ec)override{
			component_.log(
				[ec](logsys::stdlogb& os){
					os << "server thread error code: " << ec.message()
						<< "(Warning)";
				});
		}

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
				[this, resource = req.target()](logsys::stdlogb& os){
					os << "http file handler get (" << resource << ")";
				}, [this, &req, &send]{
					webservice::file_request_handler::operator()(
						std::move(req), std::move(send));
				});
		}

	private:
		void on_error(
			webservice::http_request_location location,
			boost::system::error_code ec
		)override{
			component_.log(
				[ec, location](logsys::stdlogb& os){
					os << "http file handler location("
						<< to_string_view(location)
						<< ") error code: " << ec.message() << "(Warning)";
				});
		}

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


		template < typename Module >
		auto add_live_service(Module module){
			std::string service_name = module("service_name"_param);
			if(service_name.empty() || service_name[0] != '/'){
				service_name = "/" + service_name;
			}

			struct live{
				live_service< Module >& service;
				ws_service_handler< Component >& handler;

				~live(){
					try{
						handler.erase_service(service.name);
					}catch(...){
						assert(false);
					}
				}
			};

			auto service = std::make_unique< live_service< Module > >(module);
			live result{*service, ws_handler_};
			ws_handler_.add_service(service_name, std::move(service));
			return result;
		}


	private:
		server(
			std::unique_ptr< ws_service_handler< Component > >&& ws_handler,
			Component component
		)
			: ws_handler_(*ws_handler.get())
			, server_(
				std::make_unique< file_request_handler< Component > >(
					component),
				std::move(ws_handler),
				std::make_unique< error_handler< Component > >(component),
				boost::asio::ip::make_address(component("address"_param)),
				component("port"_param),
				component("thread_count"_param))
		{
			ws_handler_.add_service("/",
				std::make_unique< control_service< Component > >(component));
		}

		ws_service_handler< Component >& ws_handler_;
		webservice::server server_;
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
					default_value(50))
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
						module.state().service.send(*iter);
					}),
					no_overtaking
				))
			)
		);

		init(name, declarant);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}

