//-----------------------------------------------------------------------------
// Copyright (c) 2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _disposer_module__type_enabled__hpp_INCLUDED_
#define _disposer_module__type_enabled__hpp_INCLUDED_

#include <boost/hana.hpp>

#include <sstream>


namespace disposer_module{


	constexpr auto map_to = [](auto&& set, auto const& value){
		namespace hana = boost::hana;
		return hana::unpack(
				static_cast< decltype(set) >(set), hana::make_map ^hana::on^
				hana::curry< 2 >(hana::flip(hana::make_pair))(value)
			);
	};


	template < typename Set >
	using type_enabled = decltype(map_to(Set{}, false));

	constexpr auto is_value_false = [](auto const& key_value){
		namespace hana = boost::hana;
		return !hana::second(key_value);
	};

	constexpr auto output_value_list = [](auto const& map){
		namespace hana = boost::hana;
		std::ostringstream os;
		os << "'type_";
		bool first = true;
		hana::for_each(map, [&os, first](auto const& key_value){
			if(!first){ os << "', 'type_"; }else{ first = false; }
			os << hana::second(key_value);
		});
		os << "'";
		return os.str();
	};


}


#endif
