//-----------------------------------------------------------------------------
// Copyright (c) 2015 Benjamin Buch
//
// https://github.com/bebuch/disposer
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _disposer_module_async_get_hpp_INCLUDED_
#define _disposer_module_async_get_hpp_INCLUDED_

#include "bitmap_sequence.hpp"

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

		disposer::module_input< std::future< T > > future{"future"};

		disposer::module_output< T > result{"result"};

		void trigger(std::size_t id)override{
			for(auto& pair: future.get(id)){
				auto id = pair.first;
				result(id, pair.second.get());
			}
		}
	};

	template <>
	struct module< void >: disposer::module_base{
		module(std::string const& type, std::string const& chain, std::string const& name):
			disposer::module_base(type, chain, name){
				inputs = disposer::make_input_list(future);
			}

		disposer::module_input< std::future< void > > future{"future"};

		void trigger(std::size_t id)override{
			for(auto& pair: future.get(id)){
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
		add_module_maker("async_get_float_sequence", &make_module< std::shared_ptr< bitmap_const_pointer_sequence< float > const > >);
	}


} }

#endif
