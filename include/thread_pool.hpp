//-----------------------------------------------------------------------------
// Copyright (c) 2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _disposer_module__thread_pool__hpp_INCLUDED_
#define _disposer_module__thread_pool__hpp_INCLUDED_

#include <thread>
#include <atomic>
#include <algorithm>


namespace disposer_module{


	class thread_pool{
	public:
		thread_pool(
			std::size_t thread_count = std::thread::hardware_concurrency()
		):
			cores_(thread_count) {}

		template < typename F >
		void operator()(
			std::size_t first_index,
			std::size_t last_index,
			F&& function
		){
			auto const count = std::min(cores_, last_index - first_index);

			std::vector< std::thread > workers;
			workers.reserve(count);

			std::atomic< std::size_t > index(first_index);
			for(std::size_t i = 0; i < count; ++i){
				workers.emplace_back([&index, &function, last_index]{
					for(std::size_t i = index++; i < last_index; i = index++){
						function(i);
					}
				});
			}

			for(auto& worker: workers){
				worker.join();
			}
		}


	private:
		std::size_t cores_;
	};


}


#endif
