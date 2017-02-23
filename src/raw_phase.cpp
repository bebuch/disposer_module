//-----------------------------------------------------------------------------
// Copyright (c) 2015-2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#define _USE_MATH_DEFINES
#include "bitmap_sequence.hpp"

#include <disposer/module.hpp>

#include <bitmap/size_io.hpp>

#include <boost/dll.hpp>
#include <boost/hana.hpp>

#include <cstdint>
#include <limits>
#include <cmath>


namespace disposer_module{ namespace raw_phase{


	namespace hana = boost::hana;


	using disposer::type_position_v;

	using intensity_type_list = disposer::type_list<
		std::uint8_t,
		std::uint16_t,
		std::uint32_t,
		float,
		double
	>;

	using phase_type_list = disposer::type_list<
		float,
		double
	>;

	enum class input_t{
		uint8,
		uint16,
		uint32,
		float32,
		float64
	};

	enum class output_t{
		float32,
		float64
	};

	struct parameter{
		output_t out_type;

		bool dark_environement;
		bool have_dark; // needed if parameter dark_environement != true
	};


	template < typename IntensityT >
	struct data_set_with_dark{
		using value_type = IntensityT;

		bitmap< IntensityT > bright;
		bitmap< IntensityT > dark;
		bitmap_vector< IntensityT > cos;
	};

	template < typename IntensityT >
	struct data_set_without_dark{
		using value_type = IntensityT;

		bitmap< IntensityT > bright;
		bitmap_vector< IntensityT > cos;
	};


	struct module: disposer::module_base{
		module(disposer::make_data const& data, parameter&& param):
			disposer::module_base(
				data,
				{slots.bright_image, slots.dark_image, slots.cos_images},
				{signals.phase_image, signals.modulation_image}
			),
			param(std::move(param))
			{}


		template < typename PhaseT, typename IntensityT >
		std::tuple< bitmap< PhaseT >, bitmap< IntensityT > >
		decode(
			data_set_with_dark< IntensityT > const& data
		)const;

		template < typename PhaseT, typename IntensityT >
		std::tuple< bitmap< PhaseT >, bitmap< IntensityT > >
		decode(
			data_set_without_dark< IntensityT > const& data
		)const;


		struct{
			disposer::container_input< bitmap, intensity_type_list >
				bright_image{"bright_image"};

			disposer::container_input< bitmap, intensity_type_list >
				dark_image{"dark_image"};

			disposer::container_input< bitmap_vector, intensity_type_list >
				cos_images{"cos_images"};
		} slots;

		struct{
			disposer::container_output< bitmap, phase_type_list >
				phase_image{"phase_image"};

			disposer::container_output< bitmap, intensity_type_list >
				modulation_image{"modulation_image"};
		} signals;


		void input_ready()override;


		void exec()override;


		parameter param;
	};


	template < typename SizeT, typename T >
	inline void check_size(
		SizeT const& size,
		bitmap< T > const& img,
		std::string_view img_name
	){
		if(size == img.size()) return;

		std::ostringstream os;
		os << "different size in bright image (" << size
			<< ") and " << img_name << " (" << img.size()
			<< ") image";
		throw std::logic_error(os.str());
	}

	template < typename SizeT, typename T >
	inline void check_cos_images(
		SizeT const& size,
		bitmap_vector< T > const& images
	){
		for(auto& img: images){
			check_size(size, img, "cos");
		}
	}



	template < typename T >
	inline T save_atan2(T n, T d){
		using std::atan2;
		if(n == 0 && d == 0){
			return T(M_PI) / 2;
		}else{
			return atan2(n, d);
		}
	}


	/// Calculate roh and modulation (90 degrees phase shift)
	template < typename PhaseT, typename IntensityT >
	inline std::tuple< PhaseT, IntensityT > roh_and_modulation(
		std::array< IntensityT, 4 > const& v
	){
		using std::sqrt;

		auto n = static_cast< PhaseT >(v[1] - v[3]);
		auto d = static_cast< PhaseT >(v[2] - v[0]);

		return std::make_tuple(
			save_atan2(n, d),
			static_cast< IntensityT >(sqrt(n * n + d * d))
		);
	}

