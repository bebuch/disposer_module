//-----------------------------------------------------------------------------
// Copyright (c) 2015 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "log.hpp"

#include <disposer/disposer.hpp>
#include <disposer/module_base.hpp>

#include <boost/filesystem.hpp>
#include <boost/dll.hpp>

#include <regex>
#include <iostream>


int main(){
	namespace fs = boost::filesystem;

	try{
		std::list< boost::dll::shared_library > modules;

		::disposer::disposer disposer;

		auto program_dir = boost::dll::program_location().remove_filename();
		std::cout << "Search for DLLs in '" << program_dir << "'" << std::endl;

		std::regex regex(".*\\.so");
		for(auto const& file: fs::directory_iterator(program_dir)){
			if(
				!is_regular_file(file) ||
				!std::regex_match(file.path().filename().string(), regex)
			) continue;

			::disposer::log([&file](disposer_module::log::info& os){
				os << "load shared library '" << file.path().string() << "'";
			}, [&]{
				modules.emplace_back(file.path().string());

				modules.back().get_alias<
					void(::disposer::disposer&)
				>("init")(disposer);
			});
		}

		disposer.load("plan.ini");

		for(auto& chain: disposer.chains()){
			disposer.trigger(chain);
		}
	}catch(std::exception const& e){
		std::cerr << "Exception: ["
			<< boost::typeindex::type_id_runtime(e).pretty_name() << "] "
			<< e.what() << std::endl;
	}catch(...){
		std::cerr << "Unknown exception" << std::endl;
	}
}
