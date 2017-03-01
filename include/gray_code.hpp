//-----------------------------------------------------------------------------
// Copyright (c) 2012-2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _disposer_module__gray_code__hpp_INCLUDED_
#define _disposer_module__gray_code__hpp_INCLUDED_

#include <cstdint>
#include <utility>
#include <array>


namespace disposer_module{


	namespace impl{ namespace gray_code{


		constexpr std::uint8_t gray_to_binary_code(std::uint8_t x){
			return
				(x >> 0) ^
				(x >> 1) ^
				(x >> 2) ^
				(x >> 3) ^
				(x >> 4) ^
				(x >> 5) ^
				(x >> 6) ^
				(x >> 7);
		}

		template < std::size_t ... i >
		constexpr auto gray_to_binary_code_map(std::index_sequence< i ... >){
			return std::array< std::uint8_t, 256 >{{
				gray_to_binary_code(static_cast< std::uint8_t >(i)) ...
			}};
		}

		constexpr auto map =
			gray_to_binary_code_map(std::make_index_sequence< 256 >());


	} }


	constexpr std::uint8_t gray_to_binary_code(std::uint8_t gray){
		return impl::gray_code::map[gray];
	}

	constexpr std::uint16_t gray_to_binary_code(std::uint16_t gray){
		return
			(static_cast< std::uint16_t >(gray_to_binary_code(
				static_cast< std::uint8_t >(gray >> 8))) << 8) +
			(
				gray_to_binary_code(
					static_cast< std::uint8_t >(gray >> 8)) & 1
				? static_cast< std::uint8_t >(~gray_to_binary_code(
					static_cast< std::uint8_t >(gray)))
				: gray_to_binary_code(static_cast< std::uint8_t >(gray))
			);
	}

	constexpr std::uint32_t gray_to_binary_code(std::uint32_t gray){
		return
			(static_cast< std::uint32_t >(gray_to_binary_code(
				static_cast< std::uint16_t >(gray >> 16))) << 16) +
			(
				gray_to_binary_code(
					static_cast< std::uint16_t >(gray >> 16)) & 1
				? static_cast< std::uint16_t >(~gray_to_binary_code(
					static_cast< std::uint16_t >(gray)))
				: gray_to_binary_code(static_cast< std::uint16_t >(gray))
			);
	}

	constexpr std::uint64_t gray_to_binary_code(std::uint64_t gray){
		return
			(static_cast< std::uint64_t >(gray_to_binary_code(
				static_cast< std::uint32_t >(gray >> 32))) << 32) +
			(
				gray_to_binary_code(
					static_cast< std::uint32_t >(gray >> 32)) & 1
				? static_cast< std::uint32_t >(~gray_to_binary_code(
					static_cast< std::uint32_t >(gray)))
				: gray_to_binary_code(static_cast< std::uint32_t >(gray))
			);
	}

	constexpr std::uint8_t gray_to_binary_code(std::int8_t gray){
		return gray_to_binary_code(static_cast< std::uint8_t >(gray));
	}

	constexpr std::uint16_t gray_to_binary_code(std::int16_t gray){
		return gray_to_binary_code(static_cast< std::uint16_t >(gray));
	}

	constexpr std::uint32_t gray_to_binary_code(std::int32_t gray){
		return gray_to_binary_code(static_cast< std::uint32_t >(gray));
	}

	constexpr std::uint64_t gray_to_binary_code(std::int64_t gray){
		return gray_to_binary_code(static_cast< std::uint64_t >(gray));
	}

	constexpr std::uint8_t binary_to_gray_code(std::uint8_t binary){
		return (binary >> 1) ^ binary;
	}

	constexpr std::uint16_t binary_to_gray_code(std::uint16_t binary){
		return (binary >> 1) ^ binary;
	}

	constexpr std::uint32_t binary_to_gray_code(std::uint32_t binary){
		return (binary >> 1) ^ binary;
	}

	constexpr std::uint64_t binary_to_gray_code(std::uint64_t binary){
		return (binary >> 1) ^ binary;
	}

	constexpr std::uint8_t binary_to_gray_code(std::int8_t binary){
		return binary_to_gray_code(static_cast< std::uint8_t >(binary));
	}

	constexpr std::uint16_t binary_to_gray_code(std::int16_t binary){
		return binary_to_gray_code(static_cast< std::uint16_t >(binary));
	}

	constexpr std::uint32_t binary_to_gray_code(std::int32_t binary){
		return binary_to_gray_code(static_cast< std::uint32_t >(binary));
	}

	constexpr std::uint64_t binary_to_gray_code(std::int64_t binary){
		return binary_to_gray_code(static_cast< std::uint64_t >(binary));
	}


}

#endif
