//-----------------------------------------------------------------------------
// Copyright (c) 2015-2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <logsys/stdlogb.hpp>

#include <disposer/disposer.hpp>
#include <disposer/module_base.hpp>

#include <boost/filesystem.hpp>
#include <boost/dll.hpp>

#include <regex>
#include <iostream>
#include <future>


int main(int argc, char** argv){
	namespace fs = boost::filesystem;

	bool exec_all = (argc == 1);
	std::string exec_chain;
	std::size_t exec_count = 0;
	if(!exec_all){
		if(argc != 3){
			std::cerr << argv[0] << " [chain exec_count]" << std::endl;
			return 1;
		}

		exec_chain = argv[1];
		try{
			exec_count = boost::lexical_cast< std::size_t >(argv[2]);
		}catch(...){
			std::cerr << argv[0] << " [chain exec_count]" << std::endl;
			std::cerr << "exec_count parsing failed: " << argv[2] << std::endl;
			return 1;
		}
	}

	// modules must be deleted last, to access the destructors in shared libs
	std::list< boost::dll::shared_library > modules;
	::disposer::disposer disposer;

	if(!logsys::exception_catching_log([](
		logsys::stdlogb& os){ os << "loading modules"; },
	[&disposer, &modules]{
		auto program_dir = boost::dll::program_location().remove_filename();
		std::cout << "Search for DLLs in '" << program_dir << "'" << std::endl;

		std::regex regex(".*\\.so");
		for(auto const& file: fs::directory_iterator(program_dir)){
			if(
				!is_regular_file(file) ||
				!std::regex_match(file.path().filename().string(), regex)
			) continue;

			logsys::log([&file](logsys::stdlogb& os){
				os << "load shared library '" << file.path().string() << "'";
			}, [&]{
				modules.emplace_back(file.path().string());

				modules.back().get_alias<
					void(::disposer::module_declarant&)
				>("init")(disposer.declarant());
			});
		}
	})) return 1;

	return !logsys::exception_catching_log(
		[](logsys::stdlogb& os){ os << "exec chains"; },
	[&disposer, exec_all, &exec_chain, exec_count]{
		disposer.load("plan.ini");

		if(exec_all){
			for(auto& chain_name: disposer.chains()){
				auto& chain = disposer.get_chain(chain_name);
				chain.enable();
				chain.exec();
				chain.disable();
			}
		}else{
			auto& chain = disposer.get_chain(exec_chain);

			// single thread version
			chain.enable();
			for(std::size_t i = 0; i < exec_count; ++i){
				chain.exec();
			}
			chain.disable();


// 			// multi threaded version
// 			auto const cores = std::thread::hardware_concurrency();
// 			std::vector< std::thread > workers;
// 			workers.reserve(cores);
//
// 			chain.enable();
// 			std::atomic< std::size_t > index(0);
// 			for(std::size_t i = 0; i < cores; ++i){
// 				workers.emplace_back([&chain, &index]{
// 					while(index++ < 1000){
// 						chain.exec();
// 					}
// 				});
// 			}
//
// 			for(auto& worker: workers){
// 				worker.join();
// 			}
// 			chain.disable();
		}
	});
}
