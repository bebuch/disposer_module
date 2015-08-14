//-----------------------------------------------------------------------------
// Copyright (c) 2015 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _disposer_module_log_hpp_INCLUDED_
#define _disposer_module_log_hpp_INCLUDED_

#include "size_io.hpp"
#include "point_io.hpp"
#include "std_array_io.hpp"
#include "std_vector_io.hpp"

#include <disposer/log.hpp>
#include <disposer/log_tag.hpp>

#include <fstream>


namespace disposer_module{


	namespace log{


		struct info: disposer::log_tag_base{};


	}


}


#endif
