//-----------------------------------------------------------------------------
// Copyright (c) 2009-2015 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _disposer_module__big_exception__hpp_INCLUDED_
#define _disposer_module__big_exception__hpp_INCLUDED_

#include <string>
#include <stdexcept>

namespace disposer_module{ namespace big{


	/// \brief Exception class that will throws by load or save big files
	class big_error: public std::runtime_error{
	public:
		/// \brief Constructs an object of class big_error
		explicit big_error(std::string const& what_arg);

		/// \brief Constructs an object of class big_error
		explicit big_error(char const* what_arg);
	};


	//=========================================================================
	// Implementation
	//=========================================================================

	inline
	big_error::big_error(
		std::string const& what_arg
	):
		std::runtime_error(what_arg)
		{}

	inline
	big_error::big_error(
		char const* what_arg
	):
		std::runtime_error(what_arg)
		{}


} }

#endif
