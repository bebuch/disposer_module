//-----------------------------------------------------------------------------
// Copyright (c) 2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "thread_pool.hpp"

#include <disposer/module.hpp>

#include <bitmap/bitmap.hpp>
#include <bitmap/pixel.hpp>

#include <boost/dll.hpp>


namespace disposer_module::demosaic{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;

	namespace pixel = ::bitmap::pixel;

	template < typename T >
	using bitmap = ::bitmap::bitmap< T >;


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


	template < typename Module, typename T >
	auto exec(Module const& module, bitmap< T > const& image){
		auto const xo = module("x_offset"_param).get();
		auto const yo = module("y_offset"_param).get();
		auto const xc = module("x_count"_param).get();
		auto const yc = module("y_count"_param).get();

		bitmap< T > result(
			(image.width() - xo - 1) / xc + 1,
			(image.height() - yo - 1) / yc + 1
		);

		thread_pool pool;
		pool(0, result.height(),
			[&result, &image, xc, yc, xo, yo](std::size_t y){
				for(std::size_t x = 0; x < result.width(); ++x){
					result(x, y) = image(xo + x * xc, yo + y * yc);
				}
			});

		return result;
	}


	void init(std::string const& name, module_declarant& disposer){
		auto init = module_register_fn(
			module_configure(
				"image"_in(types,
					template_transform_c< bitmap >,
					required),
				"image"_out(types,
					template_transform_c< bitmap >,
					enable_by_types_of("image"_in)
				),
				"x_count"_param(hana::type_c< std::size_t >,
					value_verify([](auto const& /*iop*/, auto const& value){
						if(value > 0) return;
						throw std::logic_error("must be greater 0");
					})
				),
				"y_count"_param(hana::type_c< std::size_t >,
					value_verify([](auto const& /*iop*/, auto const& value){
						if(value > 0) return;
						throw std::logic_error("must be greater 0");
					})
				),
				"x_offset"_param(hana::type_c< std::size_t >,
					value_verify([](auto const& iop, auto const& value){
						if(value < iop("x_count"_param).get()) return;
						throw std::logic_error("must be lesser x_count");
					})
				),
				"y_offset"_param(hana::type_c< std::size_t >,
					value_verify([](auto const& iop, auto const& value){
						if(value < iop("y_count"_param).get()) return;
						throw std::logic_error("must be lesser y_count");
					})
				)
			),
			module_enable([]{
				return [](auto& module){
					auto values = module("image"_in).get_references();
					for(auto const& value: values){
						std::visit([&module](auto const& img_ref){
							module("image"_out).put(
								exec(module, img_ref.get()));
						}, value);
					}
				};
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
