//-----------------------------------------------------------------------------
// Copyright (c) 2015-2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <disposer/disposer.hpp>

#include <logsys/log.hpp>
#include <logsys/stdlogb.hpp>

#include <cxxopts.hpp>

#include <boost/filesystem.hpp>
#include <boost/stacktrace.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/dll.hpp>

#include <regex>
#include <iostream>
#include <future>
#include <csignal>


void signal_handler(int signum){
	std::signal(signum, SIG_DFL);
	std::cerr << boost::stacktrace::stacktrace();
	std::raise(SIGABRT);
}


int main(int argc, char** argv){
	using namespace std::literals::string_view_literals;
	namespace fs = boost::filesystem;

	// Set signal handler
	std::signal(SIGSEGV, signal_handler);
	std::signal(SIGABRT, signal_handler);

	cxxopts::Options options(argv[0], "disposer module system");

	options.add_options()
		("c,config", "Configuration file", cxxopts::value< std::string >(),
			"config.ini")
		("l,log", "Your log file",
			cxxopts::value< std::string >()->default_value("disposer.log"),
			"disposer.log")
		("s,server", "Run until the enter key is pressed")
		("m,multithreading",
			"All N executions of a chain are stated instantly")
		("chain", "Execute a chain", cxxopts::value< std::string >(), "Name")
		("n,count", "Count of chain executions",
			cxxopts::value< std::size_t >()->default_value("1"), "Count");

	try{
		options.parse(argc, argv);
		cxxopts::check_required(options, {"config"});
		bool const server = options["server"].count() > 0;
		if(server && options["multithreading"].count() > 0){
			throw std::logic_error("Option ‘multithreading‘ is not compatible "
				"with option ‘server‘");
		}
		bool const chain = options["chain"].count() > 0;
		if(server && chain){
			throw std::logic_error("Option ‘chain‘ is not compatible "
				"with option ‘server‘");
		}
		if(server && options["count"].count() > 0){
			throw std::logic_error("Option ‘count‘ is not compatible "
				"with option ‘server‘");
		}
		if(!server && !chain){
			throw std::logic_error("Need option ‘server‘ option ‘chain‘");
		}
	}catch(std::exception const& e){
		std::cerr << e.what() << "\n\n";
		std::cout << options.help();
		return -1;
	}

	// modules must be deleted last, to access the destructors in shared libs
	std::list< boost::dll::shared_library > libraries;
	::disposer::disposer disposer;

	if(!logsys::exception_catching_log([](
		logsys::stdlogb& os){ os << "loading modules"; },
	[&disposer, &libraries]{
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
				auto& library = libraries.emplace_back(file.path().string(),
					boost::dll::load_mode::rtld_deepbind);

				if(library.has("init_component")){
					library.get_alias<
							void(
								std::string const&,
								::disposer::component_declarant&
							)
						>("init_component")(lib_name,
							disposer.component_declarant());
				}else if(library.has("init")){
					library.get_alias<
							void(
								std::string const&,
								::disposer::module_declarant&
							)
						>("init")(lib_name, disposer.module_declarant());
				}else{
					logsys::log([&lib_name](logsys::stdlogb& os){
						os << "shared library '" << lib_name
							<< "' is nighter a component nor a module";
					});
				}
			});
		}
	})) return 1;

	std::string const config = options["config"].as< std::string >();

	if(!logsys::exception_catching_log(
		[](logsys::stdlogb& os){ os << "load config file"; },
		[&disposer, &config]{ disposer.load(config); })) return -1;

	if(options["server"].count() > 0){
		std::cout << "Hit Enter to exit!" << std::endl;
		std::cin.get();
		return 0;
	}

	return !logsys::exception_catching_log(
		[](logsys::stdlogb& os){ os << "exec chains"; },
	[&disposer, &options]{
		auto const multithreading = options["multithreading"].count() > 0;
		auto const exec_chain = options["chain"].as< std::string >();
		auto const exec_count = options["count"].as< std::size_t >();

		auto& chain = disposer.get_chain(exec_chain);

		if(!multithreading){
			// single thread version
			::disposer::chain_enable_guard enable(chain);
			for(std::size_t i = 0; i < exec_count; ++i){
				chain.exec();
			}
		}else{
			// multi threaded version
			std::vector< std::future< void > > tasks;
			tasks.reserve(exec_count);

			::disposer::chain_enable_guard enable(chain);
			for(std::size_t i = 0; i < exec_count; ++i){
				tasks.push_back(std::async([&chain]{ chain.exec(); }));
			}

			for(auto& task: tasks){
				task.get();
			}
		}
	});
}
