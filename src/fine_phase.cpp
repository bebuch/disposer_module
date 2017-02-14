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
		double,
		long double
	>;

	enum class input_t{
		uint8,
		uint16,
		uint32
	};

	enum class output_t{
		float32,
		float64,
		float128
	};

	struct parameter{
		output_t out_type;

// 		bool dark_image;
// 		bool bright_image;
// 		bool gray_code;
// 		bool second_direction;
//
// 		std::size_t gray_count;
// 		std::size_t cos_count;
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
			disposer::container_output< bitmap_vector, type_list >
				fine_phase_images{"fine_phases"};

			disposer::container_output< bitmap_vector, type_list >
				quality_images{"quality_images"};
		} signals;


		void exec()override;


		void input_ready()override{
			signals.vector.activate_types(slots.vector.active_types());
		}


		parameter const param;
	};


	disposer::module_ptr make_module(disposer::make_data& data){
		if(data.is_first()) throw disposer::module_not_as_start(data);

		parameter param;

		data.params.set(param.x, "x");
		data.params.set(param.y, "y");

		data.params.set(param.width, "width");
		data.params.set(param.height, "height");

		// for integral is NaN defined as 0
		hana::for_each(
			disposer::hana_type_list< type_list >,
			[&data, &param](auto type_t){
				using type = typename decltype(type_t)::type;
				data.params.set(
					std::get< type >(param.default_value),
					"default_value",
					std::numeric_limits< type >::quiet_NaN()
				);
			}
		);

		return std::make_unique< module >(data, std::move(param));
	}


	template < typename T >
	bitmap< T > module::fine_phase(bitmap< T > const& image)const{
		bitmap< T > result(
			param.width,
			param.height,
			std::get< T >(param.default_value)
		);

		std::size_t const bx =
			param.x < 0 ? static_cast< std::size_t >(-param.x) : 0;

		std::size_t const by =
			param.y < 0 ? static_cast< std::size_t >(-param.y) : 0;

		std::size_t const ex =
			static_cast< std::int64_t >(param.width) <
			static_cast< std::int64_t >(image.width()) - param.x ?
				param.width :
					static_cast< std::int32_t >(image.width()) >
					param.x ? image.width() - param.x : 0;

		std::size_t const ey =
			static_cast< std::int64_t >(param.height) <
			static_cast< std::int64_t >(image.height()) - param.y ?
				param.height :
					static_cast< std::int32_t >(image.height()) > param.y ?
					image.height() - param.y : 0;


		// line wise copy
		for(std::size_t y = by; y < ey; ++y){
			std::copy(
				&image(bx + param.x, y + param.y),
				&image(ex + param.x, y + param.y),
				&result(bx, y)
			);
		}

		return result;
	}


	struct visitor{
		visitor(fine_phase::module& module, std::size_t id):
			module(module), id(id) {}

		template < typename T >
		void operator()(
			disposer::input_data< bitmap_vector< T > > const& vector
		){
			auto& data = vector.data();

			bitmap_vector< T > result(data.size());
			for(std::size_t i = 0; i < data.size(); ++i){
				result[i] = module.fine_phase(data[i]);
			}

			module.signals.vector.put< T >(std::move(result));
		}

		fine_phase::module& module;
		std::size_t const id;
	};



	void module::exec(){
		for(auto const& pair: slots.vector.get()){
			fine_phase::visitor visitor(*this, pair.first);
			std::visit(visitor, pair.second);
		}
	}


	void init(disposer::module_adder& add){
		add("fine_phase", &make_module);
	}

	BOOST_DLL_AUTO_ALIAS(init)


} }
