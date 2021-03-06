//-----------------------------------------------------------------------------
// Copyright (c) 2017-2018 Benjamin Buch
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

#include <turbojpeg.h>


namespace disposer_module::encode_jpg{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;

	namespace pixel = ::bmp::pixel;
	using ::bmp::bitmap;


	template < typename T >
	auto to_jpg_image(bitmap< T > const& img, int quality){
		// JPEG encoding
		auto compressor_deleter = [](void* data){ tjDestroy(data); };
		std::unique_ptr< void, decltype(compressor_deleter) > compressor(
			tjInitCompress(),
			compressor_deleter
		);
		if(!compressor) throw std::runtime_error("tjInitCompress failed");

		// Image buffer
		std::uint8_t* data = nullptr;
		unsigned long size = 0;

		if(tjCompress2(
			compressor.get(),
			const_cast< std::uint8_t* >(
				reinterpret_cast< std::uint8_t const* >(img.data())),
			static_cast< int >(img.width()),
			static_cast< int >(img.width() * sizeof(T)),
			static_cast< int >(img.height()),
			sizeof(T) == 3 ? TJPF_RGB: TJPF_GRAY,
			&data,
			&size,
			sizeof(T) == 3 ? TJSAMP_420 : TJSAMP_GRAY,
			quality,
			0
		) != 0) throw std::runtime_error("tjCompress2 failed");

		auto data_deleter = [](std::uint8_t* data){ tjFree(data); };
		std::unique_ptr< std::uint8_t, decltype(data_deleter) > data_ptr(
			data,
			data_deleter
		);

		// output
		return std::string(data_ptr.get(), data_ptr.get() + size);
	}


	void init(std::string const& name, declarant& disposer){
		auto init = generate_module(
			"encodes an image in JPEG image format",
			dimension_list{
				dimension_c<
					std::int8_t,
					std::uint8_t,
					pixel::rgb8,
					pixel::rgb8u
				>
			},
			module_configure(
				make("image"_in, wrapped_type_ref_c< bitmap, 0 >,
					"the image to be encoded"),
				make("data"_out, free_type_c< std::string >,
					"the resulting encoded binary data"),
				make("quality"_param, free_type_c< std::size_t >,
					"quality of the encoded image in percent",
					verify_value_fn([](std::size_t value){
						if(value > 100){
							throw std::logic_error(
								"expected a percent value (0% - 100%)");
						}
					}),
					default_value(90))
			),
			exec_fn([](auto module){
				for(auto const& img: module("image"_in).references()){
					auto const quality = module("quality"_param);
					module("data"_out).push(to_jpg_image(
						img, static_cast< int >(quality)));
				}
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
