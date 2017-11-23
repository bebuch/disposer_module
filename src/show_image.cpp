//-----------------------------------------------------------------------------
// Copyright (c) 2015-2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <bitmap/bitmap.hpp>
#include <bitmap/pixel.hpp>

#include <disposer/module.hpp>

#include <boost/dll.hpp>

#include <CImg.h>



namespace disposer_module::show_image{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;
	using hana::type_c;

	namespace pixel = ::bmp::pixel;
	using ::bmp::bitmap;


	template < typename T >
	struct resources;

	template <>
	struct resources< std::uint8_t >{
		resources(std::string const& title):
			display(200, 100, title.c_str()){}

		cimg_library::CImgDisplay display;
		cimg_library::CImg< std::uint8_t > img;

		void set_image(bitmap< std::uint8_t > const& image){
			img = cimg_library::CImg< std::uint8_t >(
				reinterpret_cast< std::uint8_t const* >(image.data()),
				image.width(), image.height(), 1, 1);
		}
	};

	template <>
	struct resources< std::uint16_t >{
		resources(std::string const& title):
			display(200, 100, title.c_str()){}

		cimg_library::CImgDisplay display;
		cimg_library::CImg< std::uint16_t > img;

		void set_image(bitmap< std::uint16_t > const& image){
			img = cimg_library::CImg< std::uint16_t >(
				reinterpret_cast< std::uint16_t const* >(image.data()),
				image.width(), image.height(), 1, 1);
		}
	};

	template <>
	struct resources< pixel::rgb8u >{
		resources(std::string const& title):
			display(200, 100, title.c_str()){}

		cimg_library::CImgDisplay display;
		cimg_library::CImg< std::uint8_t > img;

		void set_image(bitmap< pixel::rgb8u > const& image){
			img = cimg_library::CImg< std::uint8_t >(
				reinterpret_cast< std::uint8_t const* >(image.data()),
				image.width(), image.height(), 1, 3);
		}
	};


	void init(std::string const& name, module_declarant& disposer){
		auto init = generate_module(
			dimension_list{
				dimension_c<
					std::uint8_t,
					std::uint16_t,
					pixel::rgb8u
				>
			},
			module_configure(
				make("image"_in, wrapped_type_ref_c< bitmap, 0 >),
				make("window_title"_param, free_type_c< std::string >)
			),
			module_init_fn([](auto const module){
				using type = typename
					decltype(module.dimension(hana::size_c< 0 >))::type;

				return resources< type >(module("window_title"_param));
			}),
			exec_fn([](auto module){
				auto& state = module.state();
				for(auto const& img: module("image"_in).references()){
					state.set_image(img);

					if(
						state.display.width() != state.img.width() ||
						state.display.height() != state.img.height()
					){
						state.display.resize(state.img, false);
					}

					state.display.display(state.img);
					state.display.show();
				}
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)



}
