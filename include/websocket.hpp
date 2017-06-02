//-----------------------------------------------------------------------------
// Copyright (c) 2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _disposer_module__websocket__hpp_INCLUDED_
#define _disposer_module__websocket__hpp_INCLUDED_

#include <string>


namespace disposer{
	class module_declarant;
}


namespace disposer_module{
	class http_server;
}


namespace disposer_module::websocket{


	void register_module(
		std::string const& name,
		disposer::module_declarant& disposer,
		disposer_module::http_server& server);


}


#endif
