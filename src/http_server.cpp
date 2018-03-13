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


	class live_service: public webservice::basic_ws_service< std::string >{
	public:
		live_service(std::string name)
			: name(std::move(name)) {}

		void on_open(std::uintptr_t identifier)override{
			std::unique_lock lock(mutex_);
			ready_.emplace(identifier, false);
		}

		void on_close(std::uintptr_t identifier)override{
			std::unique_lock lock(mutex_);
			ready_.erase(identifier);
		}

		void on_text(
			std::uintptr_t identifier,
			std::string&& text
		)override{
			if(text != "ready"){
				throw std::runtime_error(
					"live_service '" + name + "' message is not 'ready' but '"
					+ text + "'");
			}
			std::shared_lock lock(mutex_);
			ready_[identifier] = true;
		}

		void on_binary(
			std::uintptr_t identifier,
			std::string&& text
		)override{
			throw std::runtime_error(
				"live_service '" + name
				+ "' received unexpected binary message: '" + text + "'");
		}

		void send(std::string data){
			std::set< std::uintptr_t > ready_sessions;
			{
				std::shared_lock lock(mutex_);
				for(auto& [identifier, ready]: ready_){
					if(!ready) continue;
					ready = false;
					ready_sessions.insert(ready_sessions.end(), identifier);
				}
			}
			send_text(ready_sessions, std::move(data));
		}


		std::string const name;

	private:
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

		void on_open(std::uintptr_t identifier)override{
			send_text(identifier, nlohmann::json{"chains", [this]{
					nlohmann::json chains;
					std::shared_lock lock(mutex_);
					for(auto const& [name, chain]: active_chains_){
						(void)chain;
						chains.push_back(name);
					}
					return chains;
				}()});
		}

		void on_text(
			std::uintptr_t identifier,
			nlohmann::json&& data
		)override{
			auto answer = component_.exception_catching_log(
				[&data](logsys::stdlogb& os){
					os << "json message: ";
					try{
						os << data.dump();
					}catch(...){
						os << "ERROR: JSON serialization failed";
					}
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
				[](logsys::stdlogb& os){
					os << "Send error message";
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
			std::uintptr_t /*identifier*/,
			std::string&& text
		)override{
			throw std::runtime_error(
				"control_service received unexpected binary message: '"
				+ text + "'");
		}


	private:
		Component component_;

		std::shared_mutex mutex_;

		std::map< std::string, live_chain< Component > > active_chains_;
	};


	class server{
	public:
		template < typename Component >
		server(Component component)
			: server(
					std::make_unique< webservice::ws_service_handler >(),
					component
				) {}

		server(server&&) = default;


		auto add_live_service(std::string service_name){
			if(service_name.empty() || service_name[0] != '/'){
				service_name = "/" + service_name;
			}

			struct live{
				live_service& service;
				webservice::ws_service_handler& handler;

				~live(){
					try{
						handler.erase_service(service.name);
					}catch(...){
						assert(false);
					}
				}
			};

			auto service = std::make_unique< live_service >(service_name);
			live result{*service, ws_handler_};
			ws_handler_.add_service(service_name, std::move(service));
			return result;
		}


	private:
		template < typename Component >
		server(
			std::unique_ptr< webservice::ws_service_handler >&& ws_handler,
			Component component
		)
			: ws_handler_(*ws_handler.get())
			, server_(
				std::make_unique< webservice::file_request_handler >(
						component("root"_param)),
				std::move(ws_handler),
				std::make_unique< webservice::error_handler >(),
				boost::asio::ip::make_address(component("address"_param)),
				component("port"_param),
				component("thread_count"_param))
		{
			ws_handler_.add_service("/",
				std::make_unique< control_service< Component > >(component));
		}

		webservice::ws_service_handler& ws_handler_;
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
				return server(component);
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
							.add_live_service(module("service_name"_param));
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

