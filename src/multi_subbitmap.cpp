//-----------------------------------------------------------------------------
// Copyright (c) 2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <disposer/module.hpp>

#include <bitmap/subbitmap.hpp>

#include <range/v3/view.hpp>

#include <boost/dll.hpp>

#include <boost/spirit/home/x3.hpp>


namespace disposer_module::multi_subbitmap{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;
	using namespace hana::literals;

	namespace pixel = ::bmp::pixel;

	template < typename T >
	using bitmap = ::bmp::bitmap< T >;

	template < typename T >
	using bitmap_vector = ::bmp::bitmap_vector< T >;


	struct list_parser{
		std::vector< float > operator()(std::string_view value)const{
			namespace x3 = boost::spirit::x3;
			using x3::float_;
			using x3::phrase_parse;
			using x3::_attr;
			using x3::ascii::space;

			std::vector< float > list;
			auto push_back = [&list](auto& ctx){ list.push_back(_attr(ctx)); };

			auto first = value.begin();
			auto const last = value.end();
			bool const match =
				phrase_parse(first, last, float_[push_back] % ',', space);

			if(!match || first != last){
				throw std::logic_error("expected comma separated float list");
			}

			return list;
		}
	};


	template < typename Module, typename Bitmaps >
	auto exec(Module const module, Bitmaps const& images){
		auto const& xos = module("x_offsets"_param);
		auto const& yos = module("y_offsets"_param);
		auto const w = module("width"_param);
		auto const h = module("height"_param);

		if(xos.size() != images.size()){
			throw std::logic_error("wrong image count");
		}

		Bitmaps result;
		result.reserve(images.size());
		// TODO: use structured bindungs as soon as clang fixed BUG 34749
		for(auto const& tuple: ranges::view::zip(images, xos, yos)){
			auto const& image = std::get< 0 >(tuple);
			auto const& xo = std::get< 1 >(tuple);
			auto const& yo = std::get< 2 >(tuple);
			module.log([xo, yo](logsys::stdlogb& os){
				os << "x = " << xo << ", y = " << yo;
			}, [&]{
				result.push_back(subbitmap(image, ::bmp::rect{xo, yo, w, h}));
			});
		}

		return result;
	}

	void init(std::string const& name, declarant& disposer){
		auto init = generate_module(
			"subpixel subbitmap (via bilinear interpolation) of every bitmap "
			"in a vector of bitmaps with own x-y-offset for every bitmap, "
			"throw if subbitmap is out of range",
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
				>
			},
			module_configure(
				make("x_offsets"_param, free_type_c< std::vector< float > >,
					"comma separated list of float numbers as x offset "
					"for the corresponding bitmap in the vector",
					parser_fn< list_parser >(),
					verify_value_fn([](auto const& values){
						if(!values.empty()) return;
						throw std::logic_error("Need at least one x value");
					})),
				make("y_offsets"_param, free_type_c< std::vector< float > >,
					"comma separated list of float numbers as y offset "
					"for the corresponding bitmap in the vector "
					"(must have the same count of numbers as x_offset)",
					parser_fn< list_parser >(),
					verify_value_fn([](auto const& values, auto const module){
						auto const& x_offsets = module("x_offsets"_param);
						if(values.size() == x_offsets.size()) return;
						throw std::logic_error(
							"different element count as in x_offsets");
					})),
				make("width"_param, free_type_c< std::size_t >,
					"width of the target bitmaps (1 unsigned int value)"),
				make("height"_param, free_type_c< std::size_t >,
					"height of the target bitmaps (1 unsigned int value)"),
				make("images"_in, wrapped_type_ref_c< bitmap_vector, 0 >,
					"original bitmaps"),
				make("images"_out, wrapped_type_ref_c< bitmap_vector, 0 >,
					"target bitmaps")
			),
			exec_fn([](auto module){
				for(auto const& img: module("images"_in).references()){
					module("images"_out).push(exec(module, img));
				}
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
