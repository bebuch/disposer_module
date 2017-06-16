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

	namespace pixel = ::bitmap::pixel;

	template < typename T >
	using bitmap = ::bitmap::bitmap< T >;

	template < typename T >
	using bitmap_vector = std::vector< bitmap< T > >;


	constexpr auto types = hana::tuple_t<
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
		>;


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
				IOP_List const& iop,
				std::string_view value,
				hana::basic_type< T > type
			)const{
				return stream_parser{}(iop, value, type);
			}

			template < typename IOP_List, typename T >
			pixel::basic_ga< T > operator()(
				IOP_List const& /*iop*/,
				std::string_view value,
				hana::basic_type< pixel::basic_ga< T > >
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
				IOP_List const& /*iop*/,
				std::string_view value,
				hana::basic_type< pixel::basic_rgb< T > >
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
				IOP_List const& /*iop*/,
				std::string_view value,
				hana::basic_type< pixel::basic_rgba< T > >
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


	}


	struct bitmap_vector_join_t{
		template < typename Module, typename T >
		bitmap< T > operator()(
			Module const& module,
			bitmap_vector< T > const& vectors
		)const{
			auto const ips = module("images_per_line"_param).get();
			auto const default_value =
				module("default_value"_param).get(hana::type_c< T >);
			auto const& input_size = vectors[0].size();

			std::size_t width =
				std::min(ips, vectors.size()) * input_size.width();
			std::size_t height =
				((vectors.size() - 1) / ips + 1) * input_size.height();

			bitmap< T > result(width, height, default_value);

			// parallel version with thread_pool is slower
			for(std::size_t i = 0; i < vectors.size(); ++i){
				auto const vwidth = vectors[i].width();
				auto const x_offset = (i % ips) * input_size.width();
				auto const y_offset = (i / ips) * input_size.height();

				for(std::size_t y = 0; y < input_size.height(); ++y){
					auto const in_start = vectors[i].data() + (y * vwidth);
					auto const in_end = in_start + vwidth;
					auto const out_start =
						result.data() + (y_offset + y) * width + x_offset;

					std::copy(in_start, in_end, out_start);
				}
			}

			return result;
		}
	};

	constexpr auto bitmap_vector_join = bitmap_vector_join_t();


	void init(std::string const& name, module_declarant& disposer){
		auto init = module_register_fn(
			module_configure(
				"images"_in(types,
					type_transform([](auto type)noexcept{
						return hana::type_c< std::vector< bitmap<
							typename decltype(type)::type > > >;
					}),
					required),
				"image"_out(types,
					template_transform_c< bitmap >,
					enable_by_types_of("images"_in)
				),
				"images_per_line"_param(hana::type_c< std::size_t >,
					value_verify([](auto const& /*iop*/, auto const& value){
						if(value > 0) return;
						throw std::logic_error("must be greater 0");
					})
				),
				"default_value"_param(types,
					parser(value_parser()),
					enable_by_types_of("images"_in))
			),
			normal_id_increase(),
			module_enable([]{
				return [](auto& module){
					auto values = module("images"_in).get_references();
					for(auto const& pair: values){
						std::visit([&module](auto const& imgs_ref){
							module("image"_out).put(
								bitmap_vector_join(module, imgs_ref.get()));
						}, pair.second);
					}
				};
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
