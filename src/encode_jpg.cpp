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

#include <turbojpeg.h>


namespace disposer_module::encode_jpg{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;

	namespace pixel = ::bitmap::pixel;
	using ::bitmap::bitmap;


	constexpr auto types = hana::tuple_t<
			std::int8_t,
			std::uint8_t,
			pixel::rgb8,
			pixel::rgb8u
		>;


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


	void init(std::string const& name, module_declarant& disposer){
		auto init = module_register_fn(
			module_configure(
				"image"_in(types,
					template_transform_c< bitmap >,
					required),
				"data"_out(hana::type_c< std::string >),
				"quality"_param(hana::type_c< std::size_t >,
					value_verify([](auto const& /*iop*/, std::size_t value){
						if(value > 100){
							throw std::logic_error(
								"expected a percent value (0% - 100%)");
						}
					}),
					default_values(std::size_t(90)))
			),
			normal_id_increase(),
			[](auto const& /*module*/){
				return [](auto& module, std::size_t /*id*/){
					auto values = module("image"_in).get_references();
					for(auto const& pair: values){
						std::visit([&module](auto const& img){
							auto const quality = module("quality"_param).get();
							module("data"_out).put(to_jpg_image(
								img.get(), static_cast< int >(quality)));
						}, pair.second);
					}
				};
			}
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
