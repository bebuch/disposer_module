//-----------------------------------------------------------------------------
// Copyright (c) 2009-2015 Benjamin Buch
//
// https://github.com/bebuch/disposer
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _disposer_module_big_types_hpp_INCLUDED_
#define _disposer_module_big_types_hpp_INCLUDED_

#include "bitmap.hpp"

namespace disposer_module{ namespace big{

	/// \brief It's a meta function, that get the numeric value for the type in a big file
	///
	/// By default, the numeric value is the size of the c++ type in byte. You can specialize
	/// the template for another size
	template < typename ValueType > struct type {
		/// \brief Type in big files
		static unsigned short const value = sizeof(ValueType);
	};

	/// \brief The specialization for float big files
	template <> struct type< float >{
		/// \brief Type in big files
		static unsigned short const value = sizeof(float) | 0x10;
	};
	/// \brief The specialization for double big files
	template <> struct type< double >{
		/// \brief Type in big files
		static unsigned short const value = sizeof(double) | 0x10;
	};
	/// \brief The specialization for long double big files
	template <> struct type< long double >{
		/// \brief Type in big files
		static unsigned short const value = sizeof(long double) | 0x10;
	};


	/// \brief bit type off signed char
	typedef bitmap<signed char>    char_type;
	/// \brief bit type off unsigned char
	typedef bitmap<unsigned char>  unsigned_char_type;
	/// \brief bit type off short
	typedef bitmap<signed short>   short_type;
	/// \brief bit type off unsigned short
	typedef bitmap<unsigned short> unsigned_short_type;
	/// \brief bit type off int
	typedef bitmap<signed int>     int_type;
	/// \brief bit type off unsigned int
	typedef bitmap<unsigned int>   unsigned_int_type;
	/// \brief bit type off long
	typedef bitmap<signed long>    long_type;
	/// \brief bit type off unsigned long
	typedef bitmap<unsigned long>  unsigned_long_type;
	/// \brief bit type off float
	typedef bitmap<float>          float_type;
	/// \brief bit type off double
	typedef bitmap<double>         double_type;
	/// \brief bit type off long double
	typedef bitmap<long double>    long_double_type;

	/// \brief bit type for an image
	typedef unsigned_char_type image;
	/// \brief bit type for a quality picture
	typedef unsigned_char_type quality;
	/// \brief bit type for a finephase
	typedef float_type         finephase;


} }

#endif
