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

	// modules must be deleted last, to access the destructors in shared libs
	std::list< boost::dll::shared_library > modules;
	::disposer::disposer disposer;

	if(!::disposer::exception_catching_log([](
		disposer_module::log::info& os){ os << "loading modules"; },
	[&disposer, &modules]{
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
					void(::disposer::module_adder&)
				>("init")(disposer.adder());
			});
		}
	})) return 1;

	return ::disposer::exception_catching_log(
		[](disposer_module::log::info& os){ os << "trigger chains"; },
	[&disposer]{
		disposer.load("plan.ini");

		for(auto& chain: disposer.chains()){
			disposer.trigger(chain);
		}
	});
}
