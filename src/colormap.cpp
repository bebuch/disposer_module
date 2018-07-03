//-----------------------------------------------------------------------------
// Copyright (c) 2017-2018 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <disposer/module.hpp>

#include <bitmap/bitmap.hpp>
#include <bitmap/pixel.hpp>

#include <io_tools/range_to_string.hpp>

#include <boost/dll.hpp>


namespace disposer_module::subbitmap{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;

	namespace pixel = ::bmp::pixel;

	constexpr auto dim1 = dimension_c<
			std::int8_t,
			std::int16_t,
			std::int32_t,
			std::int64_t,
			std::uint8_t,
			std::uint16_t,
			std::uint32_t,
			std::uint64_t,
			float,
			double
		>;

	constexpr auto dim2 = dimension_c<
			pixel::rgb8,
			pixel::rgb16,
			pixel::rgb32,
			pixel::rgb64,
			pixel::rgb8u,
			pixel::rgb16u,
			pixel::rgb32u,
			pixel::rgb64u,
			pixel::rgb32f,
			pixel::rgb64f
		>;

	constexpr std::array< std::string_view, 10 > list{{
			"int8",
			"int16",
			"int32",
			"int64",
			"uint8",
			"uint16",
			"uint32",
			"uint64",
			"float32",
			"float64"
		}};

	std::string format_description(){
		static_assert(dim2.type_count == list.size());
		std::ostringstream os;
		std::size_t i = 0;
		hana::for_each(dim2.types, [&os, &i](auto t){
				os << "\n* " << list[i] << " => "
					<< ct_pretty_name< typename decltype(t)::type >();
				++i;
			});
		return os.str();
	}

	template < typename T >
	using bitmap = ::bmp::bitmap< T >;

	template < typename OutT, typename Module, typename InT >
	bitmap< OutT > exec(Module const module, bitmap< InT > const& image){
		auto const min = module("gray_min"_param);
		auto const max = module("gray_max"_param);
		auto const diff = max - min;

		auto const channel_min = module("color_channel_min"_param);
		auto const channel_max = module("color_channel_max"_param);
		auto const channel_diff = channel_max - channel_min;

		bitmap< OutT > result(image.size());
		std::transform(std::cbegin(image), std::cend(image), std::begin(result),
			[min, max, diff, channel_min, channel_max, channel_diff]
			(auto value){
				using channel_type = typename OutT::value_type;

				if(value < min){
					value = min;
				}

				if(value > max){
					value = max;
				}

				value -= min;
				double pos = static_cast< double >(value) / diff;

				if(pos < 1. / 6){
					pos *= 3 * M_PI;
					return OutT{
						channel_max,
						channel_min,
						static_cast< channel_type >(
							std::sin(M_PI / 2 - pos) * channel_diff
							+ channel_min)};
				}else if(pos < 2. / 6){
					pos = (pos - 1. / 6) * 3 * M_PI;
					return OutT{
						channel_max,
						static_cast< channel_type >(
							std::sin(pos) * channel_diff + channel_min),
						channel_min};
				}else if(pos < 3. / 6){
					pos = (pos - 2. / 6) * 3 * M_PI;
					return OutT{
						static_cast< channel_type >(
							std::sin(M_PI / 2 - pos) * channel_diff
							+ channel_min),
						channel_max,
						channel_min};
				}else if(pos < 4. / 6){
					pos = (pos - 3. / 6) * 3 * M_PI;
					return OutT{
						channel_min,
						channel_max,
						static_cast< channel_type >(
							std::sin(pos) * channel_diff + channel_min)};
				}else if(pos < 5. / 6){
					pos = (pos - 4. / 6) * 3 * M_PI;
					return OutT{
						channel_min,
						static_cast< channel_type >(
							std::sin(M_PI / 2 - pos) * channel_diff
							+ channel_min),
						channel_max};
				}else{
					pos = (pos - 5. / 6) * 3 * M_PI;
					return OutT{
						static_cast< channel_type >(
							std::sin(pos) * channel_diff + channel_min),
						channel_min,
						channel_max};
				}
			});

		return result;
	}

	void init(std::string const& name, declarant& disposer){
		auto init = generate_module(
			"map a color table to a gray scale image",
			dimension_list{
				dim1,
				dim2
			},
			module_configure(
				make("format"_param,
					free_type_c< std::optional< std::size_t > >,
					"set dimension 2 by dimension 1 (default) or by value:"
						+ format_description(),
					parser_fn([](std::string_view data){
						auto iter = std::find(list.begin(), list.end(), data);
						if(iter == list.end()){
							throw std::runtime_error("unknown value '"
								+ std::string(data)
								+ "', valid values are: "
								+ io_tools::range_to_string(list));
						}
						return iter - list.begin();
					})),
				make("image"_in, wrapped_type_ref_c< bitmap, 0 >,
					"original bitmap"),
				make("gray_min"_param, type_ref_c< 0 >,
					"value in image that is mapped to the minimal table color"),
				make("gray_max"_param, type_ref_c< 0 >,
					"value in image that is mapped to the maximal table color",
					verify_value_fn([](auto const& max, auto const module){
						if(module("gray_min"_param) < max) return;
						throw std::logic_error(
							"gray_max must be greater gray_min");
					})),
				set_dimension_fn([](auto const module){
					auto const optional_number = module("format"_param);
					std::size_t const number =
						[module, &optional_number]()->std::size_t{
							if(optional_number){
								return *optional_number;
							}else{
								using namespace hana::literals;
								auto type =
									hana::template_< pixel::basic_rgb >(
										module.dimension(hana::size_c< 0 >));
								auto types =
									module.dimension_types(hana::size_c< 1 >);
								auto index = hana::index_if(types,
									hana::equal.to(type));
								return index.value();
							}
						}();
					return solved_dimensions{index_component< 1 >{number}};
				}),
				make("color_channel_min"_param,
					wrapped_type_ref_c< pixel::channel_type_t, 1 >,
					"minimal value of any RGB channel"),
				make("color_channel_max"_param,
					wrapped_type_ref_c< pixel::channel_type_t, 1 >,
					"maximal value of any RGB channel",
					verify_value_fn([](auto const& max, auto const module){
						if(module("color_channel_min"_param) < max) return;
						throw std::logic_error("color_channel_max must be "
							"greater color_channel_min");
					})),
				make("image"_out, wrapped_type_ref_c< bitmap, 1 >,
					"target bitmap")
			),
			exec_fn([](auto module){
				for(auto const& img: module("image"_in).references()){
					using out_t = typename std::remove_reference_t<
						decltype(module("image"_out)) >::type::value_type;
					module("image"_out).push(exec< out_t >(module, img));
				}
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
