//-----------------------------------------------------------------------------
// Copyright (c) 2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "thread_pool.hpp"

#define DISPOSER_BITMAP_PIXEL_AS_TEXT
#include <disposer/module.hpp>

#include <bitmap/bitmap.hpp>
#include <bitmap/pixel.hpp>

#include <boost/dll.hpp>
#include <boost/spirit/home/x3.hpp>


namespace disposer_module::bitmap_vector_join{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;

	namespace pixel = ::bmp::pixel;

	template < typename T >
	using bitmap = ::bmp::bitmap< T >;


	namespace{


		template < typename T >
		auto get_parser(){
			static_assert(
				std::is_floating_point_v< T > ||
				std::is_unsigned_v< T > ||
				std::is_signed_v< T >
			);

			namespace x3 = boost::spirit::x3;

			if constexpr(std::is_floating_point_v< T >){
				return x3::double_;
			}else if constexpr(std::is_unsigned_v< T >){
				return x3::uint64;
			}else{
				return x3::int64;
			}
		}


		template < typename T, typename Value >
		T protected_cast(Value v){
			if constexpr(
				!std::is_floating_point_v< T > && !std::is_same_v< T, Value >
			){
				if(
					std::numeric_limits< T >::min() > v ||
					std::numeric_limits< T >::max() < v
				){
					std::ostringstream os;
					os << "value '" << v << "' does not match in type [" <<
						type_name< T >() << "]";
					throw std::out_of_range(os.str());
				}
			}

			return static_cast< T >(v);
		}


		struct value_parser{
			template < typename IOP_List, typename T >
			T operator()(
				std::string_view value,
				hana::basic_type< T > type,
				IOP_List const& module
			)const{
				return stream_parser_t{}(value, type, module);
			}

			template < typename IOP_List, typename T >
			pixel::basic_ga< T > operator()(
				std::string_view value,
				hana::basic_type< pixel::basic_ga< T > >,
				IOP_List const& /*module*/
			)const{
				namespace x3 = boost::spirit::x3;

				pixel::basic_ga< T > px;
				auto g = [&px](auto& ctx)
					{ px.g = protected_cast< T >(_attr(ctx)); };
				auto a = [&px](auto& ctx)
					{ px.a = protected_cast< T >(_attr(ctx)); };

				auto vparser = get_parser< T >();

				auto first = value.begin();
				auto const last = value.end();
				bool const match = x3::phrase_parse(first, last,
					(x3::lit('{')
						>> vparser[g] >> x3::lit(',')
						>> vparser[a] >> x3::lit('}')),
					x3::space);

				if(!match || first != last){
					throw std::logic_error(
						"expected '{gray, alpha}' but got '"
						+ std::string(value) + "'");
				}

				return px;
			}

			template < typename IOP_List, typename T >
			pixel::basic_rgb< T > operator()(
				std::string_view value,
				hana::basic_type< pixel::basic_rgb< T > >,
				IOP_List const& /*module*/
			)const{
				namespace x3 = boost::spirit::x3;

				pixel::basic_rgb< T > px;
				auto r = [&px](auto& ctx)
					{ px.r = protected_cast< T >(_attr(ctx)); };
				auto g = [&px](auto& ctx)
					{ px.g = protected_cast< T >(_attr(ctx)); };
				auto b = [&px](auto& ctx)
					{ px.b = protected_cast< T >(_attr(ctx)); };

				auto vparser = get_parser< T >();

				auto first = value.begin();
				auto const last = value.end();
				bool const match = x3::phrase_parse(first, last,
					(x3::lit('{')
						>> vparser[r] >> x3::lit(',')
						>> vparser[g] >> x3::lit(',')
						>> vparser[b] >> x3::lit('}')),
					x3::space);

				if(!match || first != last){
					throw std::logic_error(
						"expected '{red, green, blue}' but got '"
						+ std::string(value) + "'");
				}

				return px;
			}

			template < typename IOP_List, typename T >
			pixel::basic_rgba< T > operator()(
				std::string_view value,
				hana::basic_type< pixel::basic_rgba< T > >,
				IOP_List const& /*module*/
			)const{
				namespace x3 = boost::spirit::x3;

				pixel::basic_rgba< T > px;
				auto r = [&px](auto& ctx)
					{ px.r = protected_cast< T >(_attr(ctx)); };
				auto g = [&px](auto& ctx)
					{ px.g = protected_cast< T >(_attr(ctx)); };
				auto b = [&px](auto& ctx)
					{ px.b = protected_cast< T >(_attr(ctx)); };
				auto a = [&px](auto& ctx)
					{ px.a = protected_cast< T >(_attr(ctx)); };

				auto vparser = get_parser< T >();

				auto first = value.begin();
				auto const last = value.end();
				bool const match = x3::phrase_parse(first, last,
					(x3::lit('{')
						>> vparser[r] >> x3::lit(',')
						>> vparser[g] >> x3::lit(',')
						>> vparser[b] >> x3::lit(',')
						>> vparser[a] >> x3::lit('}')),
					x3::space);

				if(!match || first != last){
					throw std::logic_error(
						"expected '{red, green, blue, alpha}' but got '"
						+ std::string(value) + "'");
				}

				return px;
			}
		};


