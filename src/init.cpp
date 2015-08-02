#include "init.hpp"

#include "big_loader.hpp"
#include "big_saver.hpp"
#include "raster.hpp"
#include "async_program.hpp"
#include "async_get.hpp"
#include "add_to_log.hpp"
#include "subbitmap.hpp"


namespace disposer_module{


	void init(){
		big_loader::init();
		big_saver::init();
		raster::init();
		async_program::init();
		async_get::init();
		add_to_log::init();
		subbitmap::init();
	}


}
