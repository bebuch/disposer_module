//-----------------------------------------------------------------------------
// Copyright (c) 2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <disposer/module.hpp>

#include <m3api/xiApi.h>
#undef align

#include <boost/dll.hpp>

#include "bitmap.hpp"


namespace disposer_module{ namespace camera_ximea{



	struct parameter{

	};


	struct module: disposer::module_base{
		module(disposer::make_data const& data, parameter&& param):
			disposer::module_base(data, {image}),
			param(std::move(param))
			{}


		disposer::output< bitmap< std::uint8_t > >
			image{"image"};


		void exec()override;


		void input_ready()override;


		parameter const param;
	};


	disposer::module_ptr make_module(disposer::make_data& data){
		if(data.outputs.find("image") == data.outputs.end()){
			throw std::logic_error(data.type_name +
				": No output defined (use 'image')");
		}

		parameter param;

		return std::make_unique< module >(data, std::move(param));
	}


	void module::input_ready(){
		image.activate< bitmap< std::uint8_t > >();
	}


	void module::exec(){

	}


	void init(disposer::module_adder& add){
		add("camera_ximea", &make_module);
	}

	BOOST_DLL_AUTO_ALIAS(init)


} }
