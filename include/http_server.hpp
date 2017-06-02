//-----------------------------------------------------------------------------
// Copyright (c) 2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _disposer_module__http_server__hpp_INCLUDED_
#define _disposer_module__http_server__hpp_INCLUDED_

#include <memory>
#include <string>


namespace disposer{
	class disposer;
}

namespace disposer_module{


	class request_handler;

	class websocket_identifier{
	private:
		websocket_identifier(std::string const& name)
			: name(name) {}

		std::string const& name;

		friend class request_handler;
	};

	struct http_server_init_t{
		websocket_identifier key;
		bool success;
	};

	struct http_server_impl;

	class http_server{
	public:
		http_server(
			disposer::disposer& disposer,
			std::string const& root,
			std::size_t port);

		http_server(http_server&&);

		~http_server();

		http_server_init_t init(std::string const& service_name);

		websocket_identifier unique_init(std::string const& service_name);

		void uninit(websocket_identifier key);

		void send(websocket_identifier key, std::string const& data);


	private:
		std::unique_ptr< http_server_impl > impl_;
	};


}


#endif
