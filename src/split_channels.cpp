//-----------------------------------------------------------------------------
// Copyright (c) 2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "bitmap_vector.hpp"

#include <bitmap/pixel.hpp>

#include <disposer/module.hpp>

#include <boost/hana.hpp>
#include <boost/dll.hpp>

#include <cstdint>
#include <iostream>

#include "thread_pool.hpp"


namespace disposer_module{ namespace split_channels{


	namespace hana = boost::hana;
	namespace pixel = ::bitmap::pixel;


	using disposer::type_position_v;

	using input_type_list = disposer::type_list<
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

	using output_type_list = disposer::type_list<
		std::int8_t,
		std::uint8_t,
		std::int16_t,
		std::uint16_t,
		std::int32_t,
		std::uint32_t,
		std::int64_t,
		std::uint64_t,
		float,
		double
	>;


	struct module: disposer::module_base{
		module(disposer::make_data const& data):
			disposer::module_base(
				data,
				{slots.image},
				{signals.image_vector}
			)
			{}

		struct{
			disposer::container_input< bitmap, input_type_list >
				image{"image"};
		} slots;

		struct{
			disposer::container_output< bitmap_vector, output_type_list >
				image_vector{"image_vector"};
		} signals;


		void exec()override;
		void input_ready()override;
	};


	disposer::module_ptr make_module(disposer::make_data& data){
		if(data.is_first()) throw disposer::module_not_as_start(data);

		return std::make_unique< module >(data);
	}


	enum class pixel_type{
		ga,
		rgb,
		rgba
	};

	template < typename T >
	constexpr pixel_type get_pixel_type(pixel::basic_ga< T > const&){
		return pixel_type::ga;
	}

	template < typename T >
	constexpr pixel_type get_pixel_type(pixel::basic_rgb< T > const&){
		return pixel_type::rgb;
	}

	template < typename T >
	constexpr pixel_type get_pixel_type(pixel::basic_rgba< T > const&){
		return pixel_type::rgba;
	}


	namespace{


		template < typename T >
		auto split_channels(bitmap< T > const& image){
			bitmap_vector< typename T::value_type > result(T::channel_count,
				bitmap< typename T::value_type >(image.size())
			);

			constexpr auto type = get_pixel_type(T());
			static_assert(
				type == pixel_type::ga ||
				type == pixel_type::rgb ||
				type == pixel_type::rgba);

			for(std::size_t y = 0; y < image.height(); ++y){
				for(std::size_t x = 0; x < image.width(); ++x){
					if constexpr(type == pixel_type::ga){
						result[0](x, y) = image(x, y).g;
						result[1](x, y) = image(x, y).a;
					}else if constexpr(type == pixel_type::rgb){
						result[0](x, y) = image(x, y).r;
						result[1](x, y) = image(x, y).g;
						result[2](x, y) = image(x, y).b;
					}else if constexpr(type == pixel_type::rgba){
						result[0](x, y) = image(x, y).r;
						result[1](x, y) = image(x, y).g;
						result[2](x, y) = image(x, y).b;
						result[3](x, y) = image(x, y).a;
					}
				}
			}

			return result;
		}


	}


	void module::exec(){
		for(auto const& [id, img]: slots.image.get()){
			(void)id;
			std::visit([this](auto const& img_input_data){
				using bitmap_type =
					std::decay_t< decltype(img_input_data.data()) >;
				using pixel_type = typename bitmap_type::value_type;
				using channel_type = typename pixel_type::value_type;

				signals.image_vector.put< channel_type >
					(split_channels(img_input_data.data()));
			}, img);
		}
	}

	void module::input_ready(){
		signals.image_vector.enable_types(
			slots.image.enabled_types_transformed(
				[](auto type){
					return hana::type_c< bitmap_vector< typename
						decltype(type)::type::value_type::value_type > >;
				}
			)
		);
	}


	void init(disposer::module_declarant& add){
		add("split_channels", &make_module);
	}

	BOOST_DLL_AUTO_ALIAS(init)


} }
