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

#include <boost/dll.hpp>


namespace disposer_module::normalize_bitmap{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;

	template < typename T >
	using bitmap = ::bmp::bitmap< T >;


	void init(std::string const& name, module_declarant& disposer){
		auto init = module_register_fn(
			dimension_list{
				dimension_c<
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
				>
			},
			module_configure(
				make("image"_in, wrapped_type_ref_c< bitmap, 0 >),
				make("min"_param, type_ref_c< 0 >),
				make("max"_param, type_ref_c< 0 >),
				make("image"_out, wrapped_type_ref_c< bitmap, 0 >)
			),
			exec_fn([](auto& module){
				auto t = module.dimension(hana::size_c< 0 >);
				using type = typename decltype(t)::type;

				for(auto img: module("image"_in).values()){
					auto const [min_iter, max_iter] =
						std::minmax_element(img.begin(), img.end());
					auto const min = *min_iter;
					auto const max = *max_iter;
					auto const diff = max - min;
					auto const new_min = module("min"_param);
					auto const new_max = module("max"_param);
					auto const new_diff = new_max - new_min;
					auto const scale = static_cast< double >(new_diff) / diff;

					for(auto& value: img){
						value = static_cast< type >(
							(value - min) * scale + new_min);
					}

					module("image"_out).push(std::move(img));
				}
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
