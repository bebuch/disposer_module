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
#include <numeric>
#include <limits>
#include <cmath>


namespace disposer_module{ namespace raw_phase{


	namespace hana = boost::hana;


	template < typename T >
	using reference_vector =
		std::vector< std::reference_wrapper< bitmap< T > const > >;


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
		std::size_t rotate;
		bool reverse_images;

		output_t out_type;
	};


	struct module: disposer::module_base{
		module(disposer::make_data const& data, parameter&& param):
			disposer::module_base(
				data,
				{slots.cos_images},
				{signals.phase_image, signals.modulation_image}
			),
			param(std::move(param))
			{}


		template < typename PhaseT, typename IntensityT >
		std::tuple< bitmap< PhaseT >, bitmap< IntensityT > >
		decode(reference_vector< IntensityT >&& cos)const;


		struct{
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



	template < typename T >
	inline T save_atan2(T n, T d){
		using std::atan2;
		if(n == 0 && d == 0){
			return T(M_PI) / 2;
		}else{
			return atan2(n, d);
		}
	}


	/// Calculate roh and modulation for equidistant phase shifts
	template < typename PhaseT, std::size_t CosCount >
	struct calc_roh_and_modulation;

	/// Calculate roh and modulation (90 degrees phase shift)
	template < typename PhaseT >
	struct calc_roh_and_modulation< PhaseT, 4 >{
		parameter const& param;

		template < typename IntensityT >
		std::tuple< PhaseT, IntensityT > operator()(
			std::array< IntensityT, 4 > const& v
		)const{
			using std::sqrt;

			auto n = static_cast< PhaseT >(v[1] - v[3]);
			auto d = static_cast< PhaseT >(v[2] - v[0]);

			return std::make_tuple(
				save_atan2(n, d),
				static_cast< IntensityT >(sqrt(n * n + d * d))
			);
		}

		template < typename IntensityT >
		std::tuple< PhaseT, IntensityT > operator()(
			reference_vector< IntensityT > const& cos,
			std::size_t x, std::size_t y
		)const{
			return (*this)(
				std::array< IntensityT, 4 >{{
					cos[ 0](x, y), cos[ 1](x, y),
					cos[ 2](x, y), cos[ 3](x, y)
				}});
		}
	};

	/// Calculate roh and modulation (60 degrees phase shift)
	template < typename PhaseT >
	struct calc_roh_and_modulation< PhaseT, 6 >{
		parameter const& param;

		template < typename IntensityT >
		std::tuple< PhaseT, IntensityT > operator()(
			std::array< IntensityT, 6 > const& v
		)const{
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

		template < typename IntensityT >
		std::tuple< PhaseT, IntensityT > operator()(
			reference_vector< IntensityT > const& cos,
			std::size_t x, std::size_t y
		)const{
			return (*this)(
				std::array< IntensityT, 6 >{{
					cos[ 0](x, y), cos[ 1](x, y),
					cos[ 2](x, y), cos[ 3](x, y),
					cos[ 4](x, y), cos[ 5](x, y)
				}});

		}
	};

	/// Calculate roh and modulation (45 degrees phase shift)
	template < typename PhaseT >
	struct calc_roh_and_modulation< PhaseT, 8 >{
		parameter const& param;

		template < typename IntensityT >
		std::tuple< PhaseT, IntensityT > operator()(
			std::array< IntensityT, 8 > const& v
		)const{
			using std::sqrt;

			static PhaseT const c_sqrt_2_d_2 = sqrt(PhaseT(2)) / 2;

			PhaseT n = (v[5] + v[7] - v[3] - v[1]) * c_sqrt_2_d_2 + v[6] - v[2];
			PhaseT d = (v[1] + v[7] - v[3] - v[5]) * c_sqrt_2_d_2 + v[0] - v[4];

			return std::make_tuple(
				save_atan2(n, d),
				static_cast< IntensityT >(sqrt(n * n + d * d) / 2)
			);
		}

		template < typename IntensityT >
		std::tuple< PhaseT, IntensityT > operator()(
			reference_vector< IntensityT > const& cos,
			std::size_t x, std::size_t y
		)const{
			return (*this)(
				std::array< IntensityT, 8 >{{
					cos[ 0](x, y), cos[ 1](x, y),
					cos[ 2](x, y), cos[ 3](x, y),
					cos[ 4](x, y), cos[ 5](x, y),
					cos[ 6](x, y), cos[ 7](x, y)
				}});
		}
	};

	/// Calculate roh and modulation (22.5 degrees phase shift)
	template < typename PhaseT >
	struct calc_roh_and_modulation< PhaseT, 16 >{
		parameter const& param;

		template < typename IntensityT >
		std::tuple< PhaseT, IntensityT > operator()(
			std::array< IntensityT, 16 > const& v
		)const{
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

		template < typename IntensityT >
		std::tuple< PhaseT, IntensityT > operator()(
			reference_vector< IntensityT > const& cos,
			std::size_t x, std::size_t y
		)const{
			return (*this)(
				std::array< IntensityT, 16 >{{
					cos[ 0](x, y), cos[ 1](x, y),
					cos[ 2](x, y), cos[ 3](x, y),
					cos[ 4](x, y), cos[ 5](x, y),
					cos[ 6](x, y), cos[ 7](x, y),
					cos[ 8](x, y), cos[ 9](x, y),
					cos[10](x, y), cos[11](x, y),
					cos[12](x, y), cos[13](x, y),
					cos[14](x, y), cos[14](x, y)
				}});
		}
	};


