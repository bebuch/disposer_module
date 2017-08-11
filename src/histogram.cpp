//-----------------------------------------------------------------------------
// Copyright (c) 2017 Benjamin Buch
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


	constexpr auto types = hana::tuple_t<
			std::int8_t,
			std::uint8_t,
			std::int16_t,
			std::uint16_t,
			std::int32_t,
			std::uint32_t,
			std::int64_t,
			std::uint64_t,
			float,
			double
		>;


	template < typename Module, typename T >
	std::vector< std::size_t > exec(
		Module const& module,
		bitmap< T > const& image
	){
		return bmp::histogram(image,
			module("min"_param).get(hana::type_c< T >),
			module("max"_param).get(hana::type_c< T >),
			module("bin_count"_param).get(),
			module("cumulative"_param).get()
		);
	}


	void init(std::string const& name, module_declarant& disposer){
		auto init = module_register_fn(
			module_configure(
				make("image"_in, types, wrap_in< bitmap >),
				make("histogram"_out,
					hana::type_c< std::vector< std::size_t > >),
				make("cumulative"_param, hana::type_c< bool >,
					default_value(false)),
				make("min"_param, types, enable_by_types_of("image"_in),
					default_value_fn([](auto const& iop, auto t){
						using type = typename decltype(t)::type;
						if constexpr(
							std::is_integral_v< type > && sizeof(type) == 1
						){
							return std::numeric_limits< type >::min();
						}
					})),
				make("max"_param, types, enable_by_types_of("image"_in),
					verify_value_fn([](auto const& iop, auto const& max){
						auto const min =
							iop("min"_param).get(hana::typeid_(max));
						if(min < max) return;
						throw std::logic_error("max must be greater min");
					}),
					default_value_fn([](auto const& iop, auto t){
						using type = typename decltype(t)::type;
						if constexpr(
							std::is_integral_v< type > && sizeof(type) == 1
						){
							return std::numeric_limits< type >::max();
						}
					})),
				make("bin_count"_param, hana::type_c< std::size_t >)
			),
			exec_fn([](auto& module){
				auto values = module("image"_in).get_references();
				for(auto const& value: values){
					std::visit([&module](auto const& img_ref){
						module("histogram"_out).put(
							exec(module, img_ref.get()));
					}, value);
				}
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
