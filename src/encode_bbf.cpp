//-----------------------------------------------------------------------------
// Copyright (c) 2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <bitmap/binary_write.hpp>

#include <disposer/module.hpp>

#include <boost/dll.hpp>


namespace disposer_module::encode_bbf{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;
	using namespace hana::literals;

	namespace pixel = ::bmp::pixel;
	using ::bmp::bitmap;


	void init(std::string const& name, module_declarant& disposer){
		auto init = generate_module(
			dimension_list{
				dimension_c<
					bool,
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
					pixel::ga8,
					pixel::ga16,
					pixel::ga32,
					pixel::ga64,
					pixel::ga8u,
					pixel::ga16u,
					pixel::ga32u,
					pixel::ga64u,
					pixel::ga32f,
					pixel::ga64f,
					pixel::rgb8,
					pixel::rgb16,
					pixel::rgb32,
					pixel::rgb64,
					pixel::rgb8u,
					pixel::rgb16u,
					pixel::rgb32u,
					pixel::rgb64u,
					pixel::rgb32f,
					pixel::rgb64f,
					pixel::rgba8,
					pixel::rgba16,
					pixel::rgba32,
					pixel::rgba64,
					pixel::rgba8u,
					pixel::rgba16u,
					pixel::rgba32u,
					pixel::rgba64u,
					pixel::rgba32f,
					pixel::rgba64f
				>
			},
			module_configure(
				make("image"_in, wrapped_type_ref_c< bitmap, 0 >),
				make("data"_out, free_type_c< std::string >),
				make("endian"_param, free_type_c< boost::endian::order >,
					parser_fn([](
						auto const& /*iop*/,
						std::string_view data,
						hana::basic_type< boost::endian::order >
					){
						using boost::endian::order;
						if(data == "little") return order::little;
						if(data == "big") return order::big;
						if(data == "native") return order::native;
						throw std::runtime_error("unknown value '"
							+ std::string(data) + "', allowed values are: "
							"little, big & native");

					}),
					default_value(boost::endian::order::native)
				)
			),
			exec_fn([](auto& module){
				for(auto const& img: module("image"_in).references()){
					std::ostringstream os(std::ios::out | std::ios::binary);
					::bmp::binary_write(img, os, module("endian"_param));
					module("data"_out).push(os.str());
				}
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
