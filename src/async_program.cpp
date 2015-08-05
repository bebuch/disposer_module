//-----------------------------------------------------------------------------
// Copyright (c) 2015 Benjamin Buch
//
// https://github.com/bebuch/disposer
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "async_program.hpp"

#include <disposer/module_base.hpp>

#include <boost/process.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>

#include <future>


namespace disposer_module{ namespace async_program{


	struct parameter{
		bool id_as_parameter;
		std::size_t id_position;

		bool output;

		std::string command;
		std::string start_in_dir;
		std::vector< std::string > parameters;
	};

	struct module: disposer::module_base{
		module(std::string const& type, std::string const& chain, std::string const& name, parameter&& param):
			disposer::module_base(type, chain, name),
			param(std::move(param)){
				outputs = disposer::make_output_list(future, future_output);
			}

		disposer::output< std::future< void > > future{"future"};
		disposer::output< std::future< std::string > > future_output{"future_output"};

		void trigger(std::size_t id)override{
			using namespace boost::process;
			using namespace boost::process::initializers;
			using namespace boost::iostreams;

			if(!param.output){
				future.put(id, std::async(std::launch::async, [this, id]{
#if defined(BOOST_WINDOWS_API)
					file_descriptor_sink sink("nul");
#elif defined(BOOST_POSIX_API)
					file_descriptor_sink sink("/dev/null");
#endif
					if(param.start_in_dir.empty()){
						auto c = execute(
							set_args(get_parameters(id)),
							bind_stdout(sink),
							bind_stderr(sink),
							inherit_env(),
							throw_on_error()
						);
						wait_for_exit(c);
					}else{
						auto c = execute(
							set_args(get_parameters(id)),
							start_in_dir(param.start_in_dir),
							bind_stdout(sink),
							bind_stderr(sink),
							inherit_env(),
							throw_on_error()
						);
						wait_for_exit(c);
					}
				}));
			}else{
				future_output.put(id, std::async(std::launch::async, [this, id]{
					auto pipe = create_pipe();
					{
						file_descriptor_sink sink(pipe.sink, close_handle);
						if(param.start_in_dir.empty()){
							auto c = execute(
								set_args(get_parameters(id)),
								bind_stdout(sink),
								bind_stderr(sink),
								inherit_env(),
								throw_on_error()
							);
							wait_for_exit(c);
						}else{
							auto c = execute(
								set_args(get_parameters(id)),
								start_in_dir(param.start_in_dir),
								bind_stdout(sink),
								bind_stderr(sink),
								inherit_env(),
								throw_on_error()
							);
							wait_for_exit(c);
						}
					}

					file_descriptor_source source(pipe.source, close_handle);

					stream< file_descriptor_source > is(source);
					return std::string{std::istream_iterator< char >(is),  std::istream_iterator< char >()};
				}));
			}
		}

		std::vector< std::string > get_parameters(std::size_t id){
			std::vector< std::string > parameters;
			parameters.reserve(param.parameters.size() + (param.id_as_parameter ? 2 : 1));

			parameters.push_back(command());

			std::size_t i = 0;
			for(auto const& value: param.parameters){
				if(param.id_as_parameter && ++i == param.id_position){
					parameters.push_back(std::to_string(id));
				}
				parameters.push_back(value);
			}

			if(param.id_as_parameter && parameters.size() < param.parameters.size() + 2){
				parameters.push_back(std::to_string(id));
			}

			return parameters;
		}

		std::string command(){
			using namespace boost::filesystem;
			auto command = absolute(param.command).string();
			if(command.empty() || !exists(command) || is_directory(command)) command = boost::process::search_path(param.command);
			if(command.empty()) throw std::runtime_error("Executable file '" + param.command + "' not found");
			return command;
		}

		parameter const param;
	};

	disposer::module_ptr make_module(
		std::string const& type,
		std::string const& chain,
		std::string const& name,
		disposer::io_list const&,
		disposer::io_list const& outputs,
		disposer::parameter_processor& params,
		bool is_start
	){
		bool is_future = outputs.find("future") != outputs.end();
		bool is_future_output = outputs.find("future_output") != outputs.end();

		if(is_future && is_future_output){
			throw std::logic_error(type + ": Can only use one output ('future' or 'future_output')");
		}

		if(!is_future && !is_future_output){
			throw std::logic_error(type + ": No output defined (use 'future' or 'future_output')");
		}

		parameter param;

		param.output = is_future_output;

		params.set(param.id_as_parameter, "id_as_parameter", true);
		params.set(param.id_position, "id_position", 1);

		if(param.id_position == 0){
			throw std::runtime_error(type + ": parameter id_position must not be 0 (0 is the program name!)");
		}

		params.set(param.command, "command");
		params.set(param.start_in_dir, "start_in_dir", "");

		std::string parameters;
		params.set(parameters, "parameters", "");

		if(!parameters.empty()){
			std::string arg;
			bool skip = true;
			bool backslash = false;
			bool quoted = false;
			
			for(auto c: parameters){
				if(skip){
					if(c == ' ' || c == '\t') continue;
					skip = false;
					if(c == '"'){
						quoted = true;
					}else{
						arg.push_back(c);
					}
					continue;
				}

				if(quoted){
					if(backslash){
						backslash = false;
						arg.push_back(c);
						continue;
					}

					if(c == '\\'){
						backslash = true;
						continue;
					}

					if(c == '"'){
						if(!arg.empty()){
							param.parameters.push_back(arg);
							arg = "";
						}
						skip = true;
						quoted = false;
						continue;
					}

					arg.push_back(c);
					continue;
				}

				if(c == ' ' || c == '\t'){
					skip = true;
					if(!arg.empty()){
						param.parameters.push_back(arg);
						arg = "";
					}
					continue;
				}

				arg.push_back(c);
			}

			if(!skip){
				if(!arg.empty()){
					param.parameters.push_back(arg);
					arg = "";
				}
			}
		}

		return std::make_unique< module >(type, chain, name, std::move(param));
	}

	void init(){
		add_module_maker("async_program", &make_module);
	}


} }
