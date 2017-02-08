//-----------------------------------------------------------------------------
// Copyright (c) 2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "bitmap_vector.hpp"

#include <disposer/module.hpp>

#include <boost/hana.hpp>
#include <boost/dll.hpp>

#include <cstdint>


namespace disposer_module{ namespace bitmap_vector_join{


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
		std::size_t images_per_line;

		disposer::type_unroll_t< std::tuple, type_list > default_value;
	};


	struct module: disposer::module_base{
		module(disposer::make_data const& data, parameter&& param):
			disposer::module_base(
				data,
				{slots.image_vector},
				{signals.image}
			),
			param(std::move(param))
			{}


		template < typename T >
		bitmap< T > bitmap_vector_join(bitmap_vector< T > const& vectors)const;


		struct{
			disposer::container_input< bitmap_vector, type_list >
				image_vector{"image_vector"};
		} slots;

		struct{
			disposer::container_output< bitmap, type_list >
				image{"image"};
		} signals;


		void exec()override;
		void input_ready()override;


		parameter const param;
	};


	disposer::module_ptr make_module(disposer::make_data& data){
		if(data.is_first()) throw disposer::module_not_as_start(data);

		parameter param;

		data.params.set(param.images_per_line, "images_per_line");
		if(param.images_per_line == 0){
			throw std::logic_error("parameter images_per_line == 0");
		}

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
	bitmap< T > module::bitmap_vector_join(
		bitmap_vector< T > const& vectors
	)const{
		auto const ips = param.images_per_line;
		auto const& input_size = vectors[0].size();

		std::size_t width =
			std::min(ips, vectors.size()) * input_size.width();
		std::size_t height =
			((vectors.size() - 1) / ips + 1) * input_size.height();

		bitmap< T > result(
			width, height, std::get< T >(param.default_value)
		);

		for(std::size_t i = 0; i < vectors.size(); ++i){
			auto const x_offset = (i % ips) * input_size.width();
			auto const y_offset = (i / ips) * input_size.height();

			for(std::size_t y = 0; y < input_size.height(); ++y){
				for(std::size_t x = 0; x < input_size.width(); ++x){
					result(x_offset + x, y_offset + y) = vectors[i](x, y);
				}
			}
		}

		return result;
	}


	struct visitor: boost::static_visitor< void >{
		visitor(bitmap_vector_join::module& module, std::size_t id):
			module(module), id(id) {}


		template < typename T >
		void operator()(
			disposer::input_data< bitmap_vector< T > > const& vector
		){
			auto& vectors = vector.data();


			auto iter = vectors.cbegin();
			for(auto size = (iter++)->size(); iter != vectors.cend(); ++iter){
				if(size == iter->size()) continue;
				throw std::runtime_error(
					"bitmaps with differenz sizes");
			}

			module.signals.image.put< T >(module.bitmap_vector_join(vectors));
		}

		bitmap_vector_join::module& module;
		std::size_t const id;
	};

	void module::exec(){
		for(auto const& pair: slots.image_vector.get()){
			visitor visitor(*this, pair.first);
			boost::apply_visitor(visitor, pair.second);
		}
	}

	void module::input_ready(){
		signals.image.activate_types(
			slots.image_vector.active_types_transformed(
				[](auto type){
					return hana::type_c< typename decltype(type)::type::value_type >;
				}
			)
		);
	}



	void init(disposer::module_adder& add){
		add("bitmap_vector_join", &make_module);
	}

	BOOST_DLL_AUTO_ALIAS(init)


} }
