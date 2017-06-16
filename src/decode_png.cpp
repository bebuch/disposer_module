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

#include <png++/png.hpp>


namespace disposer_module::decode_png{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;
	using namespace hana::literals;
	using hana::type_c;

	namespace pixel = ::bitmap::pixel;
	using ::bitmap::bitmap;


	constexpr auto types = hana::tuple_t<
			std::uint8_t,
			std::uint16_t,
			pixel::ga8u,
			pixel::ga16u,
			pixel::rgb8u,
			pixel::rgb16u,
			pixel::rgba8u,
			pixel::rgba16u
		>;


	enum class format{
		g8,
		g16,
		ga8,
		ga16,
		rgb8,
		rgb16,
		rgba8,
		rgba16
	};


	template < typename T1, typename T2 >
	constexpr auto make_type_pair()noexcept{
		return hana::make_pair(type_c< T1 >, type_c< T2 >);
	}

	auto constexpr bitmap_to_png_type = hana::make_map(
		make_type_pair< std::uint8_t, png::gray_pixel >(),
		make_type_pair< std::uint16_t, png::gray_pixel_16 >(),
		make_type_pair< pixel::ga8u, png::ga_pixel >(),
		make_type_pair< pixel::ga16u, png::ga_pixel_16 >(),
		make_type_pair< pixel::rgb8u, png::rgb_pixel >(),
		make_type_pair< pixel::rgb16u, png::rgb_pixel_16 >(),
		make_type_pair< pixel::rgba8u, png::rgba_pixel >(),
		make_type_pair< pixel::rgba16u, png::rgba_pixel_16 >()
	);


	template < typename T >
	auto decode_png(std::string const& data){
		using png_type =
			typename decltype(+bitmap_to_png_type[type_c< T >])::type;

		std::istringstream is(data, std::ios::in | std::ios::binary);
		png::image< png_type > png_image;
		png_image.read_stream(is);

		bitmap< T > img(
			static_cast< std::size_t >(png_image.get_width()),
			static_cast< std::size_t >(png_image.get_height())
		);
		for(std::size_t y = 0; y < img.height(); ++y){
			for(std::size_t x = 0; x < img.width(); ++x){
				auto& pixel = reinterpret_cast< png_type& >(img(x, y));
				pixel = png_image.get_pixel(x, y);
			}
		}

		return img;
	}

	void init(std::string const& name, module_declarant& disposer){
		auto init = module_register_fn(
			module_configure(
				"data"_in(type_c< std::string >,
					required),
				"format"_param(type_c< format >,
					parser([](
						auto const& /*iop*/,
						std::string_view data,
						hana::basic_type< format >
					){
						if(data == "g8") return format::g8;
						if(data == "g16") return format::g16;
						if(data == "ga8") return format::ga8;
						if(data == "ga16") return format::ga16;
						if(data == "rgb8") return format::rgb8;
						if(data == "rgb16") return format::rgb16;
						if(data == "rgba8") return format::rgba8;
						if(data == "rgba16") return format::rgba16;
						throw std::runtime_error("unknown value '"
							+ std::string(data) + "', allowed values are: "
							"g8, g16, ga8, ga16, rgb8, rgb16, rgba8, rgba16");

					}),
					type_as_text(
						hana::make_pair(type_c< format >, "format"_s)
					)
				),
				"image"_out(types,
					template_transform_c< bitmap >,
					enable([](auto const& iop, auto type){
						auto const format = iop("format"_param).get();
						return
							(type == type_c< std::uint8_t > &&
								format == format::g8) ||
							(type == type_c< std::uint16_t > &&
								format == format::g16) ||
							(type == type_c< pixel::ga8u > &&
								format == format::ga8) ||
							(type == type_c< pixel::ga16u > &&
								format == format::ga16) ||
							(type == type_c< pixel::rgb8u > &&
								format == format::rgb8) ||
							(type == type_c< pixel::rgb16u > &&
								format == format::rgb16) ||
							(type == type_c< pixel::rgba8u > &&
								format == format::rgba8) ||
							(type == type_c< pixel::rgba16u > &&
								format == format::rgba16);
					})
				)
			),
			normal_id_increase(),
			module_enable([]{
				return [](auto& module){
					auto& out = module("image"_out);
					auto values = module("data"_in).get_references();
					for(auto const& pair: values){
						auto const& data = pair.second;
						switch(module("format"_param).get()){
						case format::g8:
							out.put(decode_png< std::uint8_t >(data));
						break;
						case format::g16:
							out.put(decode_png< std::uint16_t >(data));
						break;
						case format::ga8:
							out.put(decode_png< pixel::ga8u >(data));
						break;
						case format::ga16:
							out.put(decode_png< pixel::ga16u >(data));
						break;
						case format::rgb8:
							out.put(decode_png< pixel::rgb8u >(data));
						break;
						case format::rgb16:
							out.put(decode_png< pixel::rgb16u >(data));
						break;
						case format::rgba8:
							out.put(decode_png< pixel::rgba8u >(data));
						break;
						case format::rgba16:
							out.put(decode_png< pixel::rgba16u >(data));
						break;
						}
					}
				};
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
