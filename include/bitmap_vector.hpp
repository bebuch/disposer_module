//-----------------------------------------------------------------------------
// Copyright (c) 2015-2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _disposer_module__bitmap_vector__hpp_INCLUDED_
#define _disposer_module__bitmap_vector__hpp_INCLUDED_

#include "bitmap.hpp"

#include <vector>
#include <memory>
#include <numeric>


namespace disposer_module{


	/// \brief Vector of disposer_module::bitmap< T >
	template < typename T >
	using bitmap_vector = std::vector< bitmap< T > >;

	/// \brief Get size of the bitmaps, throw if different sizes
	template < typename Bitmap, typename F >
	::bitmap::size< std::size_t > get_size(
		std::vector< Bitmap > const& vec,
		F&& f = [](auto const& bitmap){ return bitmap.size(); }
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
					os << img.get().size();
				}
				os << ") image";
				throw std::logic_error(os.str());
			});
	}


}

#endif
