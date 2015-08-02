//-----------------------------------------------------------------------------
// Copyright (c) 2015 Benjamin Buch
//
// https://github.com/bebuch/disposer
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _disposer_module_camera_sequence_hpp_INCLUDED_
#define _disposer_module_camera_sequence_hpp_INCLUDED_

#include "bitmap_sequence.hpp"


namespace disposer_module{


	/// \brief List of disposer_module::bitmap_sequence< T >
	template < typename T >
	using camera_sequence = std::vector< bitmap_sequence< T > >;

	/// \brief List of disposer_module::bitmap_pointer_sequence< T >
	template < typename T >
	using camera_pointer_sequence = std::vector< std::shared_ptr< bitmap_pointer_sequence< T > > >;

	/// \brief List of disposer_module::bitmap_pointer_sequence< T >
	template < typename T >
	using camera_const_pointer_sequence = std::vector< std::shared_ptr< bitmap_const_pointer_sequence< T > const > >;

	template < typename T >
	camera_const_pointer_sequence< T > make_const(camera_pointer_sequence< T >&& data){
		camera_const_pointer_sequence< T > result;
		result.reserve(data.size());

		for(auto& v: data){
			result.push_back(make_const(std::move(v)));
		}

		return result;
	}

	template < typename T >
	std::shared_ptr< camera_const_pointer_sequence< T >  const > make_const(std::shared_ptr< camera_pointer_sequence< T > >&& data){
		return std::make_shared< camera_const_pointer_sequence< T >  const >(make_const(std::move(*data)));
	}

}

#endif