		enum class orientation{
			horizontal = 0, vertical = 1
		};


	}


	struct bitmap_join_t{
		template < typename Module, typename T >
		bitmap< T > operator()(
			Module const& module,
			bitmap< T > const& img1,
			bitmap< T > const& img2
		)const{
			auto const default_value = module("default_value"_param);

			if(module("orientation"_param) == orientation::horizontal){
				std::size_t width = img1.width() + img2.width();
				std::size_t height = std::max(img1.height(), img2.height());

				bitmap< T > result(width, height, default_value);

				for(std::size_t y = 0; y < img1.height(); ++y){
					auto const in_start = img1.data() + (y * img1.width());
					auto const in_end = in_start + img1.width();
					auto const out_start = result.data() + (y * result.width());

					std::copy(in_start, in_end, out_start);
				}

				for(std::size_t y = 0; y < img2.height(); ++y){
					auto const in_start = img2.data() + (y * img2.width());
					auto const in_end = in_start + img2.width();
					auto const out_start = result.data() +
						(y * result.width() + img1.width());

					std::copy(in_start, in_end, out_start);
				}

				return result;
			}else{
				std::size_t width = std::max(img1.width(), img2.width());
				std::size_t height = img1.height() + img2.height();

				bitmap< T > result(width, height, default_value);

				for(std::size_t y = 0; y < img1.height(); ++y){
					auto const in_start = img1.data() + (y * img1.width());
					auto const in_end = in_start + img1.width();
					auto const out_start = result.data() + (y * result.width());

					std::copy(in_start, in_end, out_start);
				}

				auto const offset = img1.point_count();
				for(std::size_t y = 0; y < img2.height(); ++y){
					auto const in_start = img2.data() + (y * img2.width());
					auto const in_end = in_start + img2.width();
					auto const out_start = result.data() + offset +
						(y * result.width());

					std::copy(in_start, in_end, out_start);
				}

				return result;			}
		}
	};

	constexpr auto bitmap_join = bitmap_join_t();


	void init(std::string const& name, module_declarant& disposer){
		auto init = generate_module(
			"joins two bitmaps of same data type to one bitmap",
			dimension_list{
				dimension_c<
					std::int8_t,
					std::uint8_t,
					std::int16_t,
					std::uint16_t,
					std::int32_t,
					std::uint32_t,
					std::int64_t,
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
				>
			},
			module_configure(
				make("orientation"_param, free_type_c< orientation >,
					"join bitmaps horizontal or vertical",
					parser_fn([](
						std::string_view data,
						hana::basic_type< orientation >,
						auto const /*module*/
					){
						constexpr std::array< std::string_view, 2 > list{{
								"horizontal", "vertical"
							}};
						auto iter = std::find(list.begin(), list.end(), data);
						if(iter == list.end()){
							std::ostringstream os;
							os << "unknown value '" << data
								<< "', valid values are: ";
							bool first = true;
							for(auto name: list){
								if(first){ first = false; }else{ os << ", "; }
								os << name;
							}
							throw std::runtime_error(os.str());
						}
						return orientation(iter - list.begin());
					}),
					default_value(orientation::horizontal)),
				make("image1"_in, wrapped_type_ref_c< bitmap, 0 >,
					"first bitmap"),
				make("image2"_in, wrapped_type_ref_c< bitmap, 0 >,
					"second bitmap"),
				make("image"_out, wrapped_type_ref_c< bitmap, 0 >,
					"joined bitmap with size(image1.width + image2.width, "
					"max(image1.height, image2.height)) for horizontal "
					"orientation and size(max(image1.width, image2.width), "
					"image1.height + image2.height) for vertical orientation"),
				make("default_value"_param, type_ref_c< 0 >,
					"if first and second bitmap don't have the same width "
					"(horizontal orientation) or height "
					"(vertical orientation), the remaining pixels in the "
					"joined bitmap are filled with this value",
					parser_fn(value_parser{}))
			),
			exec_fn([](auto module){
				auto imgs1 = module("image1"_in).references();
				auto imgs2 = module("image2"_in).references();

				if(imgs1.size() != imgs2.size()){
					throw std::runtime_error("inputs image1 and image2 must "
						"have the same count of images");
				}

				for(
					auto i1 = imgs1.begin(), i2 = imgs2.begin();
					i1 != imgs1.end();
					++i1, ++i2
				){
					module("image"_out).push(bitmap_join(module, *i1, *i2));
				};
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
