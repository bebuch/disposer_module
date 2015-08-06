//-----------------------------------------------------------------------------
// Copyright (c) 2015 Benjamin Buch
//
// https://github.com/bebuch/disposer
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _disposer_module_bitmap_sequence_hpp_INCLUDED_
#define _disposer_module_bitmap_sequence_hpp_INCLUDED_

#include "bitmap.hpp"

#include <vector>
#include <memory>


namespace disposer_module{


	/// \brief Vector of disposer_module::bitmap< T >
	template < typename T >
	using bitmap_sequence = std::vector< bitmap< T > >;


}

#endif