	/// Calculate roh and modulation (60 degrees phase shift)
	template < typename PhaseT, typename IntensityT >
	inline std::tuple< PhaseT, IntensityT > roh_and_modulation(
		std::array< IntensityT, 6 > const& v
	){
		using std::sqrt;

		static PhaseT const c_sqrt_0_75 = sqrt(PhaseT(0.75));
		static PhaseT constexpr c_0_5(0.5);

		auto n = c_sqrt_0_75 *
			static_cast< PhaseT >(v[2] + v[1] - v[5] - v[4]);
		auto d = c_0_5 *
			static_cast< PhaseT >(v[4] - v[5] - v[1] + v[2] + v[3] - v[0]);

		return std::make_tuple(
			save_atan2(n, d),
			static_cast< IntensityT >(sqrt(n * n + d * d) / PhaseT(1.5))
		);
	}

	/// Calculate roh and modulation (45 degrees phase shift)
	template < typename PhaseT, typename IntensityT >
	inline std::tuple< PhaseT, IntensityT > roh_and_modulation(
		std::array< IntensityT, 8 > const& v
	){
		using std::sqrt;

		static PhaseT const c_sqrt_2_d_2 = sqrt(PhaseT(2)) / 2;

		PhaseT n = (v[5] + v[7] - v[3] - v[1]) * c_sqrt_2_d_2 + v[6] - v[2];
		PhaseT d = (v[1] + v[7] - v[3] - v[5]) * c_sqrt_2_d_2 + v[0] - v[4];

		return std::make_tuple(
			save_atan2(n, d),
			static_cast< IntensityT >(sqrt(n * n + d * d) / 2)
		);
	}

	/// Calculate roh and modulation (22.5 degrees phase shift)
	template < typename PhaseT, typename IntensityT >
	inline std::tuple< PhaseT, IntensityT > roh_and_modulation(
		std::array< IntensityT, 16 > const& v
	){
		using std::sqrt;

		static PhaseT const c_sqrt_2 = sqrt(PhaseT(2));
		static PhaseT const c1 = sqrt(2 + c_sqrt_2);
		static PhaseT const c2 = sqrt(2 - c_sqrt_2);

		PhaseT n =
			c2 * v[1] + c_sqrt_2 * v[2] + c1 * v[3] + 2 * v[4] +
			c1 * v[5] + c_sqrt_2 * v[6] + c2 * v[7] - c2 * v[9] -
			c_sqrt_2 * v[10] - c1 * v[11] - 2 * v[12] - c1 * v[13] -
			c_sqrt_2 * v[14] - c2 * v[15];

		PhaseT d = -2 * v[0] - c1 * v[1] - c_sqrt_2 * v[2] - c2 * v[3] +
			c2 * v[5] + c_sqrt_2 * v[6] + c1 * v[7] + 2 * v[8] + c1 * v[9] +
			c_sqrt_2 * v[10] + c2 * v[11] - c2 * v[13] - c_sqrt_2 * v[14] -
			c1 * v[15];

		return std::make_tuple(
			save_atan2(n, d),
			static_cast< IntensityT >(sqrt(n * n + d * d) / 8)
		);
	}

	template < typename PhaseT, typename IntensityT >
	inline std::tuple< PhaseT, IntensityT > roh_and_modulation(
		bitmap_vector< IntensityT > const& cos,
		std::size_t x, std::size_t y
	){
		switch(cos.size()){
		case 4:
			return roh_and_modulation< PhaseT >(std::array< IntensityT, 4 >{{
					cos[ 0](x, y), cos[ 1](x, y),
					cos[ 2](x, y), cos[ 3](x, y)
				}});
		break;
		case 6:
			return roh_and_modulation< PhaseT >(std::array< IntensityT, 6 >{{
					cos[ 0](x, y), cos[ 1](x, y),
					cos[ 2](x, y), cos[ 3](x, y),
					cos[ 4](x, y), cos[ 5](x, y)
				}});
		break;
		case 8:
			return roh_and_modulation< PhaseT >(std::array< IntensityT, 8 >{{
					cos[ 0](x, y), cos[ 1](x, y),
					cos[ 2](x, y), cos[ 3](x, y),
					cos[ 4](x, y), cos[ 5](x, y),
					cos[ 6](x, y), cos[ 7](x, y)
				}});
		break;
		case 16:
			return roh_and_modulation< PhaseT >(std::array< IntensityT, 16 >{{
					cos[ 0](x, y), cos[ 1](x, y),
					cos[ 2](x, y), cos[ 3](x, y),
					cos[ 4](x, y), cos[ 5](x, y),
					cos[ 6](x, y), cos[ 7](x, y),
					cos[ 8](x, y), cos[ 9](x, y),
					cos[10](x, y), cos[11](x, y),
					cos[12](x, y), cos[13](x, y),
					cos[14](x, y), cos[14](x, y)
				}});
		break;
		default:
			throw std::logic_error(
				"raw_phase calculation is only implemented for 4, 6, "
				"8 and 16 cos images, you have "
				+ std::to_string(cos.size()));
		}
	}


