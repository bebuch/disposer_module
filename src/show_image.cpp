//-----------------------------------------------------------------------------
// Copyright (c) 2015-2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <disposer/module.hpp>

#include <boost/dll.hpp>

#include <CImg.h>

#include "bitmap.hpp"



namespace disposer_module{ namespace show_image{


	struct parameter{
		std::string window_title;

		unsigned width;
		unsigned height;
	};


	struct module: disposer::module_base{
		module(disposer::make_data const& data, parameter&& param):
			disposer::module_base(data, {image}),
			display(param.width, param.height, param.window_title.c_str()){}

		disposer::input< bitmap< std::uint8_t > > image{"image"};

		virtual void exec()override{
			for(auto& pair: image.get()){
				auto& data = pair.second.data();

				img.assign(
					data.data(), data.width(), data.height()
				);
				display.display(img);
			}
		}

		cimg_library::CImgDisplay display;
		cimg_library::CImg< std::uint8_t > img;
	};

	disposer::module_ptr make_module(disposer::make_data& data){
		if(data.is_first()) throw disposer::module_not_as_start(data);

		parameter param;

		data.params.set(param.window_title, "window_title");
		data.params.set(param.width, "width");
		data.params.set(param.height, "height");

		return std::make_unique< module >(data, std::move(param));
	}

	void init(disposer::module_adder& add){
		add("show_image", &make_module);
	}

	BOOST_DLL_AUTO_ALIAS(init)


} }
