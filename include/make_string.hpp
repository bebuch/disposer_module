//-----------------------------------------------------------------------------
// Copyright (c) 2011-2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _disposer_module__make_string__hpp_INCLUDED_
#define _disposer_module__make_string__hpp_INCLUDED_

#include <sstream>
#include <iomanip>


namespace disposer_module{


	template < typename ... T >
	inline std::string make_string(T&& ... args){
		std::ostringstream os;
		os << std::boolalpha;
		(os << ... << static_cast< T&& >(args));
		return os.str();
	}

	template < typename Separator, typename T, typename ... Ts >
	inline std::string make_string_separated_by(
		Separator const& separator,
		T&& arg,
		Ts&& ... args
	){
		std::ostringstream os;
		os << std::boolalpha;
		os << static_cast< T&& >(arg);
		((os << separator) << ... << static_cast< Ts&& >(args));
		return os.str();
	}

	template < typename Separator >
	inline std::string make_string_separated_by(Separator const&){
		return {};
	}


}


#endif
