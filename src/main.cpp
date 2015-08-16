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

#include <disposer/disposer.hpp>

#include <boost/type_index.hpp>
#include <boost/filesystem.hpp>
#include <boost/dll.hpp>

#include <regex>
#include <iostream>


int main(){
	namespace fs = boost::filesystem;

	try{
		std::list< boost::dll::shared_library > modules;

		disposer::disposer head;

		std::regex regex(".*\\.so");
		for(auto const& file: fs::directory_iterator(boost::dll::program_location().remove_filename())){
			if(!is_regular_file(file)) continue;
			if(!std::regex_match(file.path().filename().string(), regex)) continue;
			modules.emplace_back(file.path().string());
			modules.back().get_alias< void(disposer::disposer&) >("init")(head);
		}

		auto chains = head.load("plan.ini");

		for(auto& chain: chains){
			chain.second.trigger();
		}
	}catch(std::exception const& e){
		std::cout << "Exception: [" << boost::typeindex::type_id_runtime(e).pretty_name() << "] " << e.what() << std::endl;
	}catch(...){
		std::cout << "Unknown exception" << std::endl;
	}
}
