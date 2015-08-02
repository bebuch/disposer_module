//-----------------------------------------------------------------------------
// Copyright (c) 2009-2015 Benjamin Buch
//
// https://github.com/bebuch/disposer
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _disposer_module_big_write_hpp_INCLUDED_
#define _disposer_module_big_write_hpp_INCLUDED_

#include "big_types.hpp"
#include "big_exception.hpp"
#include "big_undef_conversion.hpp"

#include <string>
#include <fstream>
#include <cstdint>

namespace disposer_module{ namespace big{


	/// \brief Saves a image or quality picture by a given filename
	/// \throw disposer_module::big::big_error
	template < typename BitmapType >
	void write(BitmapType const& bitmap, std::string const& filename);

	/// \brief Writes a image or quality picture to a std::ostream
	/// \throw disposer_module::big::big_error
	template < typename BitmapType >
	void write(BitmapType const& bitmap, std::ostream& os);


	//=============================================================================
	// Implementation
	//=============================================================================

	template < typename BitmapType >
	void write(BitmapType const& bitmap, std::string const& filename){
		std::ofstream os(filename.c_str(), std::ios_base::out | std::ios_base::binary);

		if(!os.is_open()){
			throw big_error("Can't open file: " + filename);
		}

		try{
			write(bitmap, os);
		}catch(big_error const& error){
			throw big_error(std::string(error.what()) + ": " + filename);
		}
	}

	template < typename BitmapType >
	void write(BitmapType const& bitmap, std::ostream& os){
		// big header informations
		std::uint16_t width  = static_cast< std::uint16_t >(bitmap.width());
		std::uint16_t height = static_cast< std::uint16_t >(bitmap.height());
		std::uint16_t type   = big::type< typename BitmapType::value_type >::value;
		std::uint32_t placeholder = 0; // 4 bytes in header are reserved

		// write the file header
		os.write(reinterpret_cast< std::istream::char_type const* >(&width),  2);
		os.write(reinterpret_cast< std::istream::char_type const* >(&height), 2);
		os.write(reinterpret_cast< std::istream::char_type const* >(&type),   2);
		os.write(reinterpret_cast< std::istream::char_type const* >(&placeholder), 4);

		if(!os.good()){
			throw big_error("Can't write big header");
		}

		auto bytes = bitmap.width() * bitmap.height() * sizeof(typename BitmapType::value_type);

		os.write(
			reinterpret_cast< std::ifstream::char_type const* >(
				std::numeric_limits< typename BitmapType::value_type >::has_quiet_NaN ?
				convert_nan_to_undef(bitmap).data() :
				bitmap.data()
			),
			bytes
		);

		if(!os.good()){
			throw big_error("Can't write big content");
		}
	}


} }

#endif
