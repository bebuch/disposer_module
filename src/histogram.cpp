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


	template < typename T, typename IndexFn >
	void histogram_count(
		std::vector< std::size_t >& histogram,
		bitmap< T > const& image,
		IndexFn const& calc_index
	){
		for(auto v: image){
			if constexpr(std::is_floating_point_v< T >){
				if(std::isnan(v)) continue;
			}

			++histogram[calc_index(v)];
		}
	}


	template < typename T >
	struct counter{
		std::vector< std::size_t >& histogram;
		bitmap< T > const& image;
		T const min;
		T const max;

		template < typename IndexFn >
		void operator()(IndexFn const& calc_index){
			auto const min_check_fn = [min = min](auto const& fn){
					return [&fn, min](auto v){ return fn(std::max(v, min)); };
				};
			auto const max_check_fn = [max = max](auto const& fn){
					return [&fn, max](auto v){ return fn(std::min(v, max)); };
				};
			auto const minmax_check_fn =
				[&min_check_fn, &max_check_fn](auto const& fn){
					return max_check_fn(min_check_fn(fn));
				};

			bool need_min_check = std::is_floating_point_v< T > ? true
				: std::numeric_limits< T >::min() < min;
			bool need_max_check = std::is_floating_point_v< T > ? true
				: std::numeric_limits< T >::max() > max;

			if(need_min_check && need_max_check){
				histogram_count(histogram, image, minmax_check_fn(calc_index));
			}else if(need_max_check){
				histogram_count(histogram, image, max_check_fn(calc_index));
			}else if(need_min_check){
				histogram_count(histogram, image, min_check_fn(calc_index));
			}else{
				histogram_count(histogram, image, calc_index);
			}
		}
	};


	template < typename T, bool = std::is_integral_v< T > >
	struct make_diff_type{ using type = T; };

	template < typename T >
	struct make_diff_type< T, true >{ using type = std::make_unsigned_t< T >; };

	template < typename T >
	using make_diff_type_t = typename make_diff_type< T >::type;


	template < typename Module, typename T >
	std::vector< std::size_t > exec(
		Module const& module,
		bitmap< T > const& image
	){
		auto const min = module("min"_param).get(hana::type_c< T >);
		auto const max = module("max"_param).get(hana::type_c< T >);

		// for signed integer types diff may not fit in singed range but it
		// fits in ever in the corresponding unsigned range
		using diff_type = make_diff_type_t< T >;

		auto const diff = static_cast< diff_type >(max - min);
		auto const bin_count = module("bin_count"_param).get();
		auto const max_index = bin_count - 1;

		std::vector< std::size_t > result(bin_count);
		counter< T > calc{result, image, min, max};
		if(min == 0 && diff == max_index){
			calc([](auto v){ return static_cast< std::size_t >(v); });
		}else{
			calc([min, max_index, diff](auto v){
					auto const v0 = static_cast< diff_type >(v - min);
					return static_cast< std::size_t >(v0 * max_index / diff);
				});
		}

		if(module("cumulative"_param).get()){
			std::size_t sum = 0;
			for(auto& v: result){
				sum += v;
				v = sum;
			}
		}

		return result;
	}


	void init(std::string const& name, module_declarant& disposer){
		auto init = module_register_fn(
			module_configure(
				"image"_in(types, template_transform_c< bitmap >),
				"histogram"_out(hana::type_c< std::vector< std::size_t > >),
				"cumulative"_param(hana::type_c< bool >, default_values(false)),
				"min"_param(types, enable_by_types_of("image"_in)
					// TODO: Default for 8 and 16 bit types
				),
				"max"_param(types, enable_by_types_of("image"_in),
					value_verify([](auto const& iop, auto const& max){
						auto const min =
							iop("min"_param).get(hana::typeid_(max));
						if(min < max) return;
						throw std::logic_error("max must be greater min");
					})
					// TODO: Default for 8 and 16 bit types
				),
				"bin_count"_param(hana::type_c< std::size_t >
					// TODO: Default for 8 and 16 bit types
				)
			),
			module_enable([]{
				return [](auto& module){
					auto values = module("image"_in).get_references();
					for(auto const& value: values){
						std::visit([&module](auto const& img_ref){
							module("histogram"_out).put(
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
