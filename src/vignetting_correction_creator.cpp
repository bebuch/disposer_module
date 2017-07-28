//-----------------------------------------------------------------------------
// Copyright (c) 2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <disposer/module.hpp>

#include <bitmap/bitmap.hpp>
#include <bitmap/pixel_algorithm.hpp>

#include <boost/dll.hpp>

#include <numeric>


namespace disposer_module::vignetting_correction_creator{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;

	namespace pixel = ::bmp::pixel;

	template < typename T >
	using bitmap = ::bmp::bitmap< T >;


	constexpr auto types = hana::tuple_t<
			std::uint8_t,
			std::uint16_t,
			std::uint32_t,
			std::uint64_t /*,
			pixel::ga8u,
			pixel::ga16u,
			pixel::ga32u,
			pixel::ga64u,
			pixel::rgb8u,
			pixel::rgb16u,
			pixel::rgb32u,
			pixel::rgb64u,
			pixel::rgba8u,
			pixel::rgba16u,
			pixel::rgba32u,
			pixel::rgba64u*/
		>;

	template < typename T >
	T max_value(bitmap< T > const& image){
		return std::accumulate(image.begin(), image.end(), *image.begin(),
			[](T const& res, T const& value){
				using std::max;
				return max(res, value);
			});
	}

	template < typename Module, typename T >
	bitmap< float > exec(Module const& /*module*/, bitmap< T > const& image){
		bitmap< float > result(image.size());
		T const org_max = max_value(image);

		// TODO: Support for 10/12/14 bit images (and so on)
		if(org_max == std::numeric_limits< T >::max()){
			throw std::runtime_error("image is overexposed");
		}

		auto const max = static_cast< float >(org_max);

		std::transform(image.begin(), image.end(), result.begin(),
			[max](auto const& v){
				if(v == 0) throw std::runtime_error("image is underexposed");
				return max / v;
			});

		return result;
	}


	void init(std::string const& name, module_declarant& disposer){
		auto init = module_register_fn(
			module_configure(
				"image"_in(types, wrap_in< bitmap >),
				"image"_out(hana::type_c< bitmap< float > >)/*,
				"max_value"_param(types)*/ // Support for 10/12/14 bit images
			),
			module_enable([]{
				return [](auto& module){
					auto values = module("image"_in).get_references();
					for(auto const& value: values){
						std::visit([&module](auto const& img_ref){
							module("image"_out).put(
								exec(module, img_ref.get()));
						}, value);
					}
				};
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
