//-----------------------------------------------------------------------------
// Copyright (c) 2015-2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "get_bitmap_size.hpp"
#include "graycode.hpp"

#include <disposer/module.hpp>

#include <bitmap/size_io.hpp>

#include <boost/dll.hpp>
#include <boost/hana.hpp>

#include <cstdint>
#include <limits>


namespace disposer_module{ namespace gray_code_decoder{


	namespace hana = boost::hana;


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

	enum class output_t{
		uint8,
		uint16,
		uint32
	};

	struct parameter{
		output_t out_type;
	};


	struct module: disposer::module_base{
		module(disposer::make_data const& data, parameter&& param):
			disposer::module_base(
				data,
				{slots.threshold_image, slots.gray_code_images},
				{signals.index_image}
			),
			param(std::move(param))
			{}


		struct{
			disposer::container_input< bitmap, intensity_type_list >
				threshold_image{"threshold_image"};

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
		os << "different size in threshold image (" << size
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
	bitmap< IndexT > decode(
		bitmap< IntensityT > const& threshold_image,
		bitmap_vector< IntensityT > const& gray_images
	){
		auto size = threshold_image.size();
		check_gray_code_images< sizeof(IndexT) >(size, gray_images);

		bitmap< IndexT > result(size);

		for(std::size_t y = 0; y < size.height(); ++y){
			for(std::size_t x = 0; x < size.width(); ++x){
				IndexT code = 0;
				std::uint64_t const threshold = threshold_image(x, y);

				for(std::size_t i = 0; i < gray_images.size(); ++i){
					code <<= 1;
					code |= gray_images[i](x, y) < threshold ? 0 : 1;
				}

				result(x, y) = gray_to_binary_code(code);
			}
		}

		return result;
	}


	disposer::module_ptr make_module(disposer::make_data& data){
		if(data.is_first()) throw disposer::module_not_as_start(data);

		parameter param;

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

		// threshold_image must have enabled types
		auto threshold_types = slots.threshold_image.enabled_types_transformed(
			bitmap_value_type
		);
		if(threshold_types.size() == 0){
			throw std::logic_error(
				"input " + slots.threshold_image.name + " enabled types");
		}

		// others must have the same types enabled as threshold_image
		// or none
		auto throw_if_diff_types = [this, &threshold_types]
			(auto const& input, auto const& input_types){
				if(threshold_types == input_types) return;

				std::ostringstream os;
				os << "different input types in '"
					<< slots.threshold_image.name << "' (";
				for(auto& type: threshold_types){
					os << "'" << type.pretty_name() << "'";
				}

				os << ") and " << input.name << " (";
				for(auto& type: input_types){
					os << "'" << type.pretty_name() << "'";
				}

				os << ")";

				throw std::logic_error(os.str());
			};

		auto gray_types = slots.gray_code_images.enabled_types_transformed(
			bitmap_vector_value_type
		);

		throw_if_diff_types(slots.gray_code_images, gray_types);

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


	void module::exec(){
		// id verificator
		auto check_ids = [](std::size_t id1, std::size_t id2){
				if(id1 == id2) return;

				// a very excotic error that should only appeare
				// if a previous module did set output.put() id's wrong
				throw std::logic_error("id mismatch");
			};


		// get all the data
		auto threshold_data = slots.threshold_image.get();
		auto gray_data = slots.gray_code_images.get();

		// check count of puts in previous modules with respect to params
		if(threshold_data.size() != gray_data.size()){
			std::ostringstream os;
			os << "different count of input data ("
				<< slots.threshold_image.name << ": " << threshold_data.size()
				<< ", "
				<< slots.gray_code_images.name << ": " << gray_data.size()
				<< ")";

			throw std::logic_error(os.str());
		}

		// check id's and types and execute decoding
		auto gray_iter = gray_data.begin();
		for(
			auto threshold_iter = threshold_data.begin();
			threshold_iter == threshold_data.end();
			++threshold_iter, ++gray_iter
		){
			auto& [threshold_id, threshold_variant] = *threshold_iter;
			auto& [gray_id, gray_variant] = *gray_iter;

			check_ids(threshold_id, gray_id);

			std::visit([this](auto& threshold_input, auto& gray_input){
					using threshold_t = typename
						std::remove_reference_t< decltype(threshold_input) >
						::value_type::value_type;
					using gray_t = typename
						std::remove_reference_t< decltype(gray_input) >
						::value_type::value_type::value_type;

					if constexpr(std::is_same_v< threshold_t, gray_t >){
						switch(param.out_type){
							case output_t::uint8:
								signals.index_image.put< std::uint8_t >(
									decode< std::uint8_t >(
										threshold_input.data(),
										gray_input.data()));
							break;
							case output_t::uint16:
								signals.index_image.put< std::uint16_t >(
									decode< std::uint16_t >(
										threshold_input.data(),
										gray_input.data()));
							break;
							case output_t::uint32:
								signals.index_image.put< std::uint32_t >(
									decode< std::uint32_t >(
										threshold_input.data(),
										gray_input.data()));
							break;
						}
					}else{
						throw std::logic_error("input type mismatch");
					}
				},
				threshold_variant, gray_variant
			);
		}
	}


	void init(disposer::module_declarant& add){
		add("gray_code_decoder", &make_module);
	}

	BOOST_DLL_AUTO_ALIAS(init)


} }
