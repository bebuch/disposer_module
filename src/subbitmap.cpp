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


namespace disposer_module{ namespace subbitmap{


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
		std::int32_t x;
		std::int32_t y;

		std::size_t width;
		std::size_t height;

		disposer::type_unroll_t< std::tuple, type_list > default_value;
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
		bitmap< T > subbitmap(bitmap< T > const& image)const;


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

		data.params.set(param.x, "x");
		data.params.set(param.y, "y");

		data.params.set(param.width, "width");
		data.params.set(param.height, "height");

		// for integral is NaN defined as 0
		hana::for_each(
			disposer::hana_type_list< type_list >,
			[&data, &param](auto type_t){
				using type = typename decltype(type_t)::type;
				data.params.set(
					std::get< type >(param.default_value),
					"default_value",
					std::numeric_limits< type >::quiet_NaN()
				);
			}
		);

		return std::make_unique< module >(data, std::move(param));
	}


	template < typename T >
	bitmap< T > module::subbitmap(bitmap< T > const& image)const{
		bitmap< T > result(
			param.width,
			param.height,
			std::get< T >(param.default_value)
		);

		std::size_t const bx =
			param.x < 0 ? static_cast< std::size_t >(-param.x) : 0;

		std::size_t const by =
			param.y < 0 ? static_cast< std::size_t >(-param.y) : 0;

		std::size_t const ex =
			static_cast< std::int64_t >(param.width) <
			static_cast< std::int64_t >(image.width()) - param.x ?
				param.width :
					static_cast< std::int32_t >(image.width()) >
					param.x ? image.width() - param.x : 0;

		std::size_t const ey =
			static_cast< std::int64_t >(param.height) <
			static_cast< std::int64_t >(image.height()) - param.y ?
				param.height :
					static_cast< std::int32_t >(image.height()) > param.y ?
					image.height() - param.y : 0;


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


	struct visitor{
		visitor(subbitmap::module& module, std::size_t id):
			module(module), id(id) {}

		subbitmap::module& module;
		std::size_t const id;
	};

	struct sequence_visitor: visitor{
		using visitor::visitor;

		template < typename T >
		void operator()(
			disposer::input_data< bitmap_sequence< T > > const& sequence
		){
			auto& data = sequence.data();

			bitmap_sequence< T > result(data.size());
			for(std::size_t i = 0; i < data.size(); ++i){
				result[i].resize(data[i].size());
				for(std::size_t j = 0; j < data[i].size(); ++j){
					result[i][j] = module.subbitmap(data[i][j]);
				}
			}

			module.signals.sequence.put< T >(std::move(result));
		}
	};

	struct vector_visitor: visitor{
		using visitor::visitor;

		template < typename T >
		void operator()(
			disposer::input_data< bitmap_vector< T > > const& vector
		){
			auto& data = vector.data();

			bitmap_vector< T > result(data.size());
			for(std::size_t i = 0; i < data.size(); ++i){
				result[i] = module.subbitmap(data[i]);
			}

			module.signals.vector.put< T >(std::move(result));
		}
	};

	struct image_visitor: visitor{
		using visitor::visitor;

		template < typename T >
		void operator()(disposer::input_data< bitmap< T > > const& image){
			auto& data = image.data();

			module.signals.image.put< T >(module.subbitmap(data));
		}
	};


	void module::exec(){
		for(auto const& pair: slots.sequence.get()){
			sequence_visitor visitor(*this, pair.first);
			std::visit(visitor, pair.second);
		}

		for(auto const& pair: slots.vector.get()){
			vector_visitor visitor(*this, pair.first);
			std::visit(visitor, pair.second);
		}

		for(auto const& pair: slots.image.get()){
			image_visitor visitor(*this, pair.first);
			std::visit(visitor, pair.second);
		}
	}


	void init(disposer::module_adder& add){
		add("subbitmap", &make_module);
	}

	BOOST_DLL_AUTO_ALIAS(init)


} }
