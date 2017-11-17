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


	template < typename Module, typename T >
	bitmap< T > exec(
		Module const& module,
		bitmap< T >&& image,
		bitmap< float > const& factor_image
	){
		auto const max_value = static_cast< float >(module("max_value"_param));
		std::transform(image.begin(), image.end(), factor_image.begin(),
			image.begin(),
			[max_value](T const v, float const r){
				return static_cast< T >(std::min(v * r, max_value));
			});
		return std::move(image);
	}


	void init(std::string const& name, module_declarant& disposer){
		auto init = generate_module(
			dimension_list{
				dimension_c<
					std::uint8_t,
					std::uint16_t,
					std::uint32_t,
					std::uint64_t
				>
			},
			module_configure(
				make("image"_in, wrapped_type_ref_c< bitmap, 0 >),
				make("image"_out, wrapped_type_ref_c< bitmap, 0 >),
				make("factor_image_filename"_param, free_type_c< std::string >),
				make("max_value"_param, type_ref_c< 0 >,
					default_value_fn([](auto const& iop, auto t){
						using type = typename decltype(t)::type;
						if constexpr(
							std::is_integral_v< type > && sizeof(type) == 1
						){
							return std::numeric_limits< type >::max();
						}
					}))
			),
			module_init_fn([](auto const& module){
				auto const path = module("factor_image_filename"_param);
				return bmp::binary_read< float >(path);
			}),
			exec_fn([](auto& module){
				auto const& factor_image = module.state();
				for(auto&& img: module("image"_in).values()){
					module("image"_out).push(
						exec(module, std::move(img), factor_image));
				}
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
