//-----------------------------------------------------------------------------
// Copyright (c) 2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <disposer/component.hpp>
#include <disposer/module.hpp>

#include <http/server_server.hpp>
#include <http/server_file_request_handler.hpp>
#include <http/server_lambda_request_handler.hpp>
#include <http/websocket_server_request_handler.hpp>
#include <http/websocket_server_json_service.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/dll.hpp>


namespace disposer_module::http_server_component{


	using namespace disposer::literals;
	namespace hana = boost::hana;


	class live_service: public http::websocket::server::json_service{
	public:
		live_service(std::string const& ready_signal):
			http::websocket::server::json_service(
				[this](
					boost::property_tree::ptree const& data,
					http::server::connection_ptr const& con
				){
					try{
						if(!data.get< bool >(ready_signal_)) return;
						std::lock_guard< std::mutex > lock(mutex_);
						ready_.at(con) = true;
					}catch(...){}
				},
				http::websocket::server::data_callback_fn(),
				[this](http::server::connection_ptr const& con){
					std::lock_guard< std::mutex > lock(mutex_);
					ready_.emplace(con, false);
				},
				[this](http::server::connection_ptr const& con){
					std::lock_guard< std::mutex > lock(mutex_);
					ready_.erase(con);
				}
			),
			ready_signal_(ready_signal) {}

		void send(std::string const& data){
			std::lock_guard< std::mutex > lock(mutex_);
			for(auto& pair: ready_){
				if(!pair.second) continue;
				pair.second = false;
				send_binary(data, pair.first);
			}
		}


	private:
		std::string const ready_signal_;

		std::mutex mutex_;
		std::map< http::server::connection_ptr, bool > ready_;
	};


	template < typename Component >
	class live_chain{
	public:
		live_chain(Component& component, std::string const& chain)
			: component_(component)
			, chain_(chain)
			, active_(true)
			, thread_([this]{
				while(active_){
					logsys::exception_catching_log(
						[this](logsys::stdlogb& os){
							os << "chain(" << chain_ << ") server live exec";
						}, [this]{
							auto& chain =
								component_.disposer().get_chain(chain_);
							disposer::chain_enable_guard enable(chain);
							while(active_){
								logsys::exception_catching_log(
									[this](logsys::stdlogb& os){
										os << "chain(" << chain_
											<< ") server live exec loop";
									}, [this, &chain]{
										while(active_){
											chain.exec();
										}
									});
							}
						});
				}
			}) {}

		live_chain(live_chain const&) = delete;
		live_chain(live_chain&&) = delete;

		void shutdown_hint(){
			active_ = false;
		}

		~live_chain(){
			shutdown_hint();
			thread_.join();
		}

	private:
		Component& component_;
		std::string chain_;
		std::atomic< bool > active_;
		std::thread thread_;
	};

	template < typename Component >
	class control_service: public http::websocket::server::json_service{
	public:
		control_service(Component& component)
			: http::websocket::server::json_service(
				[this](
					boost::property_tree::ptree const& data,
					http::server::connection_ptr const& /*con*/
				){
					try{
						auto chain_name = data.get_optional< std::string >(
							"command.chain");
						if(!chain_name) return;

						auto live = data.get_optional< bool >("command.live");
						auto count = data.get_optional< std::size_t >(
							"command.exec");
						if(!live && !count) return;

						std::lock_guard< std::mutex > lock(mutex_);
						if(live){
							if(*live){
								active_chains_.try_emplace(
									*chain_name, component_, *chain_name);
							}else{
								active_chains_.erase(*chain_name);
							}
							send(live_answer(*chain_name, *live));
						}

						if(count){
							if(active_chains_.find(*chain_name)
								!= active_chains_.end()){
								boost::property_tree::ptree answer;
								answer.put("error.chain", *chain_name);
								answer.put("error.message",
									"can not exec while live is enabled");
								send(answer);

								throw std::runtime_error("can not exec chain '"
									+ *chain_name
									+ "' while it is in live mode");
							}

							auto& chain =
								component_.disposer().get_chain(*chain_name);
							disposer::chain_enable_guard enable(chain);

							boost::property_tree::ptree answer;
							answer.put("exec.chain", *chain_name);
							answer.put("exec.count", *count);
							send(answer);

							for(std::size_t i = 0; i < *count; ++i){
								chain.exec();
							}
						}
					}catch(...){}
				},
				http::websocket::server::data_callback_fn(),
				[this](http::server::connection_ptr const& con){
					std::lock_guard< std::mutex > lock(mutex_);
					controller_.emplace(con);
					for(auto const& [name, chain]: active_chains_){
						(void)chain;
						send_json(live_answer(name, true), con);
					}
				},
				[this](http::server::connection_ptr const& con){
					std::lock_guard< std::mutex > lock(mutex_);
					controller_.erase(con);
				}
			),
			component_(component) {}

		~control_service(){
			shutdown();
		}

		void shutdown()noexcept{
			std::lock_guard< std::mutex > lock(mutex_);

			// tell all threads to get done (not necessary, only speed up)
			for(auto& [name, chain]: active_chains_){
				(void)name;
				chain.shutdown_hint();
			}

			active_chains_.clear();
		}