	template < typename PhaseT, typename IntensityT >
	std::tuple< bitmap< PhaseT >, bitmap< IntensityT > >
	module::decode(
		data_set_with_dark< IntensityT > const& data
	)const{
		auto size = data.bright.size();
		check_size(size, data.dark, "dark");
		check_cos_images(size, data.cos);

		bitmap< PhaseT > phase(size,
			std::numeric_limits< PhaseT >::quiet_NaN());
		bitmap< IntensityT > modulation(size);

		for(std::size_t y = 0; y < size.height(); ++y){
			for(std::size_t x = 0; x < size.width(); ++x){
				std::uint64_t const min = data.dark(x, y);
				std::uint64_t const max = data.bright(x, y);
				if(min >= max) continue;

				std::tie(phase(x, y), modulation(x, y)) =
					roh_and_modulation< PhaseT >(data.cos, x, y);
			}
		}

		return std::make_tuple(std::move(phase), std::move(modulation));
	}

	template < typename PhaseT, typename IntensityT >
	std::tuple< bitmap< PhaseT >, bitmap< IntensityT > >
	module::decode(
		data_set_without_dark< IntensityT > const& data
	)const{
		auto size = data.bright.size();
		check_cos_images(size, data.cos);

		bitmap< PhaseT > phase(size,
			std::numeric_limits< PhaseT >::quiet_NaN());
		bitmap< IntensityT > modulation(size);

		for(std::size_t y = 0; y < size.height(); ++y){
			for(std::size_t x = 0; x < size.width(); ++x){
				auto const max = data.bright(x, y) / 2;
				if(max <= 0) continue;

				std::tie(phase(x, y), modulation(x, y)) =
					roh_and_modulation< PhaseT >(data.cos, x, y);
			}
		}

		return std::make_tuple(std::move(phase), std::move(modulation));
	}


	disposer::module_ptr make_module(disposer::make_data& data){
		if(data.is_first()) throw disposer::module_not_as_start(data);

		parameter param;

		data.params.set(param.dark_environement, "dark_environement", false);

		static std::unordered_map< std::string_view, output_t > const
			output_types{
				{"float32", output_t::float32},
				{"float64", output_t::float64}
			};

		std::string output_type;
		data.params.set(output_type, "output_type", "float32");

		auto iter = output_types.find(std::string_view(output_type));
		if(iter == output_types.end()){
			std::ostringstream os;
			os << "parameter output_type with unknown value '" << output_type
				<< "', valid values are: ";

			bool first = true;
			for(auto const& type: output_types){
				if(!first){ os << ", "; }else{ first = false; }
				os << "'" << type.first;
			}

			throw std::logic_error(os.str());
		}

		param.out_type = iter->second;

		return std::make_unique< module >(data, std::move(param));
	}


