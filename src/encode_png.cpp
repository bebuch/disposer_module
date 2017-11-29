//-----------------------------------------------------------------------------
// Copyright (c) 2017 Benjamin Buch
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

#include <boost/hana/at_key.hpp>

#include <png++/png.hpp>


namespace disposer_module::encode_png{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;

	namespace pixel = ::bmp::pixel;
	using ::bmp::bitmap;


	template < typename T1, typename T2 >
	constexpr auto make_type_pair()noexcept{
		return hana::make_pair(hana::type_c< T1 >, hana::type_c< T2 >);
	}

	auto constexpr bitmap_to_png_type = hana::make_map(
		make_type_pair< std::int8_t, png::gray_pixel >(),
		make_type_pair< std::int16_t, png::gray_pixel_16 >(),
		make_type_pair< std::uint8_t, png::gray_pixel >(),
		make_type_pair< std::uint16_t, png::gray_pixel_16 >(),
		make_type_pair< pixel::ga8, png::ga_pixel >(),
		make_type_pair< pixel::ga16, png::ga_pixel_16 >(),
		make_type_pair< pixel::ga8u, png::ga_pixel >(),
		make_type_pair< pixel::ga16u, png::ga_pixel_16 >(),
		make_type_pair< pixel::rgb8, png::rgb_pixel >(),
		make_type_pair< pixel::rgb16, png::rgb_pixel_16 >(),
		make_type_pair< pixel::rgb8u, png::rgb_pixel >(),
		make_type_pair< pixel::rgb16u, png::rgb_pixel_16 >(),
		make_type_pair< pixel::rgba8, png::rgba_pixel >(),
		make_type_pair< pixel::rgba16, png::rgba_pixel_16 >(),
		make_type_pair< pixel::rgba8u, png::rgba_pixel >(),
		make_type_pair< pixel::rgba16u, png::rgba_pixel_16 >()
	);


	template < typename T >
	auto to_png_image(bitmap< T > const& img){
		using png_type =
			typename decltype(+bitmap_to_png_type[hana::type_c< T >])::type;

		png::image< png_type > png_image(img.width(), img.height());

		for(std::size_t y = 0; y < img.height(); ++y){
			for(std::size_t x = 0; x < img.width(); ++x){
				auto& pixel = img(x, y);
				png_image.set_pixel(x, y,
					reinterpret_cast< png_type const& >(pixel));
			}
		}

		return png_image;
	}

	void init(std::string const& name, module_declarant& disposer){
		auto init = generate_module(
			"encodes an image in PNG image format",
			dimension_list{
				dimension_c<
					std::int8_t,
					std::int16_t,
					std::uint8_t,
					std::uint16_t,
					pixel::ga8,
					pixel::ga16,
					pixel::ga8u,
					pixel::ga16u,
					pixel::rgb8,
					pixel::rgb16,
					pixel::rgb8u,
					pixel::rgb16u,
					pixel::rgba8,
					pixel::rgba16,
					pixel::rgba8u,
					pixel::rgba16u
				>
			},
			module_configure(
				make("image"_in, wrapped_type_ref_c< bitmap, 0 >,
					"the image to be encoded"),
				make("data"_out, free_type_c< std::string >,
					"the resulting encoded binary data")
			),
			exec_fn([](auto module){
				for(auto const& img: module("image"_in).references()){
					std::ostringstream os(std::ios::out | std::ios::binary);
					to_png_image(img).write_stream(os);
					module("data"_out).push(os.str());
				}
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
