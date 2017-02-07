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


namespace disposer_module{ namespace demosaic{


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
		std::size_t x_count;
		std::size_t y_count;
	};


	struct module: disposer::module_base{
		module(disposer::make_data const& data, parameter&& param):
			disposer::module_base(
				data,
				{slots.image},
				{signals.image_vector}
			),
			param(std::move(param))
			{}


		template < typename T >
		bitmap_vector< T > demosaic(bitmap< T > const& image)const;


		struct{
			disposer::container_input< bitmap, type_list >
				image{"image"};
		} slots;

		struct{
			disposer::container_output< bitmap_vector, type_list >
				image_vector{"image_vector"};
		} signals;


		void exec()override;


		parameter const param;
	};


	disposer::module_ptr make_module(disposer::make_data& data){
		if(data.is_first()) throw disposer::module_not_as_start(data);

		parameter param;

		data.params.set(param.x_count, "x_count");
		if(param.x_count == 0){
			throw std::logic_error("parameter x_count == 0");
		}

		data.params.set(param.y_count, "y_count");
		if(param.x_count == 0){
			throw std::logic_error("parameter x_count == 0");
		}

		return std::make_unique< module >(data, std::move(param));
	}


	template < typename T >
	bitmap_vector< T > module::demosaic(bitmap< T > const& image)const{
		auto const xc = param.x_count;
		auto const yc = param.y_count;

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

		bitmap_vector< T >
			result(xc * yc, bitmap< T >(width, height));

		for(std::size_t y = 0; y < image.height(); ++y){
			for(std::size_t x = 0; x < image.width(); ++x){
				auto result_index = (y % yc) * xc + x % xc;
				result[result_index](x / xc, y / yc) = image(x, y);
			}
		}

		return result;
	}


	struct visitor: boost::static_visitor< void >{
		visitor(demosaic::module& module, std::size_t id):
			module(module), id(id) {}


		template < typename T >
		void operator()(disposer::input_data< bitmap< T > > const& vector){
			module.signals.image_vector
				.put< T >(module.demosaic(vector.data()));
		}

		demosaic::module& module;
		std::size_t const id;
	};


	void module::exec(){
		for(auto const& pair: slots.image.get()){
			visitor visitor(*this, pair.first);
			boost::apply_visitor(visitor, pair.second);
		}
	}


	void init(disposer::module_adder& add){
		add("demosaic", &make_module);
	}

	BOOST_DLL_AUTO_ALIAS(init)


} }
