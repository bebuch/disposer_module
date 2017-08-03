//-----------------------------------------------------------------------------
// Copyright (c) 2017 Benjamin Buch
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


	constexpr auto types = hana::tuple_t<
			std::uint8_t,
			std::uint16_t,
			std::uint32_t,
			std::uint64_t
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
	bitmap< float > exec(Module const& module, bitmap< T > const& image){
		auto const max_v = module("max_value"_param).get(hana::type_c< T >);

		T const max = module.log([](logsys::stdlogb& os, T const* max){
				os << "find max image value";
				if(max) os << ": " << *max;
			}, [&]{
				return max_value(image);
			});

		if(max >= max_v){
			throw std::runtime_error("image is overexposed");
		}

		auto const reference = module("reference"_param).get();
		float const reference_value = module.log(
			[](logsys::stdlogb& os, float const* ref_v){
				os << "reference value";
				if(ref_v) os << ": " << *ref_v;
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


	void init(std::string const& name, module_declarant& disposer){
		auto init = module_register_fn(
			module_configure(
				make("image"_in, types, wrap_in< bitmap >),
				make("image"_out, hana::type_c< bitmap< float > >),
				make("max_value"_param, types,
					enable_by_types_of("image"_in),
					default_value_fn([](auto const& iop, auto t){
						using type = typename decltype(t)::type;
						if constexpr(
							std::is_integral_v< type > && sizeof(type) == 1
						){
							return std::numeric_limits< type >::max();
						}
					})),
				make("reference"_param, hana::type_c< float >,
					verify_value_fn([](auto const& /*iop*/, std::size_t value){
						if(value > 100 || value < 1){
							throw std::logic_error(
								"expected a percent value (1% - 100%)");
						}
					}),
					default_value(99))
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
