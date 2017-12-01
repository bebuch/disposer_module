//-----------------------------------------------------------------------------
// Copyright (c) 2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <bitmap/binary_read.hpp>

#include <io_tools/range_to_string.hpp>

#include <disposer/module.hpp>

#include <boost/dll.hpp>


namespace disposer_module::decode_bbf{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;
	using namespace hana::literals;

	namespace pixel = ::bmp::pixel;
	using ::bmp::bitmap;

	constexpr std::array< std::string_view, 41 > list{{
			"bool",
			"g8s",
			"g16s",
			"g32s",
			"g64s",
			"g8u",
			"g16u",
			"g32u",
			"g64u",
			"g32f",
			"g64f",
			"ga8s",
			"ga16s",
			"ga32s",
			"ga64s",
			"ga8u",
			"ga16u",
			"ga32u",
			"ga64u",
			"ga32f",
			"ga64f",
			"rgb8s",
			"rgb16s",
			"rgb32s",
			"rgb64s",
			"rgb8u",
			"rgb16u",
			"rgb32u",
			"rgb64u",
			"rgb32f",
			"rgb64f",
			"rgba8s",
			"rgba16s",
			"rgba32s",
			"rgba64s",
			"rgba8u",
			"rgba16u",
			"rgba32u",
			"rgba64u",
			"rgba32f",
			"rgba64f"
		}};

	constexpr auto dim = dimension_c<
			bool,
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
		>;

	std::string format_description(){
		static_assert(dim.type_count == list.size());
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
			"decodes an image from BBF image format"
				/*+ std::string(bmp::bbf_specification)*/,
			dimension_list{
				dim
			},
			module_configure(
				make("data"_in, free_type_c< std::string >,
					"the BBF encoded image"),
				make("format"_param, free_type_c< std::size_t >,
					"set dimension 1 by value:" + format_description(),
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
					std::istringstream is(value);
					bitmap< type > result;
					binary_read(result, is);
					module("image"_out).push(result);
				}
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
