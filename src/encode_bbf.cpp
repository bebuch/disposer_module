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

	namespace pixel = ::bitmap::pixel;
	using ::bitmap::bitmap;


	constexpr auto types = hana::tuple_t<
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
		>;


	void init(std::string const& name, module_declarant& disposer){
		auto init = module_register_fn(
			module_configure(
				"image"_in(types,
					template_transform_c< bitmap >,
					required),
				"data"_out(hana::type_c< std::string >),
				"endian"_param(hana::type_c< boost::endian::order >,
					parser([](
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
					default_values(boost::endian::order::native),
					type_as_text(
						hana::make_pair(hana::type_c< boost::endian::order >,
						"endian"_s)
					)
				)
			),
			module_enable([]{
				return [](auto& module){
					auto values = module("image"_in).get_references();
					for(auto const& value: values){
						std::visit([&module](auto const& img){
							std::ostringstream os
								(std::ios::out | std::ios::binary);
							::bitmap::binary_write(img.get(), os,
								module("endian"_param).get());
							module("data"_out).put(os.str());
						}, value);
					}
				};
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
