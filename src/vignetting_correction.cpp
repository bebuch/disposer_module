//-----------------------------------------------------------------------------
// Copyright (c) 2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <disposer/module.hpp>

#include <bitmap/binary_read.hpp>

#include <boost/dll.hpp>


namespace disposer_module::vignetting_correction{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;

	template < typename T >
	using bitmap = ::bmp::bitmap< T >;


	constexpr auto types = hana::tuple_t<
			std::uint8_t,
			std::uint16_t,
			std::uint32_t,
			std::uint64_t
		>;


	template < typename T >
	bitmap< T > exec(
		bitmap< T >&& image,
		bitmap< float > const& factor_image
	){
		std::transform(image.begin(), image.end(), factor_image.begin(),
			image.begin(),
			[](T const v, float const r){
				constexpr auto m = static_cast< float >(
					std::numeric_limits< T >::max());
				return static_cast< T >(std::min(v * r, m));
			});
		return std::move(image);
	}


	void init(std::string const& name, module_declarant& disposer){
		auto init = module_register_fn(
			module_configure(
				"image"_in(types, wrap_in< bitmap >),
				"image"_out(types, wrap_in< bitmap >,
					enable_by_types_of("image"_in)
				),
				"factor_image_filename"_param(hana::type_c< std::string >)
			),
			module_enable([](auto const& module){
				auto const path = module("factor_image_filename"_param).get();
				return [factor_image = bmp::binary_read< float >(path)]
					(auto& module){
						auto values = module("image"_in).get_values();
						for(auto&& value: std::move(values)){
							std::visit([&module, &factor_image](auto&& img){
								module("image"_out).put(
									exec(std::move(img), factor_image));
							}, std::move(value));
						}
					};
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