	void module::input_ready(){
		auto bitmap_value_type = [](auto type){
			return hana::type_c<
				typename decltype(type)::type::value_type >;
		};

		auto bitmap_vector_value_type = [](auto type){
			return hana::type_c<
				typename decltype(type)::type::value_type::value_type >;
		};

		// bright_image must have enabled types
		auto bright_types = slots.bright_image.enabled_types_transformed(
			bitmap_value_type
		);
		if(bright_types.size() == 0){
			throw std::logic_error(
				"input " + slots.bright_image.name + " enabled types");
		}

		// others must have the same types enabled as bright_image
		// or none
		auto throw_if_diff_types = [this, &bright_types]
			(auto const& input, auto const& input_types){
				if(bright_types == input_types) return;

				std::ostringstream os;
				os << "different input types in '"
					<< slots.bright_image.name << "' (";
				for(auto& type: bright_types){
					os << "'" << type.pretty_name() << "'";
				}

				os << ") and " << input.name << " (";
				for(auto& type: input_types){
					os << "'" << type.pretty_name() << "'";
				}

				os << ")";

				throw std::logic_error(os.str());
			};

		auto dark_types = slots.dark_image.enabled_types_transformed(
			bitmap_value_type
		);
		auto cos_types = slots.cos_images.enabled_types_transformed(
			bitmap_vector_value_type
		);

		throw_if_diff_types(slots.cos_images, cos_types);

		if(dark_types.size() > 0){
			throw_if_diff_types(slots.dark_image, dark_types);
			param.have_dark = true;
		}else if(!param.dark_environement){
			throw std::logic_error(
				"if parameter dark_environement is false (which is the "
				"default) you need a dark image"
			);
		}

		// enable output
		switch(param.out_type){
			case output_t::float32:
				signals.phase_image.enable< float >();
			break;
			case output_t::float64:
				signals.phase_image.enable< double >();
			break;
		}
		signals.modulation_image.enable_types(
			slots.bright_image.enabled_types());
	}



	using data_set_variant_with_dark = std::variant<
			data_set_with_dark< std::uint8_t >,
			data_set_with_dark< std::uint16_t >,
			data_set_with_dark< std::uint32_t >,
			data_set_with_dark< float >,
			data_set_with_dark< double >
		>;

	using data_set_variant_without_dark = std::variant<
			data_set_without_dark< std::uint8_t >,
			data_set_without_dark< std::uint16_t >,
			data_set_without_dark< std::uint32_t >,
			data_set_without_dark< float >,
			data_set_without_dark< double >
		>;


