//-----------------------------------------------------------------------------
// Copyright (c) 2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "websocket.hpp"
#include "http_server.hpp"

#include <disposer/module.hpp>


namespace disposer_module::websocket{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;


	void register_module(
		std::string const& name,
		module_declarant& disposer,
		disposer_module::http_server& server
	){
		auto init = make_module_register_fn(
			module_configure(
				"data"_in(hana::type_c< std::string >, required),
				"service_name"_param(hana::type_c< std::string >)
			),
			normal_id_increase(),
			[&server](auto const& module){
				auto init = server.init(module("service_name"_param).get());
				auto key = init.key;
				return [&server, key](auto& module, std::size_t /*id*/){
					auto list = module("data"_in).get_references();
					if(list.empty()) return;
					auto iter = list.end();
					--iter;
					server.send(key, iter->second.get());
				};
			}
		);

		init(name, disposer);
	}


}
