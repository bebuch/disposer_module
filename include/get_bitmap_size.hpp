//-----------------------------------------------------------------------------
// Copyright (c) 2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _disposer_module__get_bitmap_size__hpp_INCLUDED_
#define _disposer_module__get_bitmap_size__hpp_INCLUDED_

#include "bitmap_vector.hpp"

#include <numeric>


namespace disposer_module{


	/// \brief Get size of the bitmaps, throw if different sizes
	template < typename Bitmap, typename GetSizeFunction >
	::bitmap::size< std::size_t > get_bitmap_size(
		std::vector< Bitmap > const& vec,
		GetSizeFunction&& f
	){
		if(vec.empty()) throw std::logic_error("bitmap vector is empty");
		return std::accumulate(
			vec.cbegin() + 1, vec.cend(), f(*vec.cbegin()),
			[&vec, &f](auto& ref, auto& test){
				if(ref == f(test)) return ref;

				std::ostringstream os;
				os << "different image sizes (";
				bool first = true;
				for(auto& img: vec){
					if(first){ first = false; }else{ os << ", "; }
					os << f(img);
				}
				os << ") image";
				throw std::logic_error(os.str());
			});
	}

	template < typename T >
	::bitmap::size< std::size_t > get_bitmap_size(
		bitmap_vector< T > const& vec
	){
		return get_bitmap_size(vec,
			[](auto const& bitmap){ return bitmap.size(); });
	}

	template < typename T >
	::bitmap::size< std::size_t > get_bitmap_size(
		std::vector< std::reference_wrapper< bitmap< T > > > const& vec
	){
		return get_bitmap_size(vec,
			[](auto const& bitmap){ return bitmap.get().size(); });
	}

	template < typename T >
	::bitmap::size< std::size_t > get_bitmap_size(
		std::vector< std::reference_wrapper< bitmap< T > const > > const& vec
	){
		return get_bitmap_size(vec,
			[](auto const& bitmap){ return bitmap.get().size(); });
	}


}

#endif
