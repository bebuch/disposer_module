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


	template < typename Module, typename T >
	bitmap< T > exec(
		Module const& module,
		bitmap< T >&& image,
		bitmap< float > const& factor_image
	){
		auto const max_value = static_cast< float >(
			module("max_value"_param).get(hana::type_c< T >));
		std::transform(image.begin(), image.end(), factor_image.begin(),
			image.begin(),
			[max_value](T const v, float const r){
				return static_cast< T >(std::min(v * r, max_value));
			});
		return std::move(image);
	}


	void init(std::string const& name, module_declarant& disposer){
		auto init = module_register_fn(
			module_configure(
				make("image"_in, types, wrap_in< bitmap >),
				make("image"_out, types, wrap_in< bitmap >,
					enable_by_types_of("image"_in)),
				make("factor_image_filename"_param,
					hana::type_c< std::string >),
				make("max_value"_param, types,
					enable_by_types_of("image"_in),
					default_value_fn([](auto const& iop, auto t){
						using type = typename decltype(t)::type;
						if constexpr(
							std::is_integral_v< type > && sizeof(type) == 1
						){
							return std::numeric_limits< type >::max();
						}
					}))
			),
			state_maker_fn([](auto const& module){
				auto const path = module("factor_image_filename"_param).get();
				return bmp::binary_read< float >(path);
			}),
			exec_fn([](auto& module){
				auto const& factor_image = module.state();
				auto values = module("image"_in).get_values();
				for(auto&& value: std::move(values)){
					std::visit([&module, &factor_image](auto&& img){
						module("image"_out).put(
							exec(module, std::move(img), factor_image));
					}, std::move(value));
				}
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
