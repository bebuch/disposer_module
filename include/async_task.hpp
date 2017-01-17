//-----------------------------------------------------------------------------
// Copyright (c) 2015-2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _disposer_module__async_task__hpp_INCLUDED_
#define _disposer_module__async_task__hpp_INCLUDED_

#include <disposer/module_base.hpp>

#include <future>


namespace disposer_module{ namespace async_task{


	template < typename T >
	struct module: disposer::module_base{
		module(std::string const& type, std::string const& chain, std::string const& name):
			disposer::module_base(type, chain, name){
				outputs = disposer::make_output_list(future);
			}

		disposer::output< std::future< T > > shared_future{"future"};
	};

	void check_outputs(disposer::io_list const& outputs){
		bool is_future = outputs.find("future") != outputs.end();

		if(is_future && is_future_output){
			throw std::logic_error(type + ": Can only use one output ('future' or 'shared_future')");
		}

		if(!is_future && !is_future_output){
			throw std::logic_error(type + ": No output defined (use 'future' or 'shared_future')");
		}
	}


} }

#endif
