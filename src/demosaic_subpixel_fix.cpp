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

#include <boost/dll.hpp>


namespace disposer_module::demosaic_subpixel_fix{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;

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
				)
			),
			module_enable([]{
				return [](auto& module){
					auto values = module("images"_in).get_references();
					for(auto const& value: values){
						std::visit([&module](auto const& imgs_ref){
							auto const& images = imgs_ref.get();
							auto const xc = module("x_count"_param).get();
							auto const yc = module("y_count"_param).get();

							if(xc * yc != images.size()){
								throw std::logic_error("wrong image count");
							}

							using type = std::decay_t< decltype(images) >;
							type result(images.size());
							for(std::size_t y = 0; y < yc; ++y){
								for(std::size_t x = 0; x < xc; ++x){
									auto sub_x = static_cast< float >(x) / xc;
									auto sub_y = static_cast< float >(y) / yc;
									module.log([sub_x, sub_y]
										(logsys::stdlogb& os){
											os << "x = " << sub_x << ", y = "
												<< sub_y;
										},
									[&]{
										auto const pos = x * yc + y;
										auto const& image = images[pos];
										result[pos] = ::bitmap::subbitmap(image,
											::bitmap::rect{
												sub_x, sub_y,
												image.width() - 1,
												image.height() - 1
											});
									});
								}
							}
							module("images"_out).put(std::move(result));
						}, value);
					}
				};
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
