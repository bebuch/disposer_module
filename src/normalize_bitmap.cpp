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


	constexpr auto types = dimension_c<
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
		>;

	void init(std::string const& name, module_declarant& disposer){
		auto init = generate_module(
			dimension_list{
				types,
				types
			},
			module_configure(
				make("format"_param,
					free_type_c< std::optional< std::size_t > >,
					parser_fn([](
						auto const /*module*/,
						std::string_view data,
						hana::basic_type< std::optional< std::size_t > >
					){
						constexpr std::array< std::string_view, 10 > list{{
								"int8",
								"int16",
								"int32",
								"int64",
								"uint8",
								"uint16",
								"uint32",
								"uint64",
								"float32",
								"float64"
							}};
						auto iter = std::find(list.begin(), list.end(), data);
						if(iter == list.end()){
							std::ostringstream os;
							os << "unknown value '" << data
								<< "', valid values are: ";
							bool first = true;
							for(auto name: list){
								if(first){ first = false; }else{ os << ", "; }
								os << name;
							}
							throw std::runtime_error(os.str());
						}
						return std::optional< std::size_t >
							(iter - list.begin());
					})),
				make("image"_in, wrapped_type_ref_c< bitmap, 0 >),
				set_dimension_fn([](auto const module){
					auto const optional_number = module("format"_param);
					std::size_t const number =
						[module, &optional_number]()->std::size_t{
							if(optional_number){
								return *optional_number;
							}else{
								using namespace hana::literals;
								auto type =
									module.dimension(hana::size_c< 0 >);
								auto types =
									module.dimension_types(hana::size_c< 1 >);
								auto index = hana::index_if(types,
									hana::equal.to(type));
								return index.value();
							}
						}();
					return solved_dimensions{index_component< 1 >{number}};
				}),
				make("min"_param, type_ref_c< 1 >),
				make("max"_param, type_ref_c< 1 >),
				make("image"_out, wrapped_type_ref_c< bitmap, 1 >)
			),
			exec_fn([](auto module){
				auto t_in = module.dimension(hana::size_c< 0 >);
				auto t_out = module.dimension(hana::size_c< 1 >);
				using type = typename decltype(t_out)::type;

				if constexpr(t_in == t_out){
					for(auto img: module("image"_in).values()){
						auto const [min_iter, max_iter] =
							std::minmax_element(img.begin(), img.end());
						auto const min = *min_iter;
						auto const max = *max_iter;
						auto const diff = static_cast< double >(max) - min;
						auto const new_min = module("min"_param);
						auto const new_max = module("max"_param);
						auto const new_diff =
							static_cast< double >(new_max) - new_min;
						auto const scale = new_diff / diff;

						for(auto& value: img){
							value = static_cast< type >(
								(value - min) * scale + new_min);
						}

						module("image"_out).push(std::move(img));
					}
				}else{
					for(auto const& img: module("image"_in).references()){
						auto const [min_iter, max_iter] =
							std::minmax_element(img.begin(), img.end());
						auto const min = *min_iter;
						auto const max = *max_iter;
						auto const diff = static_cast< double >(max) - min;
						auto const new_min = module("min"_param);
						auto const new_max = module("max"_param);
						auto const new_diff =
							static_cast< double >(new_max) - new_min;
						auto const scale = new_diff / diff;

						bitmap< type > result(img.size());
						std::transform(img.begin(), img.end(), result.begin(),
							[min, scale, new_min](auto const value){
								return static_cast< type >
									((value - min) * scale + new_min);
							});

						module("image"_out).push(std::move(result));
					}
				}
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
