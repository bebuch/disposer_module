//-----------------------------------------------------------------------------
// Copyright (c) 2017-2018 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <disposer/module.hpp>

#include <bitmap/histogram.hpp>
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


	template < typename T >
	T max_value(bitmap< T > const& image){
		return std::accumulate(image.begin(), image.end(), *image.begin(),
			[](T const& res, T const& value){
				using std::max;
				return max(res, value);
			});
	}

	template < typename Module, typename T >
	bitmap< float > exec(Module const module, bitmap< T > const& image){
		auto const max_v = module("max_value"_param);

		T const max = module.log([](
				logsys::stdlogb& os,
				std::optional< T > const& max
			){
				os << "find max image value";
				if(max){
					os << ": " << *max;
				}
			}, [&]{
				return max_value(image);
			});

		if(max >= max_v){
			throw std::runtime_error("image is overexposed");
		}

		auto const reference = module("reference"_param);
		float const reference_value = module.log(
			[](logsys::stdlogb& os, std::optional< float > ref_v){
				os << "reference value";
				if(ref_v){
					os << ": " << *ref_v;
				}
			}, [&]{
				if(reference == 100) return static_cast< float >(max);

				auto const h = bmp::histogram(image, T(0), max_v, max_v, true);
				auto const pc = static_cast< float >(image.point_count());
				auto const iter = std::find_if(h.rbegin(), h.rend(),
					[pc, reference](auto const& count){
						return count / pc < reference / 100;
					});
				return static_cast< float >(max_v - (iter - h.rbegin()));
			});

		return module.log([](logsys::stdlogb& os){ os << "calculation"; },
			[&]{
				bitmap< float > result(image.size());
				std::transform(image.begin(), image.end(), result.begin(),
					[reference_value](auto const& v){
						if(v == 0){
							throw std::runtime_error("image is underexposed");
						}
						return reference_value / v;
					});
				return result;
			});
	}


	void init(std::string const& name, declarant& disposer){
		auto init = generate_module(
			"creates a vignetting correction image based on an image of a "
			"diffuse reflective white surface, the base image must not "
			"contain overexposer or underexposer",
			dimension_list{
				dimension_c<
					std::uint8_t,
					std::uint16_t,
					std::uint32_t,
					std::uint64_t
				>
			},
			module_configure(
				make("reference"_param, free_type_c< float >,
					"percent value that affects the brightness after "
					"correction",
					verify_value_fn([](std::size_t value){
						if(value > 100 || value < 1){
							throw std::logic_error(
								"expected a percent value (1% - 100%)");
						}
					}),
					default_value(99)),
				make("image"_in, wrapped_type_ref_c< bitmap, 0 >,
					"image of a diffuse reflective white surface"),
				make("image"_out, free_type_c< bitmap< float > >,
					"created vignetting correction image"),
				make("max_value"_param, type_ref_c< 0 >,
					"overexposer value (e.g. 1023 for 10 bit images)",
					default_value_fn([](auto t){
						using type = typename decltype(t)::type;
						if constexpr(
							std::is_integral_v< type > && sizeof(type) == 1
						){
							return std::numeric_limits< type >::max();
						}
					}))
			),
			exec_fn([](auto module){
				for(auto const& img: module("image"_in).references()){
					module("image"_out).push(exec(module, img));
				}
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
