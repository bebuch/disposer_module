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

	using input_type_list = disposer::type_list<
		std::uint8_t,
		std::uint16_t,
		std::uint32_t
	>;

	using output_type_list = disposer::type_list<
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

		bool second_dir;
		bool have_bright;
		bool have_dark;
	};


	struct module: disposer::module_base{
		module(disposer::make_data const& data, parameter&& param):
			disposer::module_base(
				data,
				{slots.vector},
				{signals.vector}
			),
			param(std::move(param))
			{}


		template < typename T >
		bitmap< T > fine_phase(bitmap< T > const& image)const;


		struct{
			disposer::container_input< bitmap_vector, type_list >
				bright_image{"bright_image"};

			disposer::container_input< bitmap_vector, type_list >
				dark_image{"dark_image"};

			disposer::container_input< bitmap_vector, type_list >
				gray_code_images{"gray_code_dir1_images"};

			disposer::container_input< bitmap_vector, type_list >
				cos_images{"cos_dir1_images"};

			disposer::container_input< bitmap_vector, type_list >
				gray_code_images{"gray_code_dir2_images"};

			disposer::container_input< bitmap_vector, type_list >
				cos_images{"cos_dir2_images"};
		} slots;

		struct{
			disposer::container_output< bitmap, type_list >
				fine_phase_dir1{"fine_phase_dir1"};

			disposer::container_output< bitmap, type_list >
				fine_phase_dir2{"fine_phase_dir2"};

			disposer::container_output< bitmap, type_list >
				quality_image_dir1{"quality_image_dir2"};

			disposer::container_output< bitmap, type_list >
				quality_image_dir2{"quality_image_dir2"};
		} signals;


		void exec()override;


		void input_ready()override{
			// check if all enabled inputs have the same type
			slots.cos_dir1_images.enabled_types();

			//
			switch(param.out_type){
				case output_t::float32:
					signals.fine_phase_dir1.activate< float >();
					if(param.second_dir){
						signals.fine_phase_dir2.activate< float >();
					}
				break;
				case output_t::float64:
					signals.fine_phase_dir1.activate< double >();
					if(param.second_dir){
						signals.fine_phase_dir2.activate< double >();
					}
				break;
			}

			signals.quality_image_dir1.activate< float >();

			if(param.second_dir){
				signals.quality_image_dir2.activate< float >();
			}
		}


		parameter param;
	};


	template < typename T >
	bitmap< T > difference(bitmap< T > l, bitmap< T > const& r){
		std::transform(l.begin(), l.end(), r.begin(), [](auto l, auto r){
			return l > r : l - r : T(0);
		});

		return l;
	}


	template < typename T >
	bitmap< T > calc_bright(bitmap_vector< T > const& cos_images){
		bitmap< T > result(cos_images[].size());

		for(std::size_t i = 0; i < result.point_count(); ++i){
			double sum = 0;

			for(std::size_t j = 0; j < cos_images.size(); ++j){
				sum += cos_images[j].data()[i];
			}

			result.data()[i] = static_cast< T >(sum / cos_images.size());
		}

		return l;
	}


	template < typename IntensityT, typename PhaseT >
	class calculator{
	public:
		calculator(
			bitmap< IntensityT >&& dark,
			bitmap< IntensityT >&& bright,
			bitmap_vector< IntensityT >&& gray_dir_1,
			bitmap_vector< IntensityT >&& cos_dir_1,
			bitmap_vector< IntensityT >&& gray_dir_2,
			bitmap_vector< IntensityT >&& cos_dir_2,
		):
			diff_(difference(bright, dark)),
			gray_dir_1_(std::move(gray_dir_1)),
			cos_dir_1_(std::move(cos_dir_1)),
			gray_dir_2_(std::move(gray_dir_2)),
			cos_dir_2_(std::move(cos_dir_2)){}

		calculator(
			bitmap< IntensityT >&& bright,
			bitmap_vector< IntensityT >&& gray_dir_1,
			bitmap_vector< IntensityT >&& cos_dir_1,
			bitmap_vector< IntensityT >&& gray_dir_2,
			bitmap_vector< IntensityT >&& cos_dir_2,
		):
			diff_(std::move(bright)),
			gray_dir_1_(std::move(gray_dir_1)),
			cos_dir_1_(std::move(cos_dir_1)),
			gray_dir_2_(std::move(gray_dir_2)),
			cos_dir_2_(std::move(cos_dir_2)){}

		calculator(
			bitmap_vector< IntensityT >&& gray_dir_1,
			bitmap_vector< IntensityT >&& cos_dir_1,
			bitmap_vector< IntensityT >&& gray_dir_2,
			bitmap_vector< IntensityT >&& cos_dir_2,
		):
			diff_(calc_bright(cos_dir_1)),
			gray_dir_1_(std::move(gray_dir_1)),
			cos_dir_1_(std::move(cos_dir_1)),
			gray_dir_2_(std::move(gray_dir_2)),
			cos_dir_2_(std::move(cos_dir_2)){}

		calculator(
			bitmap< IntensityT >&& dark,
			bitmap< IntensityT >&& bright,
			bitmap_vector< IntensityT >&& gray_dir_1,
			bitmap_vector< IntensityT >&& cos_dir_1
		):
			diff_(difference(bright, dark)),
			gray_dir_1_(std::move(gray_dir_1)),
			cos_dir_1_(std::move(cos_dir_1)){}

		calculator(
			bitmap< IntensityT >&& bright,
			bitmap_vector< IntensityT >&& gray_dir_1,
			bitmap_vector< IntensityT >&& cos_dir_1
		):
			diff_(std::move(bright)),
			gray_dir_1_(std::move(gray_dir_1)),
			cos_dir_1_(std::move(cos_dir_1)){}

		calculator(
			bitmap_vector< IntensityT >&& gray_dir_1,
			bitmap_vector< IntensityT >&& cos_dir_1
		):
			diff_(calc_bright(cos_dir_1)),
			gray_dir_1_(std::move(gray_dir_1)),
			cos_dir_1_(std::move(cos_dir_1)){}

	private:
		bitmap< IntensityT > const diff_;
		bitmap_vector< IntensityT > const gray_dir_1_;
		bitmap_vector< IntensityT > const cos_dir_1_;
		bitmap_vector< IntensityT > const gray_dir_2_;
		bitmap_vector< IntensityT > const cos_dir_2_;
	};


	disposer::module_ptr make_module(disposer::make_data& data){
		if(data.is_first()) throw disposer::module_not_as_start(data);

		parameter param;

		data.params.set(param.dark_environement, "dark_environement", false);

		return std::make_unique< module >(data, std::move(param));
	}


	void module::exec(){
		for(auto const& [id, img]: slots.vector.get()){
		}
	}


	void init(disposer::module_declarant& add){
		add("fine_phase", &make_module);
	}

	BOOST_DLL_AUTO_ALIAS(init)


} }
