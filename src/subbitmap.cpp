//-----------------------------------------------------------------------------
// Copyright (c) 2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <disposer/module.hpp>

#include <bitmap/subbitmap.hpp>

#include <boost/dll.hpp>


namespace disposer_module::subbitmap{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;

	namespace pixel = ::bmp::pixel;

	template < typename T >
	using bitmap = ::bmp::bitmap< T >;


	template < typename Module, typename T >
	bitmap< T > exec(Module const module, bitmap< T > const& image){
		auto const x = module("x"_param);
		auto const y = module("y"_param);
		auto const w = module("width"_param);
		auto const h = module("height"_param);
		return ::bmp::subbitmap(image, ::bmp::rect{x, y, w, h});
	}


	void init(std::string const& name, module_declarant& disposer){
		auto init = generate_module(
			dimension_list{
				dimension_c<
					std::int8_t,
					std::int16_t,
					std::int32_t,
					std::int64_t,
					std::uint8_t,
					std::uint16_t,
					std::uint32_t,
					std::uint64_t,
					float,
					double,
					pixel::ga8,
					pixel::ga16,
					pixel::ga32,
					pixel::ga64,
					pixel::ga8u,
					pixel::ga16u,
					pixel::ga32u,
					pixel::ga64u,
					pixel::ga32f,
					pixel::ga64f,
					pixel::rgb8,
					pixel::rgb16,
					pixel::rgb32,
					pixel::rgb64,
					pixel::rgb8u,
					pixel::rgb16u,
					pixel::rgb32u,
					pixel::rgb64u,
					pixel::rgb32f,
					pixel::rgb64f,
					pixel::rgba8,
					pixel::rgba16,
					pixel::rgba32,
					pixel::rgba64,
					pixel::rgba8u,
					pixel::rgba16u,
					pixel::rgba32u,
					pixel::rgba64u,
					pixel::rgba32f,
					pixel::rgba64f
				>
			},
			module_configure(
				make("x"_param, free_type_c< float >),
				make("y"_param, free_type_c< float >),
				make("width"_param, free_type_c< std::size_t >),
				make("height"_param, free_type_c< std::size_t >),
				make("image"_in, wrapped_type_ref_c< bitmap, 0 >),
				make("image"_out, wrapped_type_ref_c< bitmap, 0 >)
			),
			exec_fn([](auto module){
				for(auto const& img: module("image"_in).references()){
					module("image"_out).push(exec(module, img));
				}
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
