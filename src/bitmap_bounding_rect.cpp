//-----------------------------------------------------------------------------
// Copyright (c) 2015 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "bitmap_bounding_rect.hpp"

#include "bitmap.hpp"
#include "rect.hpp"

#include <disposer/io.hpp>

#include <cstdint>
#include <cmath>

#include <boost/dll.hpp>


namespace disposer_module{ namespace bitmap_bounding_rect{


	struct module: disposer::module_base{
		module(disposer::make_data const& data):
			disposer::module_base(data, {image}, {rect}){}


		disposer::input< bitmap< float >, bitmap< double >, bitmap< long double> > image{"image"};
		disposer::output< rect< std::size_t > > rect{"rect"};


		void trigger()override;
	};

	disposer::module_ptr make_module(disposer::make_data& data){
		if(data.is_first()) throw disposer::module_not_as_start(data);

		return std::make_unique< module >(data);
	}

	struct visitor: boost::static_visitor< rect< std::size_t > >{
		template < typename T >
		rect< std::size_t > operator()(disposer::input_data< bitmap< T > > const& data)const{
			using std::isnan;

			auto& image = data.data();

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
	};


	void module::trigger(){
		for(auto const& pair: image.get(id)){
			auto id = pair.first;

			rect.put(id, boost::apply_visitor(visitor{}, pair.second));
		}
	}

	void init(disposer::disposer& disposer){
		disposer.add_module_maker("bitmap_bounding_rect", &make_module);
	}

	BOOST_DLL_AUTO_ALIAS(init)


} }
