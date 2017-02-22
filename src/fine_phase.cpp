//-----------------------------------------------------------------------------
// Copyright (c) 2015-2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "bitmap_sequence.hpp"
#include "make_string.hpp"

#include <disposer/module.hpp>

#include <boost/hana.hpp>
#include <boost/dll.hpp>

#include <cstdint>
#include <limits>


namespace disposer_module{ namespace fine_phase{


	namespace hana = boost::hana;


	using disposer::type_position_v;

	using intensity_type_list = disposer::type_list<
		std::uint8_t,
		std::uint16_t,
		std::uint32_t
	>;

	using phase_type_list = disposer::type_list<
		float,
		double
	>;

	enum class input_t{
		uint8,
		uint16,
		uint32
	};

	enum class output_t{
		float32,
		float64
	};

	struct parameter{
		output_t out_type;

		bool dark_environement;

		bool second_dir = false;
		bool have_bright = false;
		bool have_dark = false;
	};


	struct module: disposer::module_base{
		module(disposer::make_data const& data, parameter&& param):
			disposer::module_base(
				data,
				{
					slots.bright_image, slots.dark_image,
					slots.gray_code_dir1_images, slots.cos_dir1_images,
					slots.gray_code_dir2_images, slots.cos_dir2_images
				},
				{
					signals.fine_phase_dir1, signals.quality_image_dir1,
					signals.fine_phase_dir2, signals.quality_image_dir2
				}
			),
			param(std::move(param))
			{}


		template < typename T >
		bitmap< T > fine_phase(bitmap< T > const& image)const;


		struct{
			disposer::container_input< bitmap, intensity_type_list >
				bright_image{"bright_image"};

			disposer::container_input< bitmap, intensity_type_list >
				dark_image{"dark_image"};

			disposer::container_input< bitmap_vector, intensity_type_list >
				gray_code_dir1_images{"gray_code_dir1_images"};

			disposer::container_input< bitmap_vector, intensity_type_list >
				cos_dir1_images{"cos_dir1_images"};

			disposer::container_input< bitmap_vector, intensity_type_list >
				gray_code_dir2_images{"gray_code_dir2_images"};

			disposer::container_input< bitmap_vector, intensity_type_list >
				cos_dir2_images{"cos_dir2_images"};
		} slots;

		struct{
			disposer::container_output< bitmap, phase_type_list >
				fine_phase_dir1{"fine_phase_dir1"};

			disposer::container_output< bitmap, intensity_type_list >
				quality_image_dir1{"quality_image_dir1"};

			disposer::container_output< bitmap, phase_type_list >
				fine_phase_dir2{"fine_phase_dir2"};

			disposer::container_output< bitmap, intensity_type_list >
				quality_image_dir2{"quality_image_dir2"};
		} signals;


		void exec()override;


