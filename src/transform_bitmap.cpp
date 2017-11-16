//-----------------------------------------------------------------------------
// Copyright (c) 2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <disposer/module.hpp>

#include <bitmap/rect_transform.hpp>
#include <bitmap/rect_io.hpp>

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



	void init(std::string const& name, module_declarant& disposer){
		auto init = module_register_fn(
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
				>,
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
					double
				>
			},
			module_configure(
				make("source_size"_param,
					free_type_c< bmp::size< std::size_t > >),
				make("target_size"_param,
					free_type_c< bmp::size< std::size_t > >),
				make("format"_param,
					free_type_c< std::optional< std::size_t > >,
					parser_fn([](
						auto const& /*iop*/,
						std::string_view data,
						hana::basic_type< std::optional< std::size_t > >
					){
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
						auto iter = std::find(list.begin(), list.end(), data);
						if(iter == list.end()){
							std::ostringstream os;
							os << "unknown value '" << data
								<< "', allowed values are: ";
							bool first = true;
							for(auto name: list){
								if(first){ first = false; }else{ os << ", "; }
								os << name;
							}
							throw std::runtime_error(os.str());
						}
						return std::optional< std::size_t >
							(iter - list.begin());
					})),
				make("image"_in, wrapped_type_ref_c< bitmap, 0 >),
				set_dimension_fn([](auto const& module){
					auto const optional_number = module("format"_param);
					std::size_t const number =
						[&module, &optional_number]()->std::size_t{
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
					parser_fn([](
						auto const& /*iop*/,
						std::string_view data,
						auto type
					){
						using io_tools::std_array::operator>>;
						std::istringstream is((std::string(data)));
						typename decltype(type)::type result;
						is >> result;
						stream_parser_t::verify_istream(data, is);
						return result;
					})),
				make("image"_out,
					wrapped_type_ref_c< transformed_bitmap, 0, 1 >)
			),
			module_init_fn([](auto& module){
				auto t = module.dimension(hana::size_c< 1 >);
				using pixel_type = typename decltype(t)::type;
				using matrix_type = calc_type< pixel_type >;

				auto source_points = module("source_points"_param);
				auto const source_size = module("source_size"_param);
				auto const target_size = module("target_size"_param);
				points_type< pixel_type > target_points{{
					{0, 0},
					{target_size.width(), 0},
					{0, target_size.height()},
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
			exec_fn([](auto& module){
				auto t0 = module.dimension(hana::size_c< 0 >);
				using source_pixel_type = typename decltype(t0)::type;
				auto t1 = module.dimension(hana::size_c< 1 >);
				using target_channel_type = typename decltype(t1)::type;
				using target_pixel_type = pixel::with_channel_type_t<
					source_pixel_type, target_channel_type >;
				using matrix_type = calc_type< target_channel_type >;
				auto const& state = module.state();
				for(auto const& img: module("image"_in).references()){
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