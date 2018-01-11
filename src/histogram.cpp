//-----------------------------------------------------------------------------
// Copyright (c) 2017-2018 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <bitmap/histogram.hpp>

#include <disposer/module.hpp>

#include <boost/dll.hpp>


namespace disposer_module::histogram{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;

	template < typename T >
	using bitmap = ::bmp::bitmap< T >;


	void init(std::string const& name, declarant& disposer){
		auto init = generate_module(
			"make a histogram of an image",
			dimension_list{
				dimension_c<
					std::int8_t,
					std::int16_t,
					std::int32_t,
					std::int64_t,
					std::uint8_t,
					std::uint16_t,
					std::uint32_t,
					std::uint64_t,
					float,
					double
				>
			},
			module_configure(
				make("image"_in, wrapped_type_ref_c< bitmap, 0 >,
					"the source image"),
				make("histogram"_out,
					free_type_c< std::vector< std::size_t > >,
					"the resulting histogram data"),
				make("cumulative"_param, free_type_c< bool >,
					"true for cumulative histogram, false otherwise",
					default_value(false)),
				make("min"_param, type_ref_c< 0 >,
					"values smaller than min are treated as if they are min",
					default_value_fn([](auto t){
						using type = typename decltype(t)::type;
						if constexpr(
							std::is_integral_v< type > && sizeof(type) == 1
						){
							return std::numeric_limits< type >::min();
						}
					})),
				make("max"_param, type_ref_c< 0 >,
					"values greater than max are treated as if they are max",
					verify_value_fn([](auto const& max, auto const module){
						if(module("min"_param) < max) return;
						throw std::logic_error("max must be greater min");
					}),
					default_value_fn([](auto t){
						using type = typename decltype(t)::type;
						if constexpr(
							std::is_integral_v< type > && sizeof(type) == 1
						){
							return std::numeric_limits< type >::max();
						}
					})),
				make("bin_count"_param, free_type_c< std::size_t >,
					"count of histogram bins, the bins are evenly distributed "
					"between min and max.")
			),
			exec_fn([](auto module){
				for(auto const& img: module("image"_in).references()){
					module("histogram"_out).push(bmp::histogram(img,
							module("min"_param),
							module("max"_param),
							module("bin_count"_param),
							module("cumulative"_param)
						));
				}
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
