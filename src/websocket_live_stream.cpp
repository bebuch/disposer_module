//-----------------------------------------------------------------------------
// Copyright (c) 2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <disposer/module.hpp>

#include <http/server_server.hpp>
#include <http/server_file_request_handler.hpp>
#include <http/server_lambda_request_handler.hpp>
#include <http/websocket_server_request_handler.hpp>
#include <http/websocket_server_json_service.hpp>

#include <boost/dll.hpp>
#include <boost/property_tree/ptree.hpp>


namespace disposer_module{ namespace websocket_live_stream{


	class service: public http::websocket::server::json_service{
	public:
		service(std::string const& ready_signal):
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


	class request_handler: public http::server::lambda_request_handler{
	public:
		request_handler(
			std::string const& http_root_path,
			std::string const& websocket_name,
			std::string const& ready_signal
		):
			http::server::lambda_request_handler(
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
			),
			websocket_service_(ready_signal),
			http_file_handler_(http_root_path)
		{
			websocket_handler_.register_service(
				websocket_name, websocket_service_
			);
		}

		void send(std::string const& data){
			websocket_service_.send(data);
		}


	private:
		/// \brief The WebSocket-Service
		service websocket_service_;

		/// \brief Handler for Websocket-Requests
		http::websocket::server::request_handler websocket_handler_;

		/// \brief Handler for normal HTTP-File-Requests
		http::server::file_request_handler http_file_handler_;
	};


	struct parameter{
		std::size_t port;
		std::string websocket_name;
		std::string http_root_path;
		std::string ready_signal;
	};


	template < typename Target, typename BaseClass >
	Target& as_sub_type(BaseClass* object){
		auto ptr = dynamic_cast< Target* >(object);
		if(ptr == nullptr){
			throw std::logic_error("as_sub_type failed");
		}
		return *ptr;
	}


	struct module: disposer::module_base{
		module(disposer::make_data const& mdata, parameter&& param):
			disposer::module_base(mdata, {data}),
			handler_(
				param.http_root_path,
				param.websocket_name,
				param.ready_signal
			),
			server_(std::to_string(param.port), handler_, 1)
			{}


		disposer::input< std::string > data{"data"};


		void exec()override;


		request_handler handler_;
		http::server::server server_;
	};


	disposer::module_ptr make_module(disposer::make_data& data){
		if(data.is_first()) throw disposer::module_not_as_start(data);

		if(data.inputs.find("data") == data.inputs.end()){
			throw std::logic_error("no input defined (use 'data')");
		}

		auto& params = data.params;
		auto param = parameter();

		params.set(param.port, "port", 8080);
		params.set(param.websocket_name, "websocket_name");
		params.set(param.http_root_path, "http_root_path", "www");
		params.set(param.ready_signal, "ready_signal", "ready");

		return std::make_unique< module >(data, std::move(param));
	}


	void module::exec(){
		auto list = data.get();
		if(list.empty()) return;
		auto iter = list.end();
		--iter;
		handler_.send(iter->second.data());
	}


	void init(disposer::module_declarant& add){
		add("websocket_live_stream", &make_module);
	}

	BOOST_DLL_AUTO_ALIAS(init)


} }