	void module::exec(){
		using bitmap_variant =
			disposer::container_input< bitmap, intensity_type_list >
			::value_type;

		using bitmap_vector_variant =
			disposer::container_input< bitmap_vector, intensity_type_list >
			::value_type;


		// id verificator
		auto check_ids = [](std::size_t id1, std::size_t id2){
				if(id1 == id2) return;

				// a very excotic error that should only appeare
				// if a previous module did set output.put() id's wrong
				throw std::logic_error("id mismatch");
			};

		auto decode32 = [this](auto const& data){
			using value_type =
				typename std::remove_reference_t< decltype(data) >::value_type;
			auto [phase, modulation] = decode< float >(data);
			signals.phase_image.put< float >(phase);
			signals.modulation_image.put< value_type >(modulation);
		};
		auto decode64 = [this](auto const& data){
			using value_type =
				typename std::remove_reference_t< decltype(data) >::value_type;
			auto [phase, modulation] = decode< double >(data);
			signals.phase_image.put< double >(phase);
			signals.modulation_image.put< value_type >(modulation);
		};


		if(param.have_dark){
			// get all the data
			auto bright_data = slots.bright_image.get();
			auto dark_data = slots.dark_image.get();
			auto cos_data = slots.cos_images.get();

			// check count of puts in previous modules with respect to params
			if(
				(bright_data.size() != dark_data.size()) ||
				(bright_data.size() != cos_data.size())
			){
				std::ostringstream os;
				os << "different count of input data ("
					<< slots.bright_image.name << ": " << bright_data.size()
					<< ", "
					<< slots.dark_image.name << ": " << dark_data.size()
					<< ", "
					<< slots.cos_images.name << ": " << cos_data.size()
					<< ")";

				throw std::logic_error(os.str());
			}

			// combine data in a single variant type
			using raw_data_set = std::tuple<
					bitmap_variant,
					bitmap_variant,
					bitmap_vector_variant
				>;
			std::multimap< std::size_t, raw_data_set > raw_data_sets;
			auto dark_iter = dark_data.begin();
			auto cos_iter = cos_data.begin();
			for(
				auto bright_iter = bright_data.begin();
				bright_iter != bright_data.end();
				++bright_iter, ++dark_iter, ++cos_iter
			){
				auto&& [bright_id, bright_img] = *bright_iter;
				auto&& [dark_id, dark_img] = *dark_iter;
				auto&& [cos_id, cos_img] = *cos_iter;

				check_ids(bright_id, dark_id);
				check_ids(bright_id, cos_id);

				raw_data_sets.emplace(
					std::piecewise_construct,
					std::forward_as_tuple(id),
					std::forward_as_tuple(bright_img, dark_img, cos_img));
			}

			// check corresponding inputs for same data type and move them
			// into a single variant data set
			std::multimap< std::size_t, data_set_variant_with_dark >
				calc_sets;
			for(auto&& [id, data]: raw_data_sets){
				calc_sets.emplace(id, std::visit(
					[this](auto&& bright, auto&& dark, auto&& cos)
					->data_set_variant_with_dark{
						using bright_t = typename
							std::remove_reference_t< decltype(bright) >
							::value_type::value_type;
						using dark_t = typename
							std::remove_reference_t< decltype(dark) >
							::value_type::value_type;
						using cos_t = typename
							std::remove_reference_t< decltype(cos) >
							::value_type::value_type::value_type;

						if constexpr(
							std::is_same_v< bright_t, dark_t > &&
							std::is_same_v< bright_t, cos_t >
						){
							return data_set_with_dark< bright_t >{
								bright.data(),
								dark.data(),
								cos.data()
							};
						}else{
							throw std::logic_error("input type mismatch");
						}
					},
					std::move(std::get< 0 >(data)),
					std::move(std::get< 1 >(data)),
					std::move(std::get< 2 >(data))
				));
			}

			// exec calculation
			for(auto& [id, calc_set]: calc_sets){
				(void)id;
				switch(param.out_type){
					case output_t::float32:
						std::visit(decode32, calc_set);
					break;
					case output_t::float64:
						std::visit(decode64, calc_set);
					break;
				}

			}
		}else{
			// get all the data
			auto bright_data = slots.bright_image.get();
			auto cos_data = slots.cos_images.get();

			// check count of puts in previous modules with respect to params
			if(bright_data.size() != cos_data.size()){
				std::ostringstream os;
				os << "different count of input data ("
					<< slots.bright_image.name << ": " << bright_data.size()
					<< ", "
					<< slots.cos_images.name << ": " << cos_data.size()
					<< ")";

				throw std::logic_error(os.str());
			}

			// combine data in a single variant type
			using raw_data_set = std::tuple<
					bitmap_variant,
					bitmap_vector_variant
				>;
			std::multimap< std::size_t, raw_data_set > raw_data_sets;
			auto cos_iter = cos_data.begin();
			for(
				auto bright_iter = bright_data.begin();
				bright_iter == bright_data.end();
				++bright_iter, ++cos_iter
			){
				auto&& [bright_id, bright_img] = *bright_iter;
				auto&& [cos_id, cos_img] = *cos_iter;

				check_ids(bright_id, cos_id);

				raw_data_sets.emplace(
					std::piecewise_construct,
					std::forward_as_tuple(id),
					std::forward_as_tuple(bright_img, cos_img));
			}

			// check corresponding inputs for same data type and move them
			// into a single variant data set
			std::multimap< std::size_t, data_set_variant_without_dark >
				calc_sets;
			for(auto&& [id, data]: raw_data_sets){
				calc_sets.emplace(id, std::visit(
					[this](auto&& bright, auto&& cos)
					->data_set_variant_without_dark{
						using bright_t = typename
							std::remove_reference_t< decltype(bright) >
							::value_type::value_type;
						using cos_t = typename
							std::remove_reference_t< decltype(cos) >
							::value_type::value_type::value_type;

						if constexpr(std::is_same_v< bright_t, cos_t >){
							return data_set_without_dark< bright_t >{
								bright.data(),
								cos.data()
							};
						}else{
							throw std::logic_error("input type mismatch");
						}
					},
					std::move(std::get< 0 >(data)),
					std::move(std::get< 1 >(data))
				));
			}

			// exec calculation
			for(auto& [id, calc_set]: calc_sets){
				(void)id;
				switch(param.out_type){
					case output_t::float32:
						std::visit(decode32, calc_set);
					break;
					case output_t::float64:
						std::visit(decode64, calc_set);
					break;
				}
			}
		}
	}


	void init(disposer::module_declarant& add){
		add("raw_phase", &make_module);
	}

	BOOST_DLL_AUTO_ALIAS(init)


} }
