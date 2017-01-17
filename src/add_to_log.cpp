//-----------------------------------------------------------------------------
// Copyright (c) 2015-2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "log.hpp"

#include <disposer/module.hpp>

#include <boost/dll.hpp>



namespace disposer_module{ namespace add_to_log{


	struct module: disposer::module_base{
		module(disposer::make_data const& data):
			disposer::module_base(data, {string}){}

		disposer::input< std::string > string{"string"};

		virtual void trigger()override{
			for(auto& pair: string.get()){
				auto id = pair.first;
				auto& data = pair.second.data();

				disposer::log([this, id, &data](log::info& os){
					os << type << ": id=" << id << " chain '" << chain
						<< "' module '" << name << "' data='"
						<< data << "'";
				});
			}
		}
	};

	disposer::module_ptr make_module(disposer::make_data& data){
		if(data.is_first()) throw disposer::module_not_as_start(data);

		return std::make_unique< module >(data);
	}

	void init(disposer::module_adder& add){
		add("add_to_log", &make_module);
	}

	BOOST_DLL_AUTO_ALIAS(init)


} }
