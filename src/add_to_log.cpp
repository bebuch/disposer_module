//-----------------------------------------------------------------------------
// Copyright (c) 2015 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "add_to_log.hpp"

#include "log.hpp"
#include "mask_non_print.hpp"

#include <disposer/io.hpp>

#include <boost/dll.hpp>



namespace disposer_module{ namespace add_to_log{


	struct module: disposer::module_base{
		module(std::string const& type, std::string const& chain, std::string const& name):
			disposer::module_base(type, chain, name){
				inputs = disposer::make_input_list(string);
			}

		disposer::input< std::string > string{"string"};

		void trigger(std::size_t id){
			for(auto& pair: string.get(id)){
				auto id = pair.first;
				auto& data = pair.second.data();

				disposer::log([this, id, &data](log::info& os){
					os << type << ": id=" << id << " chain '" << chain << "' module '" << name << "' data='" << mask_non_print(data) << "'";
				});
			}
		}
	};

	disposer::module_ptr make_module(
		std::string const& type,
		std::string const& chain,
		std::string const& name,
		disposer::io_list const&,
		disposer::io_list const&,
		disposer::parameter_processor&,
		bool is_start
	){
		if(is_start) throw disposer::module_not_as_start(type, chain);

		return std::make_unique< module >(type, chain, name);
	}

	void init(disposer::disposer& disposer){
		disposer.add_module_maker("add_to_log", &make_module);
	}

	BOOST_DLL_AUTO_ALIAS(init)


} }
