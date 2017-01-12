//-----------------------------------------------------------------------------
// Copyright (c) 2009-2015 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _disposer_module__big_exception__hpp_INCLUDED_
#define _disposer_module__big_exception__hpp_INCLUDED_

#include <stdexcept>

namespace disposer_module{ namespace big{


	/// \brief Exception class that will throws by load or save big files
        struct big_error: std::runtime_error{
            using std::runtime_error::runtime_error;
	};


} }

#endif
