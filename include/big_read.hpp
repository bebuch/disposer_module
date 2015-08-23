//-----------------------------------------------------------------------------
// Copyright (c) 2009-2015 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _disposer_module__big_read__hpp_INCLUDED_
#define _disposer_module__big_read__hpp_INCLUDED_

#include "big_types.hpp"
#include "big_exception.hpp"
#include "big_undef_conversion.hpp"

#include <string>
#include <fstream>
#include <cstdint>

namespace disposer_module{ namespace big{


	/// \brief Header of a big file
	struct header{
		std::uint16_t width;
		std::uint16_t height;
		std::uint16_t type;
		std::uint32_t placeholder;
	};

	/// \brief Loads a big file by a given filename
	/// \throw disposer_module::big::big_error
	template < typename BitmapType >
	void read(BitmapType& bitmap, std::string const& filename);

	/// \brief Loads a big file from a std::istream
	/// \throw disposer_module::big::big_error
	template < typename BitmapType >
	void read(BitmapType& bitmap, std::istream& is);

	/// \brief Loads the type-information of a big file from a std::istream
	/// \throw disposer_module::big::big_error
	header read_header(std::istream& is);

	/// \brief Loads the data of a big file from a std::istream
	/// First you must use the read_header-function to read the header and resize the bitmap
	/// \throw disposer_module::big::big_error
	template < typename BitmapType >
	void read_data(BitmapType& bitmap, std::istream& is);


	//=============================================================================
	// Implementation
	//=============================================================================

	template < typename BitmapType >
	void read(BitmapType& bitmap, std::string const& filename){
		std::ifstream is(filename.c_str(), std::ios_base::in | std::ios_base::binary);

		if(!is.is_open()){
			throw big_error("Can't open file: " + filename);
		}

		try{
			read(bitmap, is);
		}catch(big_error const& error){
			throw big_error(std::string(error.what()) + ": " + filename);
		}
	}

	template < typename BitmapType >
	void read(BitmapType& bitmap, std::istream& is){
		using value_type = std::remove_cv_t< std::remove_pointer_t< decltype(bitmap.data()) > >;

		header header = read_header(is);

		if(header.type != big::type_v< value_type >){
			throw big_error("Type in file is not compatible");
		}

		// The last 4 bits in the type get the size of a single value
		if((header.type & 0x000F) != sizeof(value_type)){
			throw big_error("Size type in file is not compatible");
		}

		bitmap.resize(header.width, header.height);

		read_data(bitmap, is);
	}

	inline header read_header(std::istream& is){
		header header; // big header informations

		// read the file header (10 Byte)
		is.read(reinterpret_cast< char* >(&header.width),  2);
		is.read(reinterpret_cast< char* >(&header.height), 2);
		is.read(reinterpret_cast< char* >(&header.type),   2);
		is.read(reinterpret_cast< char* >(&header.placeholder), 4);

		if(!is.good()){
			throw big_error("Can't read big header");
		}

		return header;
	}

	template < typename BitmapType >
	void read_data(BitmapType& bitmap, std::istream& is){
		using value_type = std::remove_cv_t< std::remove_pointer_t< decltype(bitmap.data()) > >;

		is.read(
			reinterpret_cast< char* >(bitmap.data()),
			bitmap.width() * bitmap.height() * sizeof(value_type)
		);

		if(!is.good()){
			throw big_error("Can't read big content");
		}

		if(std::numeric_limits< value_type >::has_quiet_NaN){
			bitmap = convert_undef_to_nan(std::move(bitmap));
		}
	}


} }

#endif
