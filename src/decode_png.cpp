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

#include <io_tools/range_to_string.hpp>

#include <boost/dll.hpp>

#include <boost/hana/at_key.hpp>

#include <png++/png.hpp>


namespace disposer_module::decode_png{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;
	using namespace hana::literals;
	using hana::type_c;

	namespace pixel = ::bmp::pixel;
	using ::bmp::bitmap;


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
	bitmap< T > decode(std::string const& data){
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

	constexpr std::array< std::string_view, 8 > list{{
			"g8",
			"g16",
			"ga8",
			"ga16",
			"rgb8",
			"rgb16",
			"rgba8",
			"rgba16"
		}};

	constexpr auto dim = dimension_c<
			std::uint8_t,
			std::uint16_t,
			pixel::ga8u,
			pixel::ga16u,
			pixel::rgb8u,
			pixel::rgb16u,
			pixel::rgba8u,
			pixel::rgba16u
		>;

	std::string format_description(){
		std::ostringstream os;
		std::size_t i = 0;
		hana::for_each(dim.types, [&os, &i](auto t){
				os << "\n* " << list[i] << " => "
					<< ct_pretty_name< typename decltype(t)::type >();
				++i;
			});
		return os.str();
	}


	void init(std::string const& name, module_declarant& disposer){
		auto init = generate_module(
			"decodes an image from PNG image format",
			dimension_list{
				dim
			},
			module_configure(
				make("data"_in, free_type_c< std::string >,
					"PNG encoded binary data"),
				make("format"_param, free_type_c< std::size_t >,
					"set dimension 1 by value: " + format_description(),
					parser_fn([](
						auto const /*module*/,
						std::string_view data,
						hana::basic_type< std::size_t >
					){
						auto iter = std::find(list.begin(), list.end(), data);
						if(iter == list.end()){
							throw std::runtime_error("unknown value '"
								+ std::string(data)
								+ "', valid values are: "
								+ io_tools::range_to_string(list));
						}
						return iter - list.begin();
					})),
				set_dimension_fn([](auto const module){
					std::size_t const number = module("format"_param);
					return solved_dimensions{index_component< 0 >{number}};
				}),
				make("image"_out, wrapped_type_ref_c< bitmap, 0 >,
					"the decoded image")
			),
			exec_fn([](auto module){
				using type = typename
					decltype(module.dimension(hana::size_c< 0 >))::type;

				for(auto const& value: module("data"_in).references()){
					module("image"_out).push(decode< type >(value));
				}
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
