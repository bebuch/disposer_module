//-----------------------------------------------------------------------------
// Copyright (c) 2017-2018 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <disposer/module.hpp>

#include <bitmap/rect_transform.hpp>
#include <bitmap/rect_io.hpp>

#include <io_tools/range_to_string.hpp>
#include <io_tools/std_array_io.hpp>

#include <boost/dll.hpp>


namespace disposer_module::transform_bitmap{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;

	namespace pixel = ::bmp::pixel;

	template < typename T >
	using bitmap = ::bmp::bitmap< T >;

	template < typename T0, typename T1 >
	using transformed_bitmap = bitmap< std::conditional_t<
		std::numeric_limits< pixel::channel_type_t< T1 > >::has_quiet_NaN,
		pixel::with_channel_type_t< T0, T1 >,
		pixel::basic_masked_pixel< pixel::with_channel_type_t< T0, T1 > > > >;

	template < typename T >
	using calc_type = std::common_type_t< T, float >;

	template < typename T >
	using points_type = std::array< bmp::point< calc_type< T > >, 4 >;


	template < typename T >
	struct state{
		bmp::matrix3x3< T > const homography;
		bmp::rect< long, long, std::size_t, std::size_t > const contour;
	};

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

	constexpr auto dim2 = dimension_c<
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

	constexpr std::array< std::string_view, 10 > list{{
			"8s",
			"16s",
			"32s",
			"64s",
			"8u",
			"16u",
			"32u",
			"64u",
			"32f",
			"64f"
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


	void init(std::string const& name, declarant& disposer){
		auto init = generate_module(
			"perspective transform by 4 free points and an target rect",
			dimension_list{
				dim1,
				dim2
			},
			module_configure(
				make("source_size"_param,
					free_type_c< bmp::size< std::size_t > >,
					"expected size of the bitmaps"),
				make("target_size"_param,
					free_type_c< bmp::size< std::size_t > >,
					"width and height of the rect in the target image"),
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
				set_dimension_fn([](auto const module){
					auto const optional_number = module("format"_param);
					std::size_t const number =
						[module, &optional_number]()->std::size_t{
							if(optional_number){
								return *optional_number;
							}else{
								using namespace hana::literals;
								auto type = hana::type_c< pixel::channel_type_t<
									typename decltype(module.dimension(
										hana::size_c< 0 >))::type > >;
								auto types =
									module.dimension_types(hana::size_c< 1 >);
								auto index = hana::index_if(types,
									hana::equal.to(type));
								return index.value();
							}
						}();
					return solved_dimensions{index_component< 1 >{number}};
				}),
				make("source_points"_param,
					wrapped_type_ref_c< points_type, 1 >,
					"4 points in the original bitmap that are mapped to the "
					"rect points in the target bitmap\n"
					"* p1 => top left point\n"
					"* p2 => top right point\n"
					"* p3 => bottom left point\n"
					"* p4 => bottom right point",
					parser_fn([](
						std::string_view data,
						auto const type
					){
						using io_tools::std_array::operator>>;
						std::istringstream is((std::string(data)));
						typename decltype(type)::type result;
						is >> result;
						stream_parser_t::verify_istream(data, is);
						return result;
					})),
				make("image"_out,
					wrapped_type_ref_c< transformed_bitmap, 0, 1 >,
					"transformed bitmap")
			),
			module_init_fn([](auto module){
				auto t = module.dimension(hana::size_c< 1 >);
				using pixel_type = typename decltype(t)::type;
				using matrix_type = calc_type< pixel_type >;

				auto const source_points = module("source_points"_param);
				auto const source_size = module("source_size"_param);
				auto const target_size = module("target_size"_param);
				points_type< matrix_type > target_points{{
					{0, 0},
					{static_cast< matrix_type >(target_size.width()), 0},
					{0, static_cast< matrix_type >(target_size.height())},
					bmp::to_point< matrix_type >(target_size)
				}};

				auto const homography = bmp::rect_transform_homography(
					source_points, target_points);

				auto const target_rect = transform_image_contour(
					homography, source_size);

				auto const target_contour = bmp::image_contour(target_rect);

				module.log([&target_contour](logsys::stdlogb& os){
						os << "calculated image contour: " << target_contour;
					});

				return state< matrix_type >{invert(homography), target_contour};
			}),
			exec_fn([](auto module){
				auto t0 = module.dimension(hana::size_c< 0 >);
				using source_pixel_type = typename decltype(t0)::type;
				auto t1 = module.dimension(hana::size_c< 1 >);
				using target_channel_type = typename decltype(t1)::type;
				using target_pixel_type = pixel::with_channel_type_t<
					source_pixel_type, target_channel_type >;
				using matrix_type = calc_type< target_channel_type >;
				auto const& state = module.state();
				auto const source_size = module("source_size"_param);
				for(auto const& img: module("image"_in).references()){
					if(img.size() != source_size){
						throw std::logic_error("image has not expected size");
					}

					module("image"_out).push(bmp::transform_bitmap<
							target_pixel_type, source_pixel_type, matrix_type
						>(state.homography, img, state.contour));
				}
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
