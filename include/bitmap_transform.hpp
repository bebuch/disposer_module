//-----------------------------------------------------------------------------
// Copyright (c) 2013-2015 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _disposer_module__bitmap_transform_hpp_INCLUDED_
#define _disposer_module__bitmap_transform_hpp_INCLUDED_

#include "bitmap.hpp"

namespace disposer_module{


	struct simple_bitmap_transform{
		enum value{
			rigth_rotate      = 0x01,
			mirror_horizontal = 0x02,
			mirror_vectical   = 0x04
		};
	};

	/// \brief Use a \ref simple_bitmap_transform on a bitmap, the rotation will be first executet
	template < typename value_type_parameter, typename value_type_result >
	inline bitmap< value_type_result > bitmap_transform(
		bitmap< value_type_parameter > const& image,
		simple_bitmap_transform::value transform,
		std::function< value_type_result(value_type_parameter) > converter
	){
		bool rotation = (transform & simple_bitmap_transform::rigth_rotate) != 0;
		bool mirror_h = (transform & simple_bitmap_transform::mirror_horizontal) != 0;
		bool mirror_v = (transform & simple_bitmap_transform::mirror_vectical) != 0;

		bitmap< value_type_result > result(
			rotation ?
			typename bitmap< value_type_result >::size_type(image.height(), image.width()) :
			image.size()
		);

		for(std::size_t y = 0; y < result.height(); ++y){
			for(std::size_t x = 0; x < result.width(); ++x){
				typename bitmap< value_type_result >::point_type target(x, y);

				if(rotation){
					target.set(target.y(), image.height() - target.x() - 1);
				}

				if(rotation ? mirror_v : mirror_h){
					target.x() = image.width() - target.x() - 1;
				}

				if(rotation ? mirror_h : mirror_v){
					target.y() = image.height() - target.y() - 1;
				}

				result(x, y) = converter(image(target));
			}
		}

		return result;
	}


}

#endif
