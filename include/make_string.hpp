//-----------------------------------------------------------------------------
// Copyright (c) 2011-2015 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _disposer_module_make_string_hpp_INCLUDED_
#define _disposer_module_make_string_hpp_INCLUDED_

#include <sstream>
#include <iomanip>


namespace disposer_module{


	namespace impl{ namespace make_string{


		template < typename Stream >
		inline void stream_output(Stream& s){}

		template < typename Stream, typename Head >
		inline void stream_output(Stream& s, Head&& head){
			s << std::forward< Head >(head);
		}

		template < typename Stream, typename Head, typename ... T >
		inline void stream_output(Stream& s, Head&& head, T&& ... args){
			s << std::forward< Head >(head);
			stream_output(s, std::forward< T >(args) ...);
		}

		template < typename Stream, typename Separator >
		inline void stream_output_separated_by(Stream& s, Separator const& separator){}

		template < typename Stream, typename Separator, typename Head >
		inline void stream_output_separated_by(Stream& s, Separator const& separator, Head&& head){
			s << std::forward< Head >(head);
		}

		template < typename Stream, typename Separator, typename Head, typename ... T >
		inline void stream_output_separated_by(Stream& s, Separator const& separator, Head&& head, T&& ... args){
			s << std::forward< Head >(head) << separator;
			stream_output_separated_by(s, separator, std::forward< T >(args) ...);
		}


	} }


	template < typename ... T >
	inline std::string make_string(T&& ... args){
		std::ostringstream os;
		os << std::boolalpha;
		impl::make_string::stream_output(os, std::forward< T >(args) ...);
		return os.str();
	}

	template < typename Separator, typename ... T >
	inline std::string make_string_separated_by(Separator const& separator, T&& ... args){
		std::ostringstream os;
		os << std::boolalpha;
		impl::make_string::stream_output_separated_by(os, separator, std::forward< T >(args) ...);
		return os.str();
	}


}


#endif
