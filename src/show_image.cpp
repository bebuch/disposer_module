//-----------------------------------------------------------------------------
// Copyright (c) 2015-2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <disposer/module.hpp>

#include <bitmap/pixel.hpp>

#include <boost/dll.hpp>

#include <CImg.h>

#include "bitmap.hpp"



namespace disposer_module{ namespace show_image{


	namespace pixel = ::bitmap::pixel;


	using type_list = disposer::type_list<
		std::uint8_t,
		std::uint16_t,
		pixel::rgb8
	>;


	struct parameter{
		std::string window_title;
	};


	using cimg_variant = std::variant<
			cimg_library::CImg< std::uint8_t >,
			cimg_library::CImg< std::uint16_t >
		>;


	struct assign_visitor{
		assign_visitor(cimg_variant& img): img(img){}

		cimg_variant& img;

		void operator()(
			disposer::input_data< bitmap< uint8_t > > const& data
		)const{
			auto& image = data.data();
			img = cimg_library::CImg< std::uint8_t >(
				image.data(), image.width(), image.height()
			);
		}

		void operator()(
			disposer::input_data< bitmap< uint16_t > > const& data
		)const{
			auto& image = data.data();
			img = cimg_library::CImg< std::uint16_t >(
				image.data(), image.width(), image.height()
			);
		}

		void operator()(
			disposer::input_data< bitmap< pixel::rgb8 > > const& data
		)const{
			auto& image = data.data();
			img = cimg_library::CImg< std::uint8_t >(
				reinterpret_cast< std::uint8_t const* >(image.data()),
				image.width(), image.height(), 1, 3
			);
		}
	};

	struct display_visitor{
		display_visitor(cimg_library::CImgDisplay& display):
			display(display){}

		cimg_library::CImgDisplay& display;

		template < typename T >
		void operator()(T const& img)const{
			if(
				display.width() != img.width() ||
				display.height() != img.height()
			){
				display.resize(img, false);
			}

			display.display(img);
		}
	};


	struct module: disposer::module_base{
		module(disposer::make_data const& data, parameter&& param):
			disposer::module_base(data, {image}),
			display(200, 100, param.window_title.c_str()){}

		disposer::container_input< bitmap, type_list > image{"image"};

		virtual void exec()override{
			for(auto& pair: image.get()){
				auto& data = pair.second;

				std::visit(assign_visitor(img), data);
				std::visit(display_visitor(display), img);
				display.show();
			}
		}

		cimg_library::CImgDisplay display;
		cimg_variant img;
	};

	disposer::module_ptr make_module(disposer::make_data& data){
		if(data.is_first()) throw disposer::module_not_as_start(data);

		parameter param;

		data.params.set(param.window_title, "window_title");

		return std::make_unique< module >(data, std::move(param));
	}

	void init(disposer::module_adder& add){
		add("show_image", &make_module);
	}

	BOOST_DLL_AUTO_ALIAS(init)


} }
