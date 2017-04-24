//-----------------------------------------------------------------------------
// Copyright (c) 2015-2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "bitmap_vector.hpp"

#include <disposer/module.hpp>

#include <io_tools/make_string.hpp>

#include <bitmap/pixel.hpp>

#include <boost/spirit/home/x3.hpp>
#include <boost/hana.hpp>
#include <boost/dll.hpp>

#include <iostream>


template <>
struct disposer::parameter_cast< std::vector< bitmap::pixel::rgb8u > >{
	std::vector< bitmap::pixel::rgb8u >
	operator()(std::string const& value)const{
		namespace x3 = boost::spirit::x3;

		auto xdigit2value = [](char c)->unsigned{
			if(c >= '0' || c <='9') return c - '0';
			if(c >= 'a' || c <='f') return c - 'a';
			if(c >= 'A' || c <='F') return c - 'A';
			assert(false);
		};

		std::vector< bitmap::pixel::rgb8u > colors;
		auto add = [&colors, xdigit2value](auto& ctx){
			auto hex_vector6 = _attr(ctx);
			assert(hex_vector6.size() == 6);
			bitmap::pixel::rgb8u rgb;
			rgb.r =
				(xdigit2value(hex_vector6[0]) << 4) |
				(xdigit2value(hex_vector6[1]));
			rgb.g =
				(xdigit2value(hex_vector6[2]) << 4) |
				(xdigit2value(hex_vector6[3]));
			rgb.b =
				(xdigit2value(hex_vector6[4]) << 4) |
				(xdigit2value(hex_vector6[5]));
			colors.push_back(rgb);
		};

		auto first = value.begin();
		auto const last = value.end();
		bool const match = x3::phrase_parse(first, last,
			(x3::lit('{')
				>> (x3::no_skip[x3::lit('#') >> x3::repeat(6)[x3::xdigit]][add]
					% ',')
				>> x3::lit('}')),
			x3::space);

		if(!match || first != last){
			throw std::logic_error(
				"expected '{#RRGGBB, ...}' (HTML color notation) but got '"
				+ value + "'");
		}

		return colors;
	}
};



namespace disposer_module{ namespace dye_bitmaps{


	namespace pixel = ::bitmap::pixel;
	namespace hana = boost::hana;

	using input_type_list = disposer::type_list<
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

	using output_type_list = disposer::type_list<
		pixel::rgb8,
		pixel::rgb16,
		pixel::rgb32,
		pixel::rgb64,
		pixel::rgb8u,
		pixel::rgb16u,
		pixel::rgb32u,
		pixel::rgb64u,
		pixel::rgb32f,
		pixel::rgb64f
	>;



	struct parameter{
		std::vector< pixel::rgb8u > colors;
	};


	struct module: disposer::module_base{
		module(disposer::make_data const& data, parameter&& param):
			disposer::module_base(
				data,
				{slots.image_vector},
				{signals.image_vector}
			),
			param(std::move(param))
			{}


		struct{
			disposer::container_input< bitmap_vector, input_type_list >
				image_vector{"image_vector"};
		} slots;

		struct{
			disposer::container_output< bitmap_vector, output_type_list >
				image_vector{"image_vector"};
		} signals;


		void exec()override;


		template < typename T >
		using rgb_bitmap = bitmap< pixel::basic_rgb< T > >;

		template < typename T >
		rgb_bitmap< T > dye(
			bitmap< T > const& image, pixel::rgb8u const& color)const;


		void input_ready()override{
			signals.image_vector.enable_types(
				slots.image_vector.enabled_types_transformed(
					[](auto type){
						using vector = typename decltype(type)::type;
						using value = typename vector::value_type::value_type;
						using pixel_vector = std::vector< rgb_bitmap< value > >;
						return hana::type_c< pixel_vector >;
					}
				)
			);
		}


		parameter const param;
	};


	disposer::module_ptr make_module(disposer::make_data& data){
		if(data.is_first()) throw disposer::module_not_as_start(data);

		parameter param;

		data.params.set(param.colors, "colors");

		return std::make_unique< module >(data, std::move(param));
	}


	template < typename T >
	module::rgb_bitmap< T > module::dye(
		bitmap< T > const& image,
		pixel::rgb8u const& rgb
	)const{
		rgb_bitmap< T > result(image.size());

		std::transform(image.begin(), image.end(), result.begin(),
			[&rgb](T value){
				return pixel::basic_rgb< T >{
					static_cast< T >(value / 256. * rgb.r),
					static_cast< T >(value / 256. * rgb.g),
					static_cast< T >(value / 256. * rgb.b)
				};
			});

		return result;
	}


	void module::exec(){
		for(auto const& [id, vec]: slots.image_vector.get()){
			(void)id;
			std::visit([this](auto const& data){
				auto const& vec = data.data();

				using vector_t = std::decay_t< decltype(vec) >;
				using value_type = typename vector_t::value_type::value_type;
				using rgb_bitmap_vector =
					std::vector< rgb_bitmap< value_type > >;

				if(vec.size() != param.colors.size()){
					throw std::runtime_error(io_tools::make_string(
						"Got ", vec.size(), " bitmaps, but parameter 'colors'"
						"defines ", param.colors.size(), " colors"
					));
				}

				rgb_bitmap_vector result;
				result.reserve(vec.size());
				for(std::size_t i = 0; i < vec.size(); ++i){
					result.push_back(dye(vec[i], param.colors[i]));
				}

				signals.image_vector.put< pixel::basic_rgb< value_type > >(
					std::move(result));
			}, vec);
		}
	}


	void init(disposer::module_declarant& add){
		add("dye_bitmaps", &make_module);
	}

	BOOST_DLL_AUTO_ALIAS(init)


} }
