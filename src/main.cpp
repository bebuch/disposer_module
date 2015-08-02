//-----------------------------------------------------------------------------
// Copyright (c) 2015 Benjamin Buch
//
// https://github.com/bebuch/disposer
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "init.hpp"

#include "name_generator.hpp"

#include <disposer/config.hpp>

#include <iostream>


int main(){
	try{
		disposer_module::init();

		auto chains = disposer::config::load("plan.ini");

		chains.at("serial").trigger();
		chains.at("parallel").trigger();
		chains.at("toTar").trigger();
	}catch(std::exception const& e){
		std::cout << "Exception: " << e.what() << std::endl;
	}catch(...){
		std::cout << "Unknown exception" << std::endl;
	}
}