	template < typename PhaseT, typename IntensityT, std::size_t CosCount >
	std::tuple< bitmap< PhaseT >, bitmap< IntensityT > >
	roh_and_modulation(
		calc_roh_and_modulation< PhaseT, CosCount > const& calc,
		::bitmap::size< std::size_t > const image_size,
		reference_vector< IntensityT > const& cos
	){
		static constexpr auto NaN = std::numeric_limits< PhaseT >::quiet_NaN();
		bitmap< PhaseT > phase(image_size, NaN);
		bitmap< IntensityT > modulation(image_size);

		for(std::size_t y = 0; y < image_size.height(); ++y){
			for(std::size_t x = 0; x < image_size.width(); ++x){
				std::tie(phase(x, y), modulation(x, y)) = calc(cos, x, y);
			}
		}

		return std::make_tuple(std::move(phase), std::move(modulation));
	}


	template < typename PhaseT, typename IntensityT >
	std::tuple< bitmap< PhaseT >, bitmap< IntensityT > >
	module::decode(reference_vector< IntensityT >&& cos)const{
		if(cos.empty()){
			throw std::logic_error(
				"input '" + slots.cos_images.name + "' has no images");
		}

		if(param.rotate > 0){
			if(param.rotate > cos.size()){
				throw std::logic_error("parameter rotate ("
					+ std::to_string(param.rotate) + ") is out of range");
			}
			std::rotate(cos.begin(), cos.begin() + param.rotate, cos.end());
		}

		if(param.reverse_images){
			std::reverse(cos.begin(), cos.end());
		}

		auto const image_size = std::accumulate(
			cos.cbegin() + 1, cos.cend(), (*cos.cbegin()).get().size(),
			[&cos](auto& ref, auto& test){
				if(ref == test.get().size()) return ref;

				std::ostringstream os;
				os << "different cos image sizes (";
				bool first = true;
				for(auto& img: cos){
					if(first){ first = false; }else{ os << ", "; }
					os << img.get().size();
				}
				os << ") image";
				throw std::logic_error(os.str());
			});

		switch(cos.size()){
		case 4:
			return roh_and_modulation(
				calc_roh_and_modulation< PhaseT, 4 >{param}, image_size, cos);
		break;
		case 6:
			return roh_and_modulation(
				calc_roh_and_modulation< PhaseT, 6 >{param}, image_size, cos);
		break;
		case 8:
			return roh_and_modulation(
				calc_roh_and_modulation< PhaseT, 8 >{param}, image_size, cos);
		break;
		case 16:
			return roh_and_modulation(
				calc_roh_and_modulation< PhaseT, 16 >{param}, image_size, cos);
		break;
		default:
			throw std::logic_error(
				"raw_phase calculation is only implemented for 4, 6, "
				"8 and 16 cos images, you have "
				+ std::to_string(cos.size()));
		}
	}


	disposer::module_ptr make_module(disposer::make_data& data){
		if(data.is_first()) throw disposer::module_not_as_start(data);

		parameter param;

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


		data.params.set(param.reverse_images, "reverse_images", false);
		data.params.set(param.rotate, "rotate", 0);


		return std::make_unique< module >(data, std::move(param));
	}


	void module::input_ready(){
		auto bitmap_value_type = [](auto type){
			return hana::type_c< typename decltype(type)::type::value_type >;
		};

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
			slots.cos_images.enabled_types_transformed(bitmap_value_type));
	}


	template < typename T >
	reference_vector< T > as_reference_vector(bitmap_vector< T > const& bitmaps){
		reference_vector< T > result;
		result.reserve(bitmaps.size());
		for(auto& bitmap: bitmaps) result.emplace_back(std::cref(bitmap));
		return result;
	}


	void module::exec(){
		auto decode32 = [this](auto& cos){
			using value_type =
				typename std::remove_reference_t< decltype(cos.data()) >
				::value_type::value_type;
			auto [phase, modulation] =
				decode< float >(as_reference_vector(cos.data()));
			signals.phase_image.put< float >(phase);
			signals.modulation_image.put< value_type >(modulation);
		};
		auto decode64 = [this](auto& cos){
			using value_type =
				typename std::remove_reference_t< decltype(cos.data()) >
				::value_type::value_type;
			auto [phase, modulation] =
				decode< double >(as_reference_vector(cos.data()));
			signals.phase_image.put< double >(phase);
			signals.modulation_image.put< value_type >(modulation);
		};

		// exec calculation
		for(auto& [id, img]: slots.cos_images.get()){
			(void)id;
			switch(param.out_type){
				case output_t::float32:
					std::visit(decode32, img);
				break;
				case output_t::float64:
					std::visit(decode64, img);
				break;
			}
		}
	}


	void init(disposer::module_declarant& add){
		add("raw_phase", &make_module);
	}

	BOOST_DLL_AUTO_ALIAS(init)


} }
