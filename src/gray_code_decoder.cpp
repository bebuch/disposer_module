//-----------------------------------------------------------------------------
// Copyright (c) 2015-2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "bitmap_sequence.hpp"
#include "graycode.hpp"

#include <disposer/module.hpp>

#include <bitmap/size_io.hpp>

#include <boost/dll.hpp>
#include <boost/hana.hpp>

#include <cstdint>
#include <limits>


namespace disposer_module{ namespace gray_code_decoder{


	namespace hana = boost::hana;


	using disposer::type_position_v;

	using intensity_type_list = disposer::type_list<
		std::uint8_t,
		std::uint16_t,
		std::uint32_t,
		float,
		double
	>;

	using index_type_list = disposer::type_list<
		std::uint8_t,
		std::uint16_t,
		std::uint32_t
	>;

	enum class input_t{
		uint8,
		uint16,
		uint32,
		float32,
		float64
	};

	enum class output_t{
		uint8,
		uint16,
		uint32
	};

	struct parameter{
		output_t out_type;

		bool dark_environement;
		bool have_dark; // needed if parameter dark_environement != true
	};


	template < typename IntensityT >
	struct data_set_with_dark{
		bitmap< IntensityT > bright;
		bitmap< IntensityT > dark;
		bitmap_vector< IntensityT > gray;
	};

	template < typename IntensityT >
	struct data_set_without_dark{
		bitmap< IntensityT > bright;
		bitmap_vector< IntensityT > gray;
	};


	struct module: disposer::module_base{
		module(disposer::make_data const& data, parameter&& param):
			disposer::module_base(
				data,
				{slots.bright_image, slots.dark_image, slots.gray_code_images},
				{signals.index_image}
			),
			param(std::move(param))
			{}


		template < typename IndexT, typename IntensityT >
		bitmap< IndexT > decode(
			data_set_with_dark< IntensityT > const& data
		)const;

		template < typename IndexT, typename IntensityT >
		bitmap< IndexT > decode(
			data_set_without_dark< IntensityT > const& data
		)const;


		struct{
			disposer::container_input< bitmap, intensity_type_list >
				bright_image{"bright_image"};

			disposer::container_input< bitmap, intensity_type_list >
				dark_image{"dark_image"};

			disposer::container_input< bitmap_vector, intensity_type_list >
				gray_code_images{"gray_code_images"};
		} slots;

		struct{
			disposer::container_output< bitmap, index_type_list >
				index_image{"index_image"};
		} signals;


		void input_ready()override;


		void exec()override;


		parameter param;
	};


