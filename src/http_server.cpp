#include "http_server.hpp"

#include <disposer/disposer.hpp>

#include <http/server_server.hpp>
#include <http/server_file_request_handler.hpp>
#include <http/server_lambda_request_handler.hpp>
#include <http/websocket_server_request_handler.hpp>
#include <http/websocket_server_json_service.hpp>

#include <boost/property_tree/ptree.hpp>

#include <set>


namespace disposer_module{


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


	class live_chain{
	public:
		live_chain(::disposer::disposer& disposer, std::string const& chain)
			: disposer_(disposer)
			, chain_(chain)
			, active_(true)
			, thread_([this]{
				while(active_){
					logsys::exception_catching_log(
						[this](logsys::stdlogb& os){
							os << "chain(" << chain_ << ") server live exec";
						}, [this]{
							auto& chain = disposer_.get_chain(chain_);
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
		::disposer::disposer& disposer_;
		std::string chain_;
		std::atomic< bool > active_;
		std::thread thread_;
	};

	class control_service: public http::websocket::server::json_service{
	public:
		control_service(::disposer::disposer& disposer)
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
									*chain_name, disposer_, *chain_name);
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

							auto& chain = disposer_.get_chain(*chain_name);
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
			)
			, disposer_(disposer) {}

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

		::disposer::disposer& disposer_;

		std::mutex mutex_;

		std::set< http::server::connection_ptr > controller_;

		std::map< std::string, live_chain > active_chains_;
	};


	class request_handler: public http::server::lambda_request_handler{
	public:
		request_handler(
				::disposer::disposer& disposer,
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
			, control_service_(disposer)
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
		control_service control_service_;
	};


	struct http_server_impl{
		http_server_impl(
				::disposer::disposer& disposer,
				std::string const& root,
				std::size_t port)
			: disposer(disposer)
			, handler(disposer, root)
			, server(std::to_string(port), handler, 2) {}

		::disposer::disposer const& disposer;
		request_handler handler;
		http::server::server server;
	};


	http_server::http_server(
			::disposer::disposer& disposer,
			std::string const& root,
			std::size_t port)
		: impl_(std::make_unique< http_server_impl >(disposer, root, port)) {}

	http_server::http_server(http_server&&) = default;

	http_server_init_t http_server::init(std::string const& service_name){
		return impl_->handler.init(service_name);
	}

	websocket_identifier http_server::unique_init(
		std::string const& service_name
	){
		return impl_->handler.unique_init(service_name);
	}

	void http_server::uninit(websocket_identifier key){
		return impl_->handler.uninit(key);
	}

	void http_server::send(websocket_identifier key, std::string const& data){
		return impl_->handler.send(key, data);
	}

	http_server::~http_server(){
		if(impl_) impl_->handler.shutdown();
	}



}

