//-----------------------------------------------------------------------------
// Copyright (c) 2009-2015 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _disposer_module__big_types__hpp_INCLUDED_
#define _disposer_module__big_types__hpp_INCLUDED_

#include "bitmap.hpp"

#include <type_traits>


namespace disposer_module{ namespace big{


	/// \brief It's a meta function, that get the numeric value for the type
	///        in a big file
	template < typename T > struct type {
		/// \brief Type in big files
		static constexpr unsigned short value =
			sizeof(T) | (std::is_signed< T >::value ? 0x20 : 0x00);
	};

	/// \brief The specialization for float big files
	template <> struct type< float >{
		/// \brief Type in big files
		static constexpr unsigned short value = sizeof(float) | 0x10;
	};

	/// \brief The specialization for double big files
	template <> struct type< double >{
		/// \brief Type in big files
		static constexpr unsigned short value = sizeof(double) | 0x10;
	};

	/// \brief The specialization for long double big files
	template <> struct type< long double >{
		/// \brief Type in big files
		static constexpr unsigned short value = sizeof(long double) | 0x10;
	};

	template < typename T >
	constexpr unsigned short type_v = type< T >::value;


} }

#endif
