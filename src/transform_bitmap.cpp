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
#include <bitmap/point_io.hpp>
#include <bitmap/size_io.hpp>

#include <io_tools/std_array_io.hpp>

#include <boost/dll.hpp>


namespace disposer_module::transform_bitmap{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;

	namespace pixel = ::bmp::pixel;

	template < typename T >
	using bitmap = ::bmp::bitmap< T >;

	template < typename T >
	using transformed_bitmap = bitmap< std::conditional_t<
		std::numeric_limits< T >::has_quiet_NaN,
		T, pixel::basic_masked_pixel< T > > >;


	struct state{
		bmp::matrix3x3< float > const homography;
		bmp::rect< long, long, std::size_t, std::size_t > const contour;
	};

	void init(std::string const& name, module_declarant& disposer){
		auto init = module_register_fn(
			dimension_list{
				dimension_c<
// 					std::int8_t,
					std::uint8_t,
// 					std::int16_t,
					std::uint16_t/*,*/
// 					std::int32_t,
// 					std::uint32_t,
// 					std::int64_t,
// 					std::uint64_t,
// 					float,
// 					double,
// 					pixel::ga8,
// 					pixel::ga16,
// 					pixel::ga32,
// 					pixel::ga64,
// 					pixel::ga8u,
// 					pixel::ga16u,
// 					pixel::ga32u,
// 					pixel::ga64u,
// 					pixel::ga32f,
// 					pixel::ga64f,
// 					pixel::rgb8,
// 					pixel::rgb16,
// 					pixel::rgb32,
// 					pixel::rgb64,
// 					pixel::rgb8u,
// 					pixel::rgb16u,
// 					pixel::rgb32u,
// 					pixel::rgb64u,
// 					pixel::rgb32f,
// 					pixel::rgb64f,
// 					pixel::rgba8,
// 					pixel::rgba16,
// 					pixel::rgba32,
// 					pixel::rgba64,
// 					pixel::rgba8u,
// 					pixel::rgba16u,
// 					pixel::rgba32u,
// 					pixel::rgba64u,
// 					pixel::rgba32f,
// 					pixel::rgba64f
				>
			},
			module_configure(
				make("source_points"_param,
					free_type_c< std::array< bmp::point< float >, 4 > >,
					parser_fn([](
						auto const& /*iop*/,
						std::string_view data,
						hana::basic_type< std::array< bmp::point< float >, 4 > >
					){
						using io_tools::std_array::operator>>;
						std::istringstream is((std::string(data)));
						std::array< bmp::point< float >, 4 > result;
						is >> result;
						stream_parser_t::verify_istream(data, is);
						return result;
					})),
				make("source_size"_param,
					free_type_c< bmp::size< std::size_t > >),
				make("target_size"_param,
					free_type_c< bmp::size< std::size_t > >),
				make("image"_in, wrapped_type_ref_c< bitmap, 0 >),
// 				make("image"_out, wrapped_type_ref_c< transformed_bitmap, 0 >)
				make("image"_out, free_type_c< bitmap< float > >)
			),
			module_init_fn([](auto& module){
				auto source_points = module("source_points"_param);
				auto const source_size = module("source_size"_param);
				auto const target_size = module("target_size"_param);
				std::array< bmp::point< float >, 4 > target_points{{
					{0, 0},
					{target_size.width(), 0},
					{0, target_size.height()},
					bmp::to_point< float >(target_size)
				}};

				auto const homography = bmp::rect_transform_homography(
					source_points, target_points);

				auto const target_rect = transform_image_contour(
					homography, source_size);

				auto const target_contour = bmp::image_contour(target_rect);

				return state{invert(homography), target_contour};

			}),
			exec_fn([](auto& module){
				auto const& state = module.state();
				for(auto const& img: module("image"_in).references()){
					module("image"_out).push(bmp::transform_bitmap< float >(
						state.homography, img, state.contour));
				}
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
