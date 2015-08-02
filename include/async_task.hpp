#ifndef _disposer_module_async_task_hpp_INCLUDED_
#define _disposer_module_async_task_hpp_INCLUDED_

#include <disposer/module_base.hpp>

#include <future>


namespace disposer_module{ namespace async_task{


	template < typename T >
	struct module: disposer::module_base{
		module(std::string const& type, std::string const& chain, std::string const& name):
			disposer::module_base(type, chain, name){
				outputs = disposer::make_output_list(future);
			}

		disposer::module_output< std::future< T > > shared_future{"future"};
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
