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

#include <boost/hana.hpp>
#include <boost/dll.hpp>

#include <cstdint>
#include <limits>


namespace disposer_module{ namespace pixel_transform{


	namespace hana = boost::hana;


	using disposer::type_position_v;

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
		long double
	>;


	struct parameter{
		long double min;
		long double max;
		long double new_min;
		long double new_max;
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
		void pixel_transform(bitmap< T >& image)const;


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

		data.params.set(param.min, "min");
		data.params.set(param.max, "max");
		data.params.set(param.new_min, "new_min");
		data.params.set(param.new_max, "new_max");

		return std::make_unique< module >(data, std::move(param));
	}


	template < typename T >
	void module::pixel_transform(bitmap< T >& image)const{
		auto const min = param.min;
		auto const max = param.max;
		auto const diff = max - min;
		auto const new_min = param.new_min;
		auto const new_max = param.new_max;
		auto const new_diff = new_max - new_min;
		auto const scale = new_diff / diff;

		for(auto& value: image){
			value = static_cast< T >((value - min) * scale + new_min);
		}
	}


	struct visitor{
		visitor(pixel_transform::module& module):
			module(module) {}

		pixel_transform::module& module;
	};

	struct sequence_visitor: visitor{
		using visitor::visitor;

		template < typename T >
		void operator()(
			disposer::input_data< bitmap_sequence< T > >&& sequence
		){
			auto output = sequence.get();

			for(auto& vector: output->data()){
				for(auto& image: vector){
					module.pixel_transform(image);
				}
			}

			module.signals.sequence.put< T >(std::move(output));
		}
	};

	struct vector_visitor: visitor{
		using visitor::visitor;

		template < typename T >
		void operator()(
			disposer::input_data< bitmap_vector< T > >&& vector
		){
			auto output = vector.get();

			for(auto& image: output->data()){
				module.pixel_transform(image);
			}

			module.signals.vector.put< T >(std::move(output));
		}
	};

	struct image_visitor: visitor{
		using visitor::visitor;

		template < typename T >
		void operator()(disposer::input_data< bitmap< T > >&& image){
			auto output = image.get();

			module.pixel_transform(output->data());

			module.signals.image.put< T >(std::move(output));
		}
	};


	void module::exec(){
		for(auto&& [id, input_data]: slots.sequence.get()){
			(void)id;
			sequence_visitor visitor(*this);
			std::visit(visitor, std::move(input_data));
		}

		for(auto&& [id, input_data]: slots.vector.get()){
			(void)id;
			vector_visitor visitor(*this);
			std::visit(visitor, std::move(input_data));
		}

		for(auto&& [id, input_data]: slots.image.get()){
			(void)id;
			image_visitor visitor(*this);
			std::visit(visitor, std::move(input_data));
		}
	}


	void init(disposer::module_adder& add){
		add("pixel_transform", &make_module);
	}

	BOOST_DLL_AUTO_ALIAS(init)


} }
