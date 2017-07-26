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

	namespace pixel = ::bmp::pixel;

	template < typename T >
	using bitmap = ::bmp::bitmap< T >;

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


	struct demosaic_t{
		template < typename Module, typename T >
		auto operator()(Module const& module, bitmap< T > const& image)const{
			auto const xc = module("x_count"_param).get();
			auto const yc = module("y_count"_param).get();

			if(image.width() % xc){
				throw std::logic_error(
					"image width is not divisible by parameter x_count");
			}

			if(image.height() % yc){
				throw std::logic_error(
					"image height is not divisible by parameter y_count");
			}

			std::size_t width = image.width() / xc;
			std::size_t height = image.height() / yc;
			std::size_t image_count = xc * yc;

			bitmap_vector< T > result;
			result.reserve(image_count);

			thread_pool pool;

			std::mutex mutex;
			pool(0, image_count,
				[&result, &mutex, width, height](std::size_t){
					auto image = bitmap< T >(width, height);

					// emplace_back is not thread safe
					std::lock_guard< std::mutex > lock(mutex);
					result.emplace_back(std::move(image));
				});

			pool(0, height,
				[&result, &image, xc, yc, width](std::size_t y){
					for(std::size_t iy = 0; iy < yc; ++iy){
						for(std::size_t x = 0; x < width; ++x){
							for(std::size_t ix = 0; ix < xc; ++ix){
								result[iy * xc + ix](x, y) =
									image(x * xc + ix, y * yc + iy);
							}
						}
					}
				});

			return result;
		}
	};

	constexpr auto demosaic = demosaic_t();


	void init(std::string const& name, module_declarant& disposer){
		auto init = module_register_fn(
			module_configure(
				"image"_in(types, template_transform_c< bitmap >),
				"images"_out(types,
					type_transform([](auto type)noexcept{
						return hana::type_c< std::vector< bitmap<
							typename decltype(type)::type > > >;
					}),
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
				)
			),
			module_enable([]{
				return [](auto& module){
					auto values = module("image"_in).get_references();
					for(auto const& value: values){
						std::visit([&module](auto const& img_ref){
							module("images"_out).put(
								demosaic(module, img_ref.get()));
						}, value);
					}
				};
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