		void input_ready()override{
			// check if all enabled inputs have the same type

			// cos_dir1_images must have the enables types
			auto types = slots.cos_dir1_images.enabled_types();
			if(types.size() == 0){
				throw std::logic_error("no enabled inputs");
			}

			// others must have the same types enabled as cos_dir1_images
			// or none
			auto throw_if_diff_types = [this, &types]
				(auto const& input, auto const& input_types){
					if(types == input_types) return;

					std::ostringstream os;
					os << "different input types in '"
						<< slots.cos_dir1_images.name << "' (";
					for(auto& type: types){
						os << "'" << type.pretty_name() << "'";
					}

					os << ") and " << input.name << " (";
					for(auto& type: input_types){
						os << "'" << type.pretty_name() << "'";
					}

					os << ")";

					throw std::logic_error(os.str());
				};

			auto bright_types = slots.bright_image.enabled_types();
			auto dark_types = slots.dark_image.enabled_types();
			auto gray_dir1_types = slots.gray_code_dir1_images.enabled_types();
			auto cos_dir1_types = slots.cos_dir1_images.enabled_types();
			auto gray_dir2_types = slots.gray_code_dir2_images.enabled_types();
			auto cos_dir2_types = slots.cos_dir2_images.enabled_types();

			if(bright_types.size() > 0){
				throw_if_diff_types(slots.bright_image, bright_types);
				param.have_bright = true;
			}

			if(dark_types.size() > 0){
				throw_if_diff_types(slots.dark_image, dark_types);
				param.have_dark = true;
			}

			if(gray_dir1_types.size() == 0){
				throw std::logic_error(
					"calculation without gray code is not implemented yet"
				);
			}
			throw_if_diff_types(slots.bright_image, gray_dir1_types);

			if(cos_dir2_types.size() > 0){
				if(gray_dir2_types.size() == 0){
					throw std::logic_error(
						"calculation without gray code is not implemented yet"
					);
				}
				throw_if_diff_types(slots.cos_dir2_images, cos_dir2_types);
				throw_if_diff_types(
					slots.gray_code_dir2_images, gray_dir2_types);
				param.second_dir = true;
			}


			// enable outputs
			switch(param.out_type){
				case output_t::float32:
					signals.fine_phase_dir1.enable< float >();
					if(param.second_dir){
						signals.fine_phase_dir2.enable< float >();
					}
				break;
				case output_t::float64:
					signals.fine_phase_dir1.enable< double >();
					if(param.second_dir){
						signals.fine_phase_dir2.enable< double >();
					}
				break;
			}

			signals.quality_image_dir1.enable_types(types);
			if(param.second_dir){
				signals.quality_image_dir2.enable_types(types);
			}
		}


		parameter param;
	};


	template < typename T >
	bitmap< T > difference(bitmap< T > lhs, bitmap< T > const& rhs){
		std::transform(lhs.begin(), lhs.end(), rhs.begin(), lhs.begin(),
			[](auto l, auto r){
				return l > r ? l - r : T(0);
			});

		return lhs;
	}


	template < typename T >
	bitmap< T > calc_bright(bitmap_vector< T > const& cos_images){
		bitmap< T > result(cos_images[0].size());

		for(std::size_t i = 0; i < result.point_count(); ++i){
			double sum = 0;

			for(std::size_t j = 0; j < cos_images.size(); ++j){
				sum += cos_images[j].data()[i];
			}

			result.data()[i] = static_cast< T >(sum / cos_images.size());
		}

		return result;
	}


	template < typename IntensityT, typename PhaseT >
	std::pair< bitmap< PhaseT >, bitmap< IntensityT > > calc_phase(
		bitmap< IntensityT > const& diff,
		bitmap_vector< IntensityT > const& cos
	){

	}

	template < typename IntensityT >
	bitmap< std::uint16_t > calc_index(
		bitmap< IntensityT > const& diff,
		bitmap_vector< IntensityT > const& gray
	){

	}

	disposer::module_ptr make_module(disposer::make_data& data){
		if(data.is_first()) throw disposer::module_not_as_start(data);

		parameter param;

		data.params.set(param.dark_environement, "dark_environement", false);

		return std::make_unique< module >(data, std::move(param));
	}


	struct raw_data_set_t{
		struct none_t{ using value_type = none_t; };

		using bitmap_t = std::variant<
				none_t,
				disposer::input_data< bitmap< std::uint8_t > >,
				disposer::input_data< bitmap< std::uint16_t > >,
				disposer::input_data< bitmap< std::uint32_t > >
			>;

		using bitmap_vector_t = std::variant<
				none_t,
				disposer::input_data< bitmap_vector< std::uint8_t > >,
				disposer::input_data< bitmap_vector< std::uint16_t > >,
				disposer::input_data< bitmap_vector< std::uint32_t > >
			>;


		raw_data_set_t(bitmap_vector_t&& cos1):
			cos1(cos1){}

		bitmap_t bright;
		bitmap_t dark;
		bitmap_vector_t gray1;
		bitmap_vector_t cos1;
		bitmap_vector_t gray2;
		bitmap_vector_t cos2;
	};

