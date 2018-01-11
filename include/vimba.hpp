//-----------------------------------------------------------------------------
// Copyright (c) 2017-2018 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _disposer_module__vimba__hpp_INCLUDED_
#define _disposer_module__vimba__hpp_INCLUDED_

#include <VimbaCPP/Include/VimbaCPP.h>

#include <mutex>
#include <memory>


namespace disposer_module{


	class vimba_system{
	public:
		static std::shared_ptr< AVT::VmbAPI::VimbaSystem > get();

	private:
		static std::mutex& mutex();
		static std::weak_ptr< AVT::VmbAPI::VimbaSystem >& instance();
	};


	void verify(VmbErrorType state);
	void verify(VmbFrameStatusType state);


}


#endif
