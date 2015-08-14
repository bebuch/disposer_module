//-----------------------------------------------------------------------------
// Copyright (c) 2015 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "init.hpp"

#include "name_generator.hpp"

#include <disposer/config.hpp>

#include <boost/type_index.hpp>

#include <iostream>


int main(){
	try{
		disposer_module::init();

		auto chains = disposer::config::load("plan.ini");

		for(auto& chain: chains){
			chain.second.trigger();
		}
	}catch(std::exception const& e){
		std::cout << "Exception: [" << boost::typeindex::type_id_runtime(e).pretty_name() << "] " << e.what() << std::endl;
	}catch(...){
		std::cout << "Unknown exception" << std::endl;
	}
}
