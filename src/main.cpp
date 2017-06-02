//-----------------------------------------------------------------------------
// Copyright (c) 2015-2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "http_server.hpp"
#include "websocket.hpp"

#include <logsys/stdlogb.hpp>

#include <disposer/disposer.hpp>
#include <disposer/module_base.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/dll.hpp>

#include <regex>
#include <iostream>
#include <future>


int main(int argc, char** argv){
	using namespace std::literals::string_view_literals;
	namespace fs = boost::filesystem;

	bool const server_mode = argc == 2 ? argv[1] == "--server"sv : false;
	bool multithreading = argc > 1 ? argv[1] == "--multithreading"sv : false;
	bool exec_all = (argc == (multithreading ? 2 : 1));
	std::string exec_chain;
	std::size_t exec_count = 0;
	if(!server_mode && !exec_all){
		if(argc != (multithreading ? 4 : 3)){
			std::cerr << argv[0] << " [--multithreading] [chain exec_count]"
				<< std::endl;
			return 1;
		}

		exec_chain = argv[multithreading ? 2 : 1];
		try{
			exec_count = boost::lexical_cast< std::size_t >(
				argv[multithreading ? 3 : 2]);
		}catch(...){
			std::cerr << argv[0] << " [chain exec_count]" << std::endl;
			std::cerr << "exec_count parsing failed: "
				<< argv[multithreading ? 3 : 2] << std::endl;
			return 1;
		}
	}

	// modules must be deleted last, to access the destructors in shared libs
	std::list< boost::dll::shared_library > modules;
	::disposer::disposer disposer;

	auto server = logsys::log(
		[](logsys::stdlogb& os){ os << "create http server"; },
		[&disposer]{ return disposer_module::http_server(disposer,
			"/home/bebuch/media/projekte/disposer_module/test/html/", 8000);
		});

	if(!logsys::exception_catching_log([](
		logsys::stdlogb& os){ os << "loading modules"; },
	[&disposer, &modules, &server]{
		disposer_module::websocket::register_module(
			"websocket", disposer.declarant(), server);

		auto program_dir = boost::dll::program_location().remove_filename();
		std::cout << "Search for DLLs in '" << program_dir << "'" << std::endl;

		std::regex regex("lib.*\\.so");
		for(auto const& file: fs::directory_iterator(program_dir)){
			if(
				!is_regular_file(file) ||
				!std::regex_match(file.path().filename().string(), regex)
			) continue;

			auto const lib_name = file.path().stem().string().substr(3);

			logsys::log([&lib_name](logsys::stdlogb& os){
				os << "load shared library '" << lib_name << "'";
			}, [&]{
				modules.emplace_back(file.path().string(),
					boost::dll::load_mode::rtld_deepbind);

				if(modules.back().has("init")){
					modules.back().get_alias<
							void(
								std::string const&,
								::disposer::module_declarant&
							)
						>("init")(lib_name, disposer.declarant());
				}else{
					logsys::log([&lib_name](logsys::stdlogb& os){
						os << "shared library '" << lib_name
							<< "' is not a module";
					});
				}
			});
		}
	})) return 1;

	return !logsys::exception_catching_log(
		[](logsys::stdlogb& os){ os << "exec chains"; },
	[&disposer, server_mode, exec_all, &exec_chain, exec_count, multithreading]{
		disposer.load("plan.ini");

		if(server_mode){
			std::cout << "Hit Enter to exit!" << std::endl;
			std::cin.get();
		}else if(exec_all){
			for(auto& chain_name: disposer.chains()){
				auto& chain = disposer.get_chain(chain_name);
				chain.enable();
				chain.exec();
				chain.disable();
			}
		}else{
			auto& chain = disposer.get_chain(exec_chain);

			if(!multithreading){
				// single thread version
				chain.enable();
				for(std::size_t i = 0; i < exec_count; ++i){
					chain.exec();
				}
				chain.disable();
			}else{
				// multi threaded version
				auto const cores = std::thread::hardware_concurrency();
				std::vector< std::thread > workers;
				workers.reserve(cores);

				chain.enable();
				std::atomic< std::size_t > index(0);
				for(std::size_t i = 0; i < cores; ++i){
					workers.emplace_back([&chain, &index]{
						while(index++ < 1000){
							chain.exec();
						}
					});
				}

				for(auto& worker: workers){
					worker.join();
				}
				chain.disable();
			}
		}
	});
}
