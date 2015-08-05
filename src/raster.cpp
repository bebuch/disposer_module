//-----------------------------------------------------------------------------
// Copyright (c) 2015 Benjamin Buch
//
// https://github.com/bebuch/disposer
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "raster.hpp"

#include "camera_sequence.hpp"
#include "make_string.hpp"

#include <disposer/module_base.hpp>

#include <cstdint>


namespace disposer_module{ namespace raster{


	struct parameter{
		std::size_t raster;
	};

	template < typename T >
	struct module: disposer::module_base{
		module(std::string const& type, std::string const& chain, std::string const& name, parameter&& param):
			disposer::module_base(type, chain, name),
			param(std::move(param)){
				inputs = disposer::make_input_list(slots.camera_sequence, slots.sequence, slots.image);
				outputs = disposer::make_output_list(signals.camera_sequence, signals.sequence, signals.image);
			}

		bitmap< T > apply_raster(bitmap< T > const& image)const;

		struct{
			disposer::module_input< camera_pointer_sequence< T > > camera_sequence{"camera_sequence"};
			disposer::module_input< bitmap_pointer_sequence< T > > sequence{"sequence"};
			disposer::module_input< bitmap< T > > image{"image"};
		} slots;

		struct{
			disposer::module_output< camera_pointer_sequence< T > > camera_sequence{"camera_sequence"};
			disposer::module_output< bitmap_pointer_sequence< T > > sequence{"sequence"};
			disposer::module_output< bitmap< T > > image{"image"};
		} signals;

		void trigger(std::size_t id)override;

		parameter const param;
	};

	template < typename T >
	disposer::module_ptr make_module(
		std::string const& type,
		std::string const& chain,
		std::string const& name,
		disposer::io_list const&,
		disposer::io_list const&,
		disposer::parameter_processor& params,
		bool is_start
	){
		if(is_start) throw disposer::module_not_as_start(type, chain);

		parameter param;

		params.set(param.raster, "raster");

		if(param.raster == 0){
			throw std::logic_error(make_string(type + ": raster (value: ", param.raster, ") needs to be greater than 0"));
		}

		return std::make_unique< module< T > >(type, chain, name, std::move(param));
	}

	void init(){
		add_module_maker("raster_int8_t", &make_module< std::int8_t >);
		add_module_maker("raster_uint8_t", &make_module< std::uint8_t >);
		add_module_maker("raster_int16_t", &make_module< std::int16_t >);
		add_module_maker("raster_uint16_t", &make_module< std::uint16_t >);
		add_module_maker("raster_int32_t", &make_module< std::int32_t >);
		add_module_maker("raster_uint32_t", &make_module< std::uint32_t >);
		add_module_maker("raster_int64_t", &make_module< std::int64_t >);
		add_module_maker("raster_uint64_t", &make_module< std::uint64_t >);
		add_module_maker("raster_float", &make_module< float >);
		add_module_maker("raster_double", &make_module< double >);
		add_module_maker("raster_long_double", &make_module< long double >);
	}


	template < typename T >
	bitmap< T > module< T >::apply_raster(bitmap< T > const& image)const{
		bitmap< T > result((image.width() - 1) / param.raster + 1, (image.height() - 1) / param.raster + 1);
		for(std::size_t y = 0; y < result.height(); ++y){
			for(std::size_t x = 0; x < result.width(); ++x){
				result(x, y) = image(x * param.raster, y * param.raster);
			}
		}
		return result;
	}


	template < typename T >
	void module< T >::trigger(std::size_t id){
		for(auto const& pair: slots.camera_sequence.get(id)){
			auto id = pair.first;
			auto& data = pair.second.data();

			camera_pointer_sequence< T > result(data.size());
			for(std::size_t i = 0; i < data.size(); ++i){
				result[i] = std::make_shared< bitmap_pointer_sequence< T > >((*data[i]).size());
				for(std::size_t j = 0; j < (*data[i]).size(); ++j){
					(*result[i])[j] = std::make_shared< bitmap< T > >(apply_raster(*(*data[i])[j]));
				}
			}

			signals.camera_sequence.put(id, std::move(result));
		}

		for(auto const& pair: slots.sequence.get(id)){
			auto id = pair.first;
			auto& data = pair.second.data();

			bitmap_pointer_sequence< T > result(data.size());
			for(std::size_t i = 0; i < data.size(); ++i){
				result[i] = std::make_shared< bitmap< T > >(apply_raster(*data[i]));
			}

			signals.sequence.put(id, std::move(result));
		}

		for(auto const& pair: slots.image.get(id)){
			auto id = pair.first;
			auto& data = pair.second.data();

			signals.image.put(id, apply_raster(data));
		}
	}


} }
