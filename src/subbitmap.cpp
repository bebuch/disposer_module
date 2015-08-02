//-----------------------------------------------------------------------------
// Copyright (c) 2015 Benjamin Buch
//
// https://github.com/bebuch/disposer
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "subbitmap.hpp"

#include "camera_sequence.hpp"
#include "make_string.hpp"

#include <disposer/module_base.hpp>

#include <cstdint>
#include <limits>


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
		module(std::string const& type, std::string const& chain, std::string const& name, parameter< T >&& param):
			disposer::module_base(type, chain, name),
			param(std::move(param)){
				inputs = disposer::make_input_list(slots.camera_sequence, slots.sequence, slots.image);
				outputs = disposer::make_output_list(signals.camera_sequence, signals.sequence, signals.image);
			}

		std::shared_ptr< bitmap< T > > subbitmap(bitmap< T > const& image)const;

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

		parameter< T > const param;
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

		parameter< T > param;

		params.set(param.x, "x");
		params.set(param.y, "y");

		params.set(param.width, "width");
		params.set(param.height, "height");

		// for integral is NaN defined as 0
		params.set(param.default_value, "default_value", std::numeric_limits< T >::quiet_NaN());

		return std::make_unique< module< T > >(type, chain, name, std::move(param));
	}

	void init(){
		add_module_maker("subbitmap_int8_t", &make_module< std::int8_t >);
		add_module_maker("subbitmap_uint8_t", &make_module< std::uint8_t >);
		add_module_maker("subbitmap_int16_t", &make_module< std::int16_t >);
		add_module_maker("subbitmap_uint16_t", &make_module< std::uint16_t >);
		add_module_maker("subbitmap_int32_t", &make_module< std::int32_t >);
		add_module_maker("subbitmap_uint32_t", &make_module< std::uint32_t >);
		add_module_maker("subbitmap_int64_t", &make_module< std::int64_t >);
		add_module_maker("subbitmap_uint64_t", &make_module< std::uint64_t >);
		add_module_maker("subbitmap_float", &make_module< float >);
		add_module_maker("subbitmap_double", &make_module< double >);
		add_module_maker("subbitmap_long_double", &make_module< long double >);
	}


	template < typename T >
	std::shared_ptr< bitmap< T > > module< T >::subbitmap(bitmap< T > const& image)const{
		auto result = std::make_shared< bitmap< T > >(param.width, param.height, param.default_value);

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
				&(*result)(bx, y)
			);
		}

		return result;
	}


	template < typename T >
	void module< T >::trigger(std::size_t id){
		for(auto const& pair: slots.camera_sequence.get(id)){
			auto id = pair.first;
			auto& data = pair.second.data();

			auto result = std::make_shared< camera_pointer_sequence< T > >(data.size());
			for(std::size_t i = 0; i < data.size(); ++i){
				(*result)[i] = std::make_shared< bitmap_pointer_sequence< T > >((*data[i]).size());
				for(std::size_t j = 0; j < (*data[i]).size(); ++j){
					(*(*result)[i])[j] = subbitmap(*(*data[i])[j]);
				}
			}

			signals.camera_sequence(id, std::move(result));
		}

		for(auto const& pair: slots.sequence.get(id)){
			auto id = pair.first;
			auto& data = pair.second.data();

			auto result = std::make_shared< bitmap_pointer_sequence< T > >(data.size());
			for(std::size_t i = 0; i < data.size(); ++i){
				(*result)[i] = subbitmap(*data[i]);
			}

			signals.sequence(id, std::move(result));
		}

		for(auto const& pair: slots.image.get(id)){
			auto id = pair.first;
			auto& data = pair.second.data();

			signals.image(id, subbitmap(data));
		}
	}


} }
