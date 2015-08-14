//-----------------------------------------------------------------------------
// Copyright (c) 2009-2015 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _disposer_module_isubstream_hpp_INCLUDED_
#define _disposer_module_isubstream_hpp_INCLUDED_

#include "substreambuf.hpp"

#include <memory>


namespace disposer_module{


	class isubstream: public std::istream{
	public:
		isubstream(std::streambuf* buffer, std::streampos start, std::streamsize size):
			std::istream(&buffer_),
			buffer_(buffer, start, size)
			{}

		isubstream(isubstream const&) = delete;
		isubstream(isubstream&&) = default;

	private:
		substreambuf buffer_;
	};


}


#endif
