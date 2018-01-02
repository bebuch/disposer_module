//-----------------------------------------------------------------------------
// Copyright (c) 2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _disposer_module__log__hpp_INCLUDED_
#define _disposer_module__log__hpp_INCLUDED_

#include <logsys/stdlogd.hpp>


namespace disposer_module{


	struct stdlog: ::logsys::stdlogd{
		static std::weak_ptr< std::ostream > weak_file_ptr;

		void exec()const noexcept override;
	};


}


#endif
