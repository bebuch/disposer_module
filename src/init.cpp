//-----------------------------------------------------------------------------
// Copyright (c) 2015 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "init.hpp"

#include "big_loader.hpp"
#include "big_saver.hpp"
#include "raster.hpp"
#include "async_get.hpp"
#include "add_to_log.hpp"
#include "subbitmap.hpp"


namespace disposer_module{


	void init(){
		big_loader::init();
		big_saver::init();
		raster::init();
		async_get::init();
		add_to_log::init();
		subbitmap::init();
	}


}
