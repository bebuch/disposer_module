//-----------------------------------------------------------------------------
// Copyright (c) 2012-2015 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _disposer_module_big_undef_conversion_hpp_INCLUDED_
#define _disposer_module_big_undef_conversion_hpp_INCLUDED_

#include <cmath>
#include <limits>
#include <algorithm>
#include <utility>


namespace disposer_module{ namespace big{


	template < typename T >
	constexpr bool isnan(T const& value){
		return value != value;
	}

	constexpr float undef = 3.402823466e38f;

	template < typename Container >
	Container convert_undef_to_nan(Container const& image){
		typedef typename Container::value_type value_type;

		Container result(image.size());
		std::transform(image.begin(), image.end(), result.begin(), [](value_type value){
			return value >= undef ? std::numeric_limits< value_type >::quiet_NaN() : value;
		});

		return std::move(result);
	}

	template < typename Container >
	Container convert_nan_to_undef(Container const& image){
		typedef typename Container::value_type value_type;

		Container result(image.size());
		std::transform(image.begin(), image.end(), result.begin(), [](value_type value){
			return isnan(value) ? undef : value;
		});

		return std::move(result);
	}


} }

#endif
