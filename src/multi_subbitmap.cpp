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

	namespace pixel = ::bitmap::pixel;

	template < typename T >
	using bitmap = ::bitmap::bitmap< T >;

	template < typename T >
	using bitmap_vector = ::bitmap::bitmap_vector< T >;


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


	struct list_parser{
		template < typename IOP_List >
		std::vector< float > operator()(
			IOP_List const& /*iop*/,
			std::string_view value,
			hana::basic_type< std::vector< float > >
		)const{
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
	auto exec(Module const& module, Bitmaps const& images){
		auto const& xos = module("x_offsets"_param).get();
		auto const& yos = module("y_offsets"_param).get();
		auto const w = module("width"_param).get();
		auto const h = module("height"_param).get();

		if(xos.size() != images.size()){
			throw std::logic_error("wrong image count");
		}

		Bitmaps result;
		result.reserve(images.size());
		for(auto const& [image, xo, yo]: ranges::view::zip(images, xos, yos)){
			module.log([xo, yo](logsys::stdlogb& os){
				os << "x = " << xo << ", y = " << yo;
			}, [&]{
				result.push_back(
					subbitmap(image, ::bitmap::rect{xo, yo, w, h}));
			});
		}

		return result;
	}

	constexpr auto as_text = type_as_text
		(hana::make_pair(hana::type_c< std::vector< float > >,
			"float32_list"_s));


	void init(std::string const& name, module_declarant& disposer){
		auto init = module_register_fn(
			module_configure(
				"images"_in(types,
					template_transform_c< bitmap_vector >,
					required),
				"images"_out(types,
					template_transform_c< bitmap_vector >,
					enable_by_types_of("images"_in)
				),
				"x_offsets"_param(hana::type_c< std::vector< float > >,
					parser_fn< list_parser >(),
					value_verify([](auto const& /*iop*/, auto const& values){
						if(!values.empty()) return;
						throw std::logic_error("Need at least one x value");
					}),
					as_text
				),
				"y_offsets"_param(hana::type_c< std::vector< float > >,
					parser_fn< list_parser >(),
					value_verify([](auto const& iop, auto const& values){
						auto const& x_offsets = iop("x_offsets"_param).get();
						if(values.size() == x_offsets.size()) return;
						throw std::logic_error(
							"different element count as in x_offsets");
					}),
					as_text
				),
				"width"_param(hana::type_c< std::size_t >),
				"height"_param(hana::type_c< std::size_t >)
			),
			module_enable([]{
				return [](auto& module){
					auto values = module("images"_in).get_references();
					for(auto const& value: values){
						std::visit([&module](auto const& imgs_ref){
							module("images"_out)
								.put(exec(module, imgs_ref.get()));
						}, value);
					}
				};
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
