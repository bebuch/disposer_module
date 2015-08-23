//-----------------------------------------------------------------------------
// Copyright (c) 2015 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "subbitmap.hpp"

#include "bitmap_sequence.hpp"
#include "make_string.hpp"

#include <disposer/io.hpp>

#include <cstdint>
#include <limits>

#include <boost/dll.hpp>


namespace disposer_module{ namespace subbitmap{


	template < typename T >
	struct parameter{
		std::int32_t x;
		std::int32_t y;

		std::size_t width;
		std::size_t height;

		T default_value;
	};

	template < typename T >
	struct module: disposer::module_base{
		module(disposer::make_data const& data, parameter< T >&& param):
			disposer::module_base(data),
			param(std::move(param)){
				inputs = disposer::make_input_list(slots.sequence, slots.vector, slots.image);
				outputs = disposer::make_output_list(signals.sequence, signals.vector, signals.image);
			}

		bitmap< T > subbitmap(bitmap< T > const& image)const;

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

		parameter< T > const param;
	};

	template < typename T >
	disposer::module_ptr make_module(disposer::make_data& data){
		if(data.is_first()) throw disposer::module_not_as_start(data);

		parameter< T > param;

		data.params.set(param.x, "x");
		data.params.set(param.y, "y");

		data.params.set(param.width, "width");
		data.params.set(param.height, "height");

		// for integral is NaN defined as 0
		data.params.set(param.default_value, "default_value", std::numeric_limits< T >::quiet_NaN());

		return std::make_unique< module< T > >(data, std::move(param));
	}


	template < typename T >
	bitmap< T > module< T >::subbitmap(bitmap< T > const& image)const{
		bitmap< T > result(param.width, param.height, param.default_value);

		std::size_t const bx = param.x < 0 ? static_cast< std::size_t >(-param.x) : 0;
		std::size_t const by = param.y < 0 ? static_cast< std::size_t >(-param.y) : 0;
		std::size_t const ex =
			static_cast< std::int64_t >(param.width) < static_cast< std::int64_t >(image.width()) - param.x ?
				param.width :
				(static_cast< std::int32_t >(image.width()) > param.x ? image.width() - param.x : 0);
		std::size_t const ey =
			static_cast< std::int64_t >(param.height) < static_cast< std::int64_t >(image.height()) - param.y ?
				param.height :
				(static_cast< std::int32_t >(image.height()) > param.y ? image.height() - param.y : 0);

		// line wise copy
		for(std::size_t y = by; y < ey; ++y){
			std::copy(
				&image(bx + param.x, y + param.y),
				&image(ex + param.x, y + param.y),
				&result(bx, y)
			);
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
					result[i][j] = subbitmap(data[i][j]);
				}
			}

			signals.sequence.put(id, std::move(result));
		}

		for(auto const& pair: slots.vector.get(id)){
			auto id = pair.first;
			auto& data = pair.second.data();

			bitmap_vector< T > result(data.size());
			for(std::size_t i = 0; i < data.size(); ++i){
				result[i] = subbitmap(data[i]);
			}

			signals.vector.put(id, std::move(result));
		}

		for(auto const& pair: slots.image.get(id)){
			auto id = pair.first;
			auto& data = pair.second.data();

			signals.image.put(id, subbitmap(data));
		}
	}


	void init(disposer::disposer& disposer){
		disposer.add_module_maker("subbitmap_int8_t", &make_module< std::int8_t >);
		disposer.add_module_maker("subbitmap_uint8_t", &make_module< std::uint8_t >);
		disposer.add_module_maker("subbitmap_int16_t", &make_module< std::int16_t >);
		disposer.add_module_maker("subbitmap_uint16_t", &make_module< std::uint16_t >);
		disposer.add_module_maker("subbitmap_int32_t", &make_module< std::int32_t >);
		disposer.add_module_maker("subbitmap_uint32_t", &make_module< std::uint32_t >);
		disposer.add_module_maker("subbitmap_int64_t", &make_module< std::int64_t >);
		disposer.add_module_maker("subbitmap_uint64_t", &make_module< std::uint64_t >);
		disposer.add_module_maker("subbitmap_float", &make_module< float >);
		disposer.add_module_maker("subbitmap_double", &make_module< double >);
		disposer.add_module_maker("subbitmap_long_double", &make_module< long double >);
	}

	BOOST_DLL_AUTO_ALIAS(init)


} }
