//-----------------------------------------------------------------------------
// Copyright (c) 2015-2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "bitmap_sequence.hpp"
#include "make_string.hpp"

#include <disposer/module.hpp>

#include <bitmap/pixel.hpp>

#include <boost/dll.hpp>

#include <cstdint>


namespace disposer_module{ namespace raster{


	namespace pixel = ::bitmap::pixel;

	using type_list = disposer::type_list<
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
		long double,
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



	struct parameter{
		std::size_t raster;
	};


	struct module: disposer::module_base{
		module(disposer::make_data const& data, parameter&& param):
			disposer::module_base(
				data,
				{slots.sequence, slots.vector, slots.image},
				{signals.sequence, signals.vector, signals.image}
			),
			param(std::move(param))
			{}


		template < typename T >
		bitmap< T > apply_raster(bitmap< T > const& image)const;


		struct{
			disposer::container_input< bitmap_sequence, type_list >
				sequence{"sequence"};

			disposer::container_input< bitmap_vector, type_list >
				vector{"vector"};

			disposer::container_input< bitmap, type_list >
				image{"image"};
		} slots;

		struct{
			disposer::container_output< bitmap_sequence, type_list >
				sequence{"sequence"};

			disposer::container_output< bitmap_vector, type_list >
				vector{"vector"};

			disposer::container_output< bitmap, type_list >
				image{"image"};
		} signals;


		void exec()override;


		void input_ready()override{
			signals.sequence.enable_types(slots.sequence.active_types());
			signals.vector.enable_types(slots.vector.active_types());
			signals.image.enable_types(slots.image.active_types());
		}


		parameter const param;
	};


	disposer::module_ptr make_module(disposer::make_data& data){
		if(data.is_first()) throw disposer::module_not_as_start(data);

		parameter param;

		data.params.set(param.raster, "raster");

		if(param.raster == 0){
			throw std::logic_error(make_string(
				"raster (value: ", param.raster, ") needs to be greater than 0"
			));
		}

		return std::make_unique< module >(data, std::move(param));
	}


	template < typename T >
	bitmap< T > module::apply_raster(bitmap< T > const& image)const{
		bitmap< T > result(
			(image.width() - 1) / param.raster + 1,
			(image.height() - 1) / param.raster + 1
		);

		for(std::size_t y = 0; y < result.height(); ++y){
			for(std::size_t x = 0; x < result.width(); ++x){
				result(x, y) = image(x * param.raster, y * param.raster);
			}
		}

		return result;
	}


	void module::exec(){
		for(auto const& [id, seq]: slots.sequence.get()){
			(void)id;
			std::visit([this](auto const& data){
				auto const& seq = data.data();

				using sequence_t = std::decay_t< decltype(seq) >;
				using value_type =
					typename sequence_t::value_type::value_type::value_type;

				sequence_t result(seq.size());
				for(std::size_t i = 0; i < seq.size(); ++i){
					result[i].resize(seq[i].size());
					for(std::size_t j = 0; j < seq[i].size(); ++j){
						result[i][j] = apply_raster(seq[i][j]);
					}
				}

				signals.sequence.put< value_type >(std::move(result));
			}, seq);
		}

		for(auto const& [id, vec]: slots.vector.get()){
			(void)id;
			std::visit([this](auto const& data){
				auto const& vec = data.data();

				using vector_t = std::decay_t< decltype(vec) >;
				using value_type = typename vector_t::value_type::value_type;

				vector_t result(vec.size());
				for(std::size_t i = 0; i < vec.size(); ++i){
					result[i] = apply_raster(vec[i]);
				}

				signals.vector.put< value_type >(std::move(result));
			}, vec);
		}

		for(auto const& [id, img]: slots.image.get()){
			(void)id;
			std::visit([this](auto const& data){
				auto const& img = data.data();

				using value_type =
					typename std::decay_t< decltype(img) >::value_type;

				signals.image.put< value_type >(apply_raster(img));
			}, img);
		}
	}


	void init(disposer::module_declarant& add){
		add("raster", &make_module);
	}

	BOOST_DLL_AUTO_ALIAS(init)


} }
