//-----------------------------------------------------------------------------
// Copyright (c) 2015 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "bitmap_sequence.hpp"
#include "make_string.hpp"

#include <disposer/module.hpp>

#include <boost/dll.hpp>

#include <cstdint>


namespace disposer_module{ namespace raster{


	struct parameter{
		std::size_t raster;
	};

	template < typename T >
	struct module: disposer::module_base{
		module(disposer::make_data const& data, parameter&& param):
			disposer::module_base(
				data,
				{slots.sequence, slots.vector, slots.image},
				{signals.sequence, signals.vector, signals.image}
			),
			param(std::move(param))
			{}


		bitmap< T > apply_raster(bitmap< T > const& image)const;


		struct{
			disposer::input< bitmap_sequence< T > > sequence{"sequence"};
			disposer::input< bitmap_vector< T > > vector{"vector"};
			disposer::input< bitmap< T > > image{"image"};
		} slots;

		struct{
			disposer::output< bitmap_sequence< T > > sequence{"sequence"};
			disposer::output< bitmap_vector< T > > vector{"vector"};
			disposer::output< bitmap< T > > image{"image"};
		} signals;


		void trigger()override;


		parameter const param;
	};

	template < typename T >
	disposer::module_ptr make_module(disposer::make_data& data){
		if(data.is_first()) throw disposer::module_not_as_start(data);

		parameter param;

		data.params.set(param.raster, "raster");

		if(param.raster == 0){
			throw std::logic_error(make_string(data.type_name + ": raster (value: ", param.raster, ") needs to be greater than 0"));
		}

		return std::make_unique< module< T > >(data, std::move(param));
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
	void module< T >::trigger(){
		for(auto const& pair: slots.sequence.get(id)){
			auto id = pair.first;
			auto& data = pair.second.data();

			bitmap_sequence< T > result(data.size());
			for(std::size_t i = 0; i < data.size(); ++i){
				result[i].resize(data[i].size());
				for(std::size_t j = 0; j < data[i].size(); ++j){
					result[i][j] = apply_raster(data[i][j]);
				}
			}

			signals.sequence.put(id, std::move(result));
		}

		for(auto const& pair: slots.vector.get(id)){
			auto id = pair.first;
			auto& data = pair.second.data();

			bitmap_vector< T > result(data.size());
			for(std::size_t i = 0; i < data.size(); ++i){
				result[i] = apply_raster(data[i]);
			}

			signals.vector.put(id, std::move(result));
		}

		for(auto const& pair: slots.image.get(id)){
			auto id = pair.first;
			auto& data = pair.second.data();

			signals.image.put(id, apply_raster(data));
		}
	}


	void init(disposer::disposer& disposer){
		disposer.add_module_maker("raster_int8_t", &make_module< std::int8_t >);
		disposer.add_module_maker("raster_uint8_t", &make_module< std::uint8_t >);
		disposer.add_module_maker("raster_int16_t", &make_module< std::int16_t >);
		disposer.add_module_maker("raster_uint16_t", &make_module< std::uint16_t >);
		disposer.add_module_maker("raster_int32_t", &make_module< std::int32_t >);
		disposer.add_module_maker("raster_uint32_t", &make_module< std::uint32_t >);
		disposer.add_module_maker("raster_int64_t", &make_module< std::int64_t >);
		disposer.add_module_maker("raster_uint64_t", &make_module< std::uint64_t >);
		disposer.add_module_maker("raster_float", &make_module< float >);
		disposer.add_module_maker("raster_double", &make_module< double >);
		disposer.add_module_maker("raster_long_double", &make_module< long double >);
	}

	BOOST_DLL_AUTO_ALIAS(init)


} }