	template < typename SizeT, typename T >
	void check_size(
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

	template < std::size_t IndexBytes, typename SizeT, typename T >
	void check_gray_code_images(
		SizeT const& size,
		bitmap_vector< T > const& images
	){
		for(auto& img: images){
			check_size(size, img, "gray code");
		}

		if(images.size() > IndexBytes * 8){
			std::ostringstream os;
			os << "too much gray code images ("
				<< images.size() << ") for output_type";
			throw std::logic_error(os.str());
		}

		if(images.size() == 0){
			throw std::logic_error("need at least one gray code image");
		}
	}


	template < typename IndexT, typename IntensityT >
	bitmap< IndexT > module::decode(
		data_set_with_dark< IntensityT > const& data
	)const{
		auto size = data.bright.size();
		check_size(size, data.dark, "dark");
		check_gray_code_images< sizeof(IndexT) >(size, data.gray);

		bitmap< IndexT > result(size);

		for(std::size_t y = 0; y < size.height(); ++y){
			for(std::size_t x = 0; x < size.width(); ++x){
				IndexT code = 0;
				std::uint64_t const min = data.dark(x, y);
				std::uint64_t const max = data.bright(x, y);
				if(min >= max) continue;

				auto const mid = static_cast< IndexT >((max + min) / 2);
				for(std::size_t i = 0; i < data.gray.size(); ++i){
					code <<= 1;
					code |= data.gray[i](x, y) < mid ? 0 : 1;
				}

				result(x, y) = gray_to_binary_code(code);
			}
		}

		return result;
	}

	template < typename IndexT, typename IntensityT >
	bitmap< IndexT > module::decode(
		data_set_without_dark< IntensityT > const& data
	)const{
		auto size = data.bright.size();
		check_gray_code_images< sizeof(IndexT) >(size, data.gray);

		bitmap< IndexT > result(size);

		for(std::size_t y = 0; y < size.height(); ++y){
			for(std::size_t x = 0; x < size.width(); ++x){
				IndexT code = 0;
				auto const max = data.bright(x, y) / 2;
				if(max <= 0) continue;

				for(std::size_t i = 0; i < data.gray.size(); ++i){
					code <<= 1;
					code |= data.gray[i](x, y) < max ? 0 : 1;
				}

				result(x, y) = gray_to_binary_code(code);
			}
		}


		return result;
	}


	disposer::module_ptr make_module(disposer::make_data& data){
		if(data.is_first()) throw disposer::module_not_as_start(data);

		parameter param;

		data.params.set(param.dark_environement, "dark_environement", false);

		static std::unordered_map< std::string_view, output_t > const
			output_types{
				{"uint8", output_t::uint8},
				{"uint16", output_t::uint16},
				{"uint32", output_t::uint32}
			};

		std::string output_type;
		data.params.set(output_type, "output_type", "uint8");

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
			auto gray_types = slots.gray_code_images.enabled_types_transformed(
				bitmap_vector_value_type
			);

			throw_if_diff_types(slots.gray_code_images, gray_types);

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
				case output_t::uint8:
					signals.index_image.enable< std::uint8_t >();
				break;
				case output_t::uint16:
					signals.index_image.enable< std::uint16_t >();
				break;
				case output_t::uint32:
					signals.index_image.enable< std::uint32_t >();
				break;
			}
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

		auto decode8 = [this](auto const& data){
			return decode< std::uint8_t >(data);
		};
		auto decode16 = [this](auto const& data){
			return decode< std::uint16_t >(data);
		};
		auto decode32 = [this](auto const& data){
			return decode< std::uint32_t >(data);
		};


		if(param.have_dark){
			// get all the data
			auto bright_data = slots.bright_image.get();
			auto dark_data = slots.dark_image.get();
			auto gray_data = slots.gray_code_images.get();

			// check count of puts in previous modules with respect to params
			if(
				(bright_data.size() != dark_data.size()) ||
				(bright_data.size() != gray_data.size())
			){
				std::ostringstream os;
				os << "different count of input data ("
					<< slots.bright_image.name << ": " << bright_data.size()
					<< ", "
					<< slots.dark_image.name << ": " << dark_data.size()
					<< ", "
					<< slots.gray_code_images.name << ": " << gray_data.size()
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
			auto gray_iter = gray_data.begin();
			for(
				auto bright_iter = bright_data.begin();
				bright_iter != bright_data.end();
				++bright_iter, ++dark_iter, ++gray_iter
			){
				auto&& [bright_id, bright_img] = *bright_iter;
				auto&& [dark_id, dark_img] = *dark_iter;
				auto&& [gray_id, gray_img] = *gray_iter;

				check_ids(bright_id, dark_id);
				check_ids(bright_id, gray_id);

				raw_data_sets.emplace(
					std::piecewise_construct,
					std::forward_as_tuple(id),
					std::forward_as_tuple(bright_img, dark_img, gray_img));
			}

			// check corresponding inputs for same data type and move them
			// into a single variant data set
			std::multimap< std::size_t, data_set_variant_with_dark >
				calc_sets;
			for(auto&& [id, data]: raw_data_sets){
				calc_sets.emplace(id, std::visit(
					[this](auto&& bright, auto&& dark, auto&& gray)
					->data_set_variant_with_dark{
						using bright_t = typename
							std::remove_reference_t< decltype(bright) >
							::value_type::value_type;
						using dark_t = typename
							std::remove_reference_t< decltype(dark) >
							::value_type::value_type;
						using gray_t = typename
							std::remove_reference_t< decltype(gray) >
							::value_type::value_type::value_type;

						if constexpr(
							std::is_same_v< bright_t, dark_t > &&
							std::is_same_v< bright_t, gray_t >
						){
							return data_set_with_dark< bright_t >{
								bright.data(),
								dark.data(),
								gray.data()
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
					case output_t::uint8:
						signals.index_image.put< std::uint8_t >
							(std::visit(decode8, calc_set));
					break;
					case output_t::uint16:
						signals.index_image.put< std::uint16_t >
							(std::visit(decode16, calc_set));
					break;
					case output_t::uint32:
						signals.index_image.put< std::uint32_t >
							(std::visit(decode32, calc_set));
					break;
				}
			}
		}else{
			// get all the data
			auto bright_data = slots.bright_image.get();
			auto gray_data = slots.gray_code_images.get();

			// check count of puts in previous modules with respect to params
			if(bright_data.size() != gray_data.size()){
				std::ostringstream os;
				os << "different count of input data ("
					<< slots.bright_image.name << ": " << bright_data.size()
					<< ", "
					<< slots.gray_code_images.name << ": " << gray_data.size()
					<< ")";

				throw std::logic_error(os.str());
			}

			// combine data in a single variant type
			using raw_data_set = std::tuple<
					bitmap_variant,
					bitmap_vector_variant
				>;
			std::multimap< std::size_t, raw_data_set > raw_data_sets;
			auto gray_iter = gray_data.begin();
			for(
				auto bright_iter = bright_data.begin();
				bright_iter == bright_data.end();
				++bright_iter, ++gray_iter
			){
				auto&& [bright_id, bright_img] = *bright_iter;
				auto&& [gray_id, gray_img] = *gray_iter;

				check_ids(bright_id, gray_id);

				raw_data_sets.emplace(
					std::piecewise_construct,
					std::forward_as_tuple(id),
					std::forward_as_tuple(bright_img, gray_img));
			}

			// check corresponding inputs for same data type and move them
			// into a single variant data set
			std::multimap< std::size_t, data_set_variant_without_dark >
				calc_sets;
			for(auto&& [id, data]: raw_data_sets){
				calc_sets.emplace(id, std::visit(
					[this](auto&& bright, auto&& gray)
					->data_set_variant_without_dark{
						using bright_t = typename
							std::remove_reference_t< decltype(bright) >
							::value_type::value_type;
						using gray_t = typename
							std::remove_reference_t< decltype(gray) >
							::value_type::value_type::value_type;

						if constexpr(std::is_same_v< bright_t, gray_t >){
							return data_set_without_dark< bright_t >{
								bright.data(),
								gray.data()
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
					case output_t::uint8:
						signals.index_image.put< std::uint8_t >
							(std::visit(decode8, calc_set));
					break;
					case output_t::uint16:
						signals.index_image.put< std::uint16_t >
							(std::visit(decode16, calc_set));
					break;
					case output_t::uint32:
						signals.index_image.put< std::uint32_t >
							(std::visit(decode32, calc_set));
					break;
				}
			}
		}
	}


	void init(disposer::module_declarant& add){
		add("gray_code_decoder", &make_module);
	}

	BOOST_DLL_AUTO_ALIAS(init)


} }
