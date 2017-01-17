//-----------------------------------------------------------------------------
// Copyright (c) 2015-2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _disposer_module__async_get__hpp_INCLUDED_
#define _disposer_module__async_get__hpp_INCLUDED_

#include "bitmap_vector.hpp"

#include <disposer/module_base.hpp>

#include <future>
#include <iostream>


namespace disposer_module{ namespace async_get{


	template < typename T >
	struct module: disposer::module_base{
		module(std::string const& type, std::string const& chain, std::string const& name):
			disposer::module_base(type, chain, name){
				inputs = disposer::make_input_list(future);
				outputs = disposer::make_output_list(result);
			}

		disposer::input< std::future< T > > future{"future"};

		disposer::output< T > result{"result"};

		void trigger()override{
			for(auto& pair: future.get()){
// 				auto id = pair.first;
				result.put(pair.second.get());
			}
		}
	};

	template <>
	struct module< void >: disposer::module_base{
		module(std::string const& type, std::string const& chain, std::string const& name):
			disposer::module_base(type, chain, name){
				inputs = disposer::make_input_list(future);
			}

		disposer::input< std::future< void > > future{"future"};

		void trigger()override{
			for(auto& pair: future.get()){
				pair.second.get();
			}
		}
	};

	template < typename T >
	disposer::module_ptr make_module(
		std::string const& type,
		std::string const& chain,
		std::string const& name,
		disposer::io_list const& inputs,
		disposer::io_list const& outputs,
		disposer::parameter_processor&,
		bool is_start
	){
		if(is_start) throw disposer::module_not_as_start(type, chain);

		if(inputs.find("future") == inputs.end()){
			throw std::logic_error(type + ": No input defined (use 'future')");
		}

		if(!std::is_void< T >::value && outputs.find("result") == outputs.end()){
			throw std::logic_error(type + ": No output defined (use 'result')");
		}

		return std::make_unique< module< T > >(type, chain, name);
	}

	inline void init(){
		add_module_maker("async_get", &make_module< void >);
		add_module_maker("async_get_string", &make_module< std::string >);
		add_module_maker("async_get_float_sequence", &make_module< bitmap_vector< float > >);
	}


} }

#endif