	template < typename IntensityT >
	struct data_set_t{
		bitmap< IntensityT > bright;
		bitmap< IntensityT > dark;
		bitmap_vector< IntensityT > gray1;
		bitmap_vector< IntensityT > cos1;
		bitmap_vector< IntensityT > gray2;
		bitmap_vector< IntensityT > cos2;
	};

	using data_set_variant = std::variant<
			data_set_t< std::uint8_t >,
			data_set_t< std::uint16_t >,
			data_set_t< std::uint32_t >
		>;

	void module::exec(){
		std::multimap< std::size_t, raw_data_set_t > raw_data_sets;

		// get all the data
		auto bright_data = slots.bright_image.get();
		auto dark_data = slots.dark_image.get();
		auto gray1_data = slots.gray_code_dir1_images.get();
		auto cos1_data = slots.cos_dir1_images.get();
		auto gray2_data = slots.gray_code_dir2_images.get();
		auto cos2_data = slots.cos_dir2_images.get();

		// check count of puts in previous modules with respect to params
		if(
			(cos1_data.size() != gray1_data.size()) ||
			(param.second_dir && cos1_data.size() != cos2_data.size()) ||
			(param.second_dir && cos1_data.size() != gray2_data.size()) ||
			(param.have_dark && cos1_data.size() != dark_data.size()) ||
			(param.have_bright && cos1_data.size() != bright_data.size())
		){
			std::ostringstream os;
			os << "different count of input data (";

			if(param.have_bright){
				os << slots.bright_image.name << ": "
					<< bright_data.size() << ", ";
			}

			if(param.have_dark){
				os << slots.dark_image.name << ": "
					<< dark_data.size() << ", ";
			}

			os << slots.cos_dir1_images.name << ": "
				<< cos1_data.size() << ", ";

			os << slots.gray_code_dir1_images.name << ": "
				<< gray1_data.size();

			if(param.second_dir){
				os << ", " << slots.cos_dir2_images.name << ": "
					<< cos2_data.size() << ", ";

				os << slots.gray_code_dir2_images.name << ": "
					<< gray2_data.size();
			}

			os << ")";

			throw std::logic_error(os.str());
		}

		// id verificator
		auto check_ids = [](std::size_t id1, std::size_t id2){
				if(id1 == id2) return;

				// a very excotic error that should only appeare
				// if a previous module did set output.put() id's wrong
				throw std::logic_error("id mismatch");
			};


		// initialize the combined data set with cos1_data is the reference
		for(auto&& [id, images]: cos1_data){
			raw_data_sets.emplace(
				std::piecewise_construct,
				std::forward_as_tuple(id),
				std::forward_as_tuple(std::visit([](auto&& data){
					return raw_data_set_t::bitmap_vector_t(std::move(data));
				}, std::move(images))));
		}

		// add other inputs to the data_set
		auto iter = raw_data_sets.begin();
		for(auto&& [id, images]: gray1_data){
			check_ids(iter->first, id);
			iter->second.gray1 = std::visit([](auto&& data){
					return raw_data_set_t::bitmap_vector_t(std::move(data));
				}, std::move(images));
			++iter;
		}

		if(param.second_dir){
			iter = raw_data_sets.begin();
			for(auto&& [id, images]: cos2_data){
				check_ids(iter->first, id);
				iter->second.cos2 = std::visit([](auto&& data){
					return raw_data_set_t::bitmap_vector_t(std::move(data));
				}, std::move(images));
				++iter;
			}

			iter = raw_data_sets.begin();
			for(auto&& [id, images]: gray2_data){
				check_ids(iter->first, id);
				iter->second.gray2 = std::visit([](auto&& data){
					return raw_data_set_t::bitmap_vector_t(std::move(data));
				}, std::move(images));
				++iter;
			}
		}

		if(param.have_dark){
			iter = raw_data_sets.begin();
			for(auto&& [id, image]: dark_data){
				check_ids(iter->first, id);
				iter->second.dark = std::visit([](auto&& data){
					return raw_data_set_t::bitmap_t(std::move(data));
				}, std::move(image));
				++iter;
			}
		}

		if(param.have_bright){
			iter = raw_data_sets.begin();
			for(auto&& [id, image]: bright_data){
				check_ids(iter->first, id);
				iter->second.bright = std::visit([](auto&& data){
					return raw_data_set_t::bitmap_t(std::move(data));
				}, std::move(image));
				++iter;
			}
		}


		// check corresponding inputs for same data type and move them into a
		// single variant data set
		std::multimap< std::size_t, data_set_variant > calc_set;
		for(auto&& [id, data]: raw_data_sets){
			calc_set.emplace(id, std::visit(
				[this](
					auto&& bright, auto&& dark,
					auto&& gray1, auto&& cos1,
					auto&& gray2, auto&& cos2
				){
					using bright_t = typename
						std::remove_reference_t< decltype(bright) >
						::value_type::value_type;
					using dark_t = typename
						std::remove_reference_t< decltype(dark) >
						::value_type::value_type;
					using gray1_t = typename
						std::remove_reference_t< decltype(gray1) >
						::value_type::value_type::value_type;
					using cos1_t = typename
						std::remove_reference_t< decltype(cos1) >
						::value_type::value_type::value_type;
					using gray2_t = typename
						std::remove_reference_t< decltype(gray2) >
						::value_type::value_type::value_type;
					using cos2_t = typename
						std::remove_reference_t< decltype(cos2) >
						::value_type::value_type::value_type;

					using none_t = raw_data_set_t::none_t;

					// Check if all input data has the same value_type or none_t
					if(
						!std::is_same_v< cos1_t, none_t > ||
						std::is_same_v< cos1_t, gray1_t > ||
						((
							param.second_dir &&
							std::is_same_v< cos1_t, gray2_t >
						) && (
							!param.second_dir &&
							std::is_same_v< none_t, gray2_t >
						)) ||
						((
							param.second_dir &&
							std::is_same_v< cos1_t, cos2_t >
						) && (
							!param.second_dir &&
							std::is_same_v< none_t, cos2_t >
						)) ||
						((
							param.have_bright &&
							std::is_same_v< cos1_t, bright_t >
						) && (
							!param.have_bright &&
							std::is_same_v< none_t, bright_t >
						)) ||
						((
							param.have_dark &&
							std::is_same_v< cos1_t, dark_t >
						) && (
							!param.have_dark &&
							std::is_same_v< none_t, dark_t >
						))
					){
						throw std::logic_error("input type mismatch");
					}

					using value_type = cos1_t;

					if constexpr(!std::is_same_v< value_type, none_t >){
						data_set_t< value_type > result;

						if constexpr(std::is_same_v< cos1_t, bright_t >){
							result.bright = std::move(bright.data());
						}

						if constexpr(std::is_same_v< cos1_t, dark_t >){
							result.dark = std::move(dark.data());
						}

						if constexpr(std::is_same_v< cos1_t, gray1_t >){
							result.gray1 = std::move(gray1.data());
						}

						result.cos1 = std::move(cos1.data());

						if constexpr(std::is_same_v< cos1_t, gray2_t >){
							result.gray2 = std::move(gray2.data());
						}

						if constexpr(std::is_same_v< cos1_t, cos2_t >){
							result.cos2 = std::move(cos2.data());
						}

						return data_set_variant(result);
					}else{
						return data_set_variant();
					}
				},
				std::move(data.bright), std::move(data.dark),
				std::move(data.gray1), std::move(data.cos1),
				std::move(data.gray2), std::move(data.cos2)
			));
		}
	}


	void init(disposer::module_declarant& add){
		add("fine_phase", &make_module);
	}

	BOOST_DLL_AUTO_ALIAS(init)


} }