	private:
		void send(boost::property_tree::ptree const& data){
			for(auto& controller: controller_){
				send_json(data, controller);
			}
		}

		boost::property_tree::ptree
		live_answer(std::string const& name, bool enabled){
			boost::property_tree::ptree answer;
			answer.put("live.chain", name);
			answer.put("live.is", enabled);
			return answer;
		}

		Component& component_;

		std::mutex mutex_;

		std::set< http::server::connection_ptr > controller_;

		std::map< std::string, live_chain< Component > > active_chains_;
	};


	class websocket_identifier{
	private:
		websocket_identifier(std::string const& name)
			: name(name) {}

		std::string const& name;

		template < typename Component >
		friend class request_handler;
	};

	struct http_server_init_t{
		websocket_identifier key;
		bool success;
	};


	template < typename Component >
	class request_handler: public http::server::lambda_request_handler{
	public:
		request_handler(
				Component& component,
				std::string const& http_root_path)
			: http::server::lambda_request_handler(
				[this](
					http::server::connection_ptr con,
					http::request const& req,
					http::reply& rep
				){
					return
						websocket_handler_.handle_request(con, req, rep) ||
						http_file_handler_.handle_request(con, req, rep);
				},
				[this]{
					control_service_.shutdown();
					websocket_handler_.shutdown();
				}
			)
			, http_file_handler_(http_root_path)
			, control_service_(component)
		{
			auto success = websocket_handler_.register_service(
				"controller", control_service_);

			if(!success){
				throw std::runtime_error("controller service already exist");
			}
		}

		http_server_init_t init(std::string const& service_name){
			auto [iter, success] = websocket_services_.try_emplace(
				service_name, "ready");

			if(success){
				websocket_handler_.register_service(service_name, iter->second);
			}

			return http_server_init_t{iter->first, success};
		}

		websocket_identifier unique_init(std::string const& service_name){
			auto [key, success] = init(service_name);

			if(success) return key;

			throw std::runtime_error("service name already exist: "
				+ service_name);
		}

		void uninit(websocket_identifier key){
			websocket_handler_.shutdown_service(key.name);
			websocket_services_.erase(key.name);
		}

		void send(websocket_identifier key, std::string const& data){
			websocket_services_.at(key.name).send(data);
		}


	private:
		/// \brief Handler for normal HTTP-File-Requests
		http::server::file_request_handler http_file_handler_;

		/// \brief Handler for Websocket-Requests
		http::websocket::server::request_handler websocket_handler_;

		/// \brief WebSocket live services
		std::map< std::string, live_service > websocket_services_;

		/// \brief WebSocket service for disposer control messages
		control_service< Component > control_service_;
	};


	template < typename Component >
	class http_server{
	public:
		http_server(
			Component& component,
			std::string const& root,
			std::uint16_t port,
			std::size_t thread_count
		)
			: handler_(component, root)
			, server_(std::to_string(port), handler_, thread_count) {}

		http_server(http_server&&) = default;

		~http_server(){
			shutdown();
		}

		void shutdown(){
			handler_.shutdown();
		}

		http_server_init_t init(std::string const& service_name){
			return handler_.init(service_name);
		}

		websocket_identifier unique_init(std::string const& service_name){
			return handler_.unique_init(service_name);
		}

		void uninit(websocket_identifier key){
			return handler_.uninit(key);
		}

		void send(websocket_identifier key, std::string const& data){
			return handler_.send(key, data);
		}


	private:
		request_handler< Component > handler_;
		http::server::server server_;
	};


	void init_component(
		std::string const& name,
		disposer::component_declarant& declarant
	){
		using namespace std::literals::string_literals;
		using namespace disposer;

		auto init = component_register_fn(
			component_configure(
				"root"_param(hana::type_c< std::string >),
				"port"_param(hana::type_c< std::uint16_t >,
					default_values(std::uint16_t(8000))),
				"thread_count"_param(hana::type_c< std::size_t >,
					default_values(std::size_t(2)),
					value_verify([](auto const& /*iop*/, std::size_t value){
						if(value > 0) return;
						throw std::logic_error("must be greater or equal 1");
					}))
			),
			disposer::component_init([](auto& component){
				return http_server< decltype(component) >(
					component,
					component("root"_param).get(),
					component("port"_param).get(),
					component("thread_count"_param).get());
			}),
			component_modules(
				"websocket"_module([](auto& component){
					return module_register_fn(
						module_configure(
							"data"_in(hana::type_c< std::string >, required),
							"service_name"_param(hana::type_c< std::string >)
						),
						normal_id_increase(),
						module_enable([&component](auto const& module){
							auto init = component.data()
								.init(module("service_name"_param).get());
							auto key = init.key;
							return [&component, key]
								(auto& module, std::size_t /*id*/){
									auto list =
										module("data"_in).get_references();
									if(list.empty()) return;
									auto iter = list.end();
									--iter;
									component.data().send(key,
										iter->second.get());
								};
						})
					);
				})
			)
		);

		init(name, declarant);
	}

	BOOST_DLL_AUTO_ALIAS(init_component)


}

