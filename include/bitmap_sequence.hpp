//-----------------------------------------------------------------------------
// Copyright (c) 2015 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _disposer_module__bitmap_sequence_hpp_INCLUDED_
#define _disposer_module__bitmap_sequence_hpp_INCLUDED_

#include "bitmap_vector.hpp"


namespace disposer_module{


	/// \brief List of disposer_module::bitmap_vector< T >
	template < typename T >
	using bitmap_sequence = std::vector< bitmap_vector< T > >;


}

#endif
