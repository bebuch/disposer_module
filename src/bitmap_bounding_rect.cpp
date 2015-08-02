//-----------------------------------------------------------------------------
// Copyright (c) 2015 Benjamin Buch
//
// https://github.com/bebuch/disposer
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "bitmap_bounding_rect.hpp"

#include "bitmap.hpp"
#include "rect.hpp"

#include <disposer/module_base.hpp>

#include <cstdint>
#include <cmath>


namespace disposer_module{ namespace bitmap_bounding_rect{


	template < typename T >
	struct module: disposer::module_base{
		module(std::string const& type, std::string const& chain, std::string const& name):
			disposer::module_base(type, chain, name){
				inputs = disposer::make_input_list(image);
				outputs = disposer::make_output_list(rect);
			}

		disposer::module_input< bitmap< T > > image{"image"};
		disposer::module_output< rect< std::size_t > > rect{"rect"};

		void trigger(std::size_t id)override;
	};

	template < typename T >
	disposer::module_ptr make_module(
		std::string const& type,
		std::string const& chain,
		std::string const& name,
		disposer::io_list const&,
		disposer::io_list const&,
		disposer::parameter_processor&,
		bool is_start
	){
		if(is_start) throw disposer::module_not_as_start(type, chain);

		return std::make_unique< module< T > >(type, chain, name);
	}

	void init(){
		add_module_maker("bitmap_bounding_rect_float", &make_module< float >);
		add_module_maker("bitmap_bounding_rect_double", &make_module< double >);
		add_module_maker("bitmap_bounding_rect_long_double", &make_module< long double >);
	}


	template < typename T >
	rect< std::size_t > calc_rect(bitmap< T > const& image){
		using std::isnan;

		std::size_t x_begin = image.width();
		std::size_t y_begin = image.height();
		std::size_t x_end = 0;
		std::size_t y_end = 0;

		// find y begin
		for(std::size_t y = 0; y < image.height(); ++y){
			for(std::size_t x = 0; x < image.width(); ++x){
				if(isnan(image(x, y))) continue;

				x_begin = x;
				y_begin = y;

				break;
			}

			if(y_begin != image.width()) break;
		}

		// return if all is NaN
		if(y_begin == image.width()) return rect< std::size_t >(0, 0);

		// find y end
		for(std::size_t y = image.height() - 1; y > 0; --y){
			for(std::size_t x = 0; x < image.width(); ++x){
				if(isnan(image(x, y))) continue;

				x_end = x;
				y_end = y;

				break;
			}

			if(y_end != 0) break;
		}

		// find x begin
		for(std::size_t y = y_begin; y < y_end; ++y){
			for(std::size_t x = 0; x < x_begin; ++x){
				if(isnan(image(x, y))) continue;

				x_begin = x;

				break;
			}
		}

		// find x begin
		for(std::size_t y = y_begin; y <= y_end; ++y){
			for(std::size_t x = x_begin; x > x_end; --x){
				if(isnan(image(x, y))) continue;

				x_end = x;

				break;
			}
		}

		// attention: end sizes are one after the end
		return rect< std::size_t >(x_begin, y_begin, 1 + x_end - x_begin, 1 + y_end - y_begin);
	}


	template < typename T >
	void module< T >::trigger(std::size_t id){
		for(auto const& pair: image.get(id)){
			auto id = pair.first;
			auto& data = pair.second.data();

			rect(id, calc_rect(data));
		}
	}


} }
