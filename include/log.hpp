#ifndef _disposer_module_log_hpp_INCLUDED_
#define _disposer_module_log_hpp_INCLUDED_

#include "size_io.hpp"
#include "point_io.hpp"
#include "std_array_io.hpp"
#include "std_vector_io.hpp"

#include <disposer/log.hpp>
#include <disposer/log_tag.hpp>

#include <fstream>


namespace disposer_module{


	namespace log{


		struct info: disposer::log_tag_base{};


	}


}


#endif
