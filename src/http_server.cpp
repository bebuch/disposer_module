#include "http_server.hpp"

#include <disposer/disposer.hpp>

#include <http/server_server.hpp>
#include <http/server_file_request_handler.hpp>
#include <http/server_lambda_request_handler.hpp>
#include <http/websocket_server_request_handler.hpp>
#include <http/websocket_server_json_service.hpp>

#include <boost/property_tree/ptree.hpp>


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
					auto& chain = disposer_.get_chain(chain_);
					chain.enable();
					while(active_){
						chain.exec();
					}
					chain.disable();
				}
			}) {}

		live_chain(live_chain const&) = delete;
		live_chain(live_chain&&) = delete;

		~live_chain(){
			active_ = false;
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
						auto chain = data.get< std::string >("command.chain");
						auto on = data.get< bool >("command.live");
						std::lock_guard< std::mutex > lock(mutex_);
						if(on){
							active_chains_.try_emplace(chain, disposer_, chain);
						}else{
							active_chains_.erase(chain);
						}
						boost::property_tree::ptree answer;
						answer.put("live.chain", chain);
						answer.put("live.is", on);
						send(answer);
					}catch(...){}
				},
				http::websocket::server::data_callback_fn(),
				[this](http::server::connection_ptr const& con){
					std::lock_guard< std::mutex > lock(mutex_);
					controller_.emplace(con);
				},
				[this](http::server::connection_ptr const& con){
					std::lock_guard< std::mutex > lock(mutex_);
					controller_.erase(con);
				}
			)
			, disposer_(disposer) {}

	private:
		void send(boost::property_tree::ptree const& data){
			for(auto& controller: controller_){
				send_json(data, controller);
			}
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
					websocket_handler_.shutdown();
				}
			)
			, control_service_(disposer)
			, http_file_handler_(http_root_path)
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
		/// \brief WebSocket service for disposer control messages
		control_service control_service_;

		/// \brief WebSocket live services
		std::map< std::string, live_service > websocket_services_;

		/// \brief Handler for Websocket-Requests
		http::websocket::server::request_handler websocket_handler_;

		/// \brief Handler for normal HTTP-File-Requests
		http::server::file_request_handler http_file_handler_;
	};


	struct http_server_impl{
		http_server_impl(
				::disposer::disposer& disposer,
				std::string const& root,
				std::size_t port)
			: disposer(disposer)
			, handler(disposer, root)
			, server(std::to_string(port), handler, 1) {}

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

	http_server::~http_server() = default;



}

