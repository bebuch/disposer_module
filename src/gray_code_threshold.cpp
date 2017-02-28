//-----------------------------------------------------------------------------
// Copyright (c) 2015-2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#define _USE_MATH_DEFINES
#include "get_bitmap_size.hpp"

#include <disposer/module.hpp>

#include <bitmap/size_io.hpp>

#include <boost/dll.hpp>
#include <boost/hana.hpp>

#include <cstdint>
#include <numeric>
#include <limits>
#include <cmath>


namespace disposer_module{ namespace gray_code_threshold{


	namespace hana = boost::hana;

	using disposer::type_position_v;

	using type_list = disposer::type_list<
		std::uint8_t,
		std::uint16_t,
		std::uint32_t,
		float,
		double
	>;


	enum class mode_t{
		bright,
		bright_dark,
		cos
	};

	struct parameter{
		bool dark_environment;
		mode_t mode;
	};


	struct module: disposer::module_base{
		module(disposer::make_data const& data, parameter&& param):
			disposer::module_base(
				data,
				{slots.bright_image, slots.dark_image, slots.cos_images},
				{signals.threshold_image}
			),
			param(std::move(param))
			{}


		struct{
			disposer::container_input< bitmap, type_list >
				bright_image{"bright_image"};

			disposer::container_input< bitmap, type_list >
				dark_image{"dark_image"};

			disposer::container_input< bitmap_vector, type_list >
				cos_images{"cos_images"};
		} slots;

		struct{
			disposer::container_output< bitmap, type_list >
				threshold_image{"threshold_image"};
		} signals;


		void input_ready()override;


		void exec()override;


		parameter param;
	};



	disposer::module_ptr make_module(disposer::make_data& data){
		if(data.is_first()) throw disposer::module_not_as_start(data);

		parameter param;
		data.params.set(param.dark_environment, "dark_environment", false);

		return std::make_unique< module >(data, std::move(param));
	}


	void module::input_ready(){
		auto types_from_bitmap = [](auto type){
			return hana::type_c<
				typename decltype(type)::type::value_type >;
		};

		auto types_from_vector = [](auto type){
			return hana::type_c<
				typename decltype(type)::type::value_type::value_type >;
		};

		auto bright_types =
			slots.bright_image.enabled_types_transformed(types_from_bitmap);
		auto dark_types =
			slots.dark_image.enabled_types_transformed(types_from_bitmap);
		auto cos_types =
			slots.cos_images.enabled_types_transformed(types_from_vector);

		if(!cos_types.empty()){
			param.mode = mode_t::cos;
			if(!bright_types.empty() || !dark_types.empty()){
				throw std::logic_error("input '" + slots.cos_images.name
					+ "' must not be used together with other inputs");
			}
		}else if(!dark_types.empty()){
			param.mode = mode_t::bright_dark;
			if(bright_types.empty()){
				throw std::logic_error("input '" + slots.dark_image.name
					+ "' needs input '" + slots.bright_image.name
					+ "' as well");
			}

			if(dark_types != bright_types){
				throw std::logic_error("inputs '" + slots.bright_image.name
					+ "' and '" + slots.dark_image.name
					+ "' must have the same types enabled");
			}
		}else if(!bright_types.empty()){
			param.mode = mode_t::bright;
			if(!param.dark_environment){
				throw std::logic_error("input '" + slots.bright_image.name
					+ "' without input '" + slots.dark_image.name
					+ "' needs parameter dark_environment to be true");
			}
		}else{
			throw std::logic_error("need at least one input, use inputs '"
				+ slots.bright_image.name + "', '" + slots.dark_image.name
				+ "' or '" + slots.cos_images.name + "'");
		}

		switch(param.mode){
			case mode_t::bright:
			case mode_t::bright_dark:
				signals.threshold_image.enable_types(
					slots.bright_image.enabled_types());
			break;
			case mode_t::cos:
				signals.threshold_image.enable_types(
					slots.cos_images.enabled_types_transformed(
						types_from_bitmap));
			break;
		}
	}


	void module::exec(){
		switch(param.mode){
			case mode_t::bright:{
				for(auto& [id, img]: slots.bright_image.get()){
					(void)id;
					std::visit([this](auto& input_data){
						auto bitmap = input_data.data();
						using value_type = typename decltype(bitmap)
							::value_type;
						for(auto& v: bitmap) v /= 2;
						signals.threshold_image.put< value_type >(bitmap);
					}, img);
				}
			}break;
			case mode_t::bright_dark:{
				// get all the data
				auto bright_data = slots.bright_image.get();
				auto dark_data = slots.dark_image.get();

				// check count of puts in previous modules
				if(bright_data.size() != dark_data.size()){
					std::ostringstream os;
					os << "different count of input data ("
						<< slots.bright_image.name << ": " << bright_data.size()
						<< ", "
						<< slots.dark_image.name << ": " << dark_data.size()
						<< ")";

					throw std::logic_error(os.str());
				}

				// id verificator
				auto check_ids = [](std::size_t id1, std::size_t id2){
						if(id1 == id2) return;

						// a very excotic error that should only appeare
						// if a previous module did set output.put() id's wrong
						throw std::logic_error("id mismatch");
					};

				// check corresponding inputs for same data type and process
				auto dark_iter = dark_data.begin();
				for(
					auto bright_iter = bright_data.begin();
					bright_iter != bright_data.end();
					++bright_iter, ++dark_iter
				){
					auto& [bright_id, bright_img] = *bright_iter;
					auto& [dark_id, dark_img] = *dark_iter;

					check_ids(bright_id, dark_id);

					std::visit(
						[this](auto& bright_input, auto& dark_input){
							using bright_t = typename
								std::remove_reference_t< decltype(bright_input)
									>::value_type::value_type;
							using dark_t = typename
								std::remove_reference_t< decltype(dark_input)
									>::value_type::value_type;

							if constexpr(std::is_same_v< bright_t, dark_t >){
								auto bright = bright_input.data();
								auto& dark = dark_input.data();
								std::transform(
									bright.begin(), bright.end(),
									dark.begin(), bright.begin(),
									[](auto bright, auto dark){
										return bright > dark
											? dark + (bright - dark) / 2
											: 0;
									});
								signals.threshold_image.put< bright_t >(bright);
							}else{
								throw std::logic_error("input type mismatch");
							}
						},
						bright_img,
						dark_img
					);
				}
			}break;
			case mode_t::cos:{
				for(auto& [id, img]: slots.cos_images.get()){
					(void)id;
					std::visit([this](auto& input_data){
						auto vector = input_data.data();
						using value_type = typename decltype(vector)
							::value_type::value_type;
						auto const size = get_bitmap_size(vector);
						bitmap< value_type > result(size);
						for(std::size_t i = 0; i < result.point_count(); ++i){
							std::common_type_t< value_type, std::int64_t >
								sum = 0;

							for(std::size_t j = 0; j < vector.size(); ++j){
								sum += vector[j].data()[i];
							}

							result.data()[i] = static_cast< value_type >(
								sum / vector.size()
							);
						}
						signals.threshold_image.put< value_type >(result);
					}, img);
				}
			}break;
		}
	}


	void init(disposer::module_declarant& add){
		add("gray_code_threshold", &make_module);
	}

	BOOST_DLL_AUTO_ALIAS(init)


} }
