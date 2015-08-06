//-----------------------------------------------------------------------------
// Copyright (c) 2015 Benjamin Buch
//
// https://github.com/bebuch/disposer
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "big_saver.hpp"

#include "log.hpp"
#include "bitmap_sequence.hpp"
#include "name_generator.hpp"
#include "big_write.hpp"
#include "tar.hpp"

#include <disposer/module_base.hpp>


namespace disposer_module{ namespace big_saver{


	enum class input_t{
		sequence,
		vector,
		image
	};

	struct parameter{
		bool tar;

		std::size_t sequence_start;
		std::size_t camera_start;

		std::string dir;

		boost::optional< std::size_t > fixed_id;

		std::size_t sequence_count;

		input_t input;

		boost::optional< name_generator< std::size_t > > tar_pattern;
		boost::optional< name_generator< std::size_t, std::size_t, std::size_t > > big_pattern;
	};

	template < typename T >
	struct module: disposer::module_base{
		using save_type = std::vector< std::vector< std::reference_wrapper< bitmap< T > const > > >;

		module(std::string const& type, std::string const& chain, std::string const& name, parameter&& param):
			disposer::module_base(type, chain, name),
			param(std::move(param)){
				inputs = disposer::make_input_list(sequence, vector, image);
			}

		disposer::input< bitmap_sequence< T > > sequence{"sequence"};
		disposer::input< bitmap_vector< T > > vector{"vector"};
		disposer::input< bitmap< T > > image{"image"};

		void save(std::size_t id, save_type const& bitmap_sequence);

		std::string tar_name(std::size_t id);
		std::string big_name(std::size_t cam, std::size_t pos);
		std::string big_name(std::size_t id, std::size_t cam, std::size_t pos);

		void trigger_sequence(std::size_t id);
		void trigger_vector(std::size_t id);
		void trigger_image(std::size_t id);

		void trigger(std::size_t id)override;

		parameter const param;
	};

	template < typename T >
	disposer::module_ptr make_module(
		std::string const& type,
		std::string const& chain,
		std::string const& name,
		disposer::io_list const& inputs,
		disposer::io_list const&,
		disposer::parameter_processor& params,
		bool is_start
	){
		if(is_start) throw disposer::module_not_as_start(type, chain);

		std::array< bool, 3 > const use_input{{
			inputs.find("sequence") != inputs.end(),
			inputs.find("vector") != inputs.end(),
			inputs.find("image") != inputs.end()
		}};

		std::size_t input_count = std::count(use_input.begin(), use_input.end(), true);

		if(input_count > 1){
			throw std::logic_error(type + ": Can only use one input ('image', 'vector' or 'sequence')");
		}

		if(input_count == 0){
			throw std::logic_error(type + ": No input defined (use 'image', 'vector' or 'sequence')");
		}

		parameter param;

		params.set(param.tar, "tar", false);

		params.set(param.sequence_start, "sequence_start", 0);
		params.set(param.camera_start, "camera_start", 0);

		params.set(param.dir, "dir", ".");

		std::size_t id_digits;
		std::size_t camera_digits;
		std::size_t position_digits;
		params.set(id_digits, "id_digits", 3);
		params.set(camera_digits, "camera_digits", 1);
		params.set(position_digits, "position_digits", 3);

		params.set(param.fixed_id, "fixed_id");

		if(use_input[0]){
			param.input = input_t::sequence;
		}else if(use_input[1]){
			param.input = input_t::vector;
		}else{
			param.input = input_t::image;
			params.set(param.sequence_count, "sequence_count");
			if(param.sequence_count == 0){
				throw std::logic_error(make_string(type + ": sequence_count (value: ", param.sequence_count, ") needs to be greater than 0"));
			}
		}

		struct format{
			std::size_t digits;

			std::string operator()(std::size_t value){
				std::ostringstream os;
				os << std::setw(digits) << std::setfill('0') << value;
				return os.str();
			}
		};

		if(param.tar){
			param.tar_pattern = make_name_generator(
				params.get< std::string >("tar_pattern", "${id}.tar"),
				{{true}},
				std::make_pair("id", format{id_digits})
			);
		}

		param.big_pattern = make_name_generator(
			params.get("big_pattern", param.tar ? std::string("${cam}_${pos}.big") : std::string("${id}_${cam}_${pos}.big")),
			{{!param.tar, true, true}},
			std::make_pair("id", format{id_digits}),
			std::make_pair("cam", format{camera_digits}),
			std::make_pair("pos", format{position_digits})
		);

		return std::make_unique< module< T > >(type, chain, name, std::move(param));
	}

	void init(){
		add_module_maker("big_saver_int8_t", &make_module< std::int8_t >);
		add_module_maker("big_saver_uint8_t", &make_module< std::uint8_t >);
		add_module_maker("big_saver_int16_t", &make_module< std::int16_t >);
		add_module_maker("big_saver_uint16_t", &make_module< std::uint16_t >);
		add_module_maker("big_saver_int32_t", &make_module< std::int32_t >);
		add_module_maker("big_saver_uint32_t", &make_module< std::uint32_t >);
		add_module_maker("big_saver_int64_t", &make_module< std::int64_t >);
		add_module_maker("big_saver_uint64_t", &make_module< std::uint64_t >);
		add_module_maker("big_saver_float", &make_module< float >);
		add_module_maker("big_saver_double", &make_module< double >);
		add_module_maker("big_saver_long_double", &make_module< long double >);
	}


	template < typename T >
	void module< T >::save(std::size_t id, save_type const& bitmap_sequence){
		auto used_id = param.fixed_id ? *param.fixed_id : id;

		if(param.tar){
			auto tarname = param.dir + "/" + (*param.tar_pattern)(used_id);
			disposer::log([this, &tarname, id](log::info& os){ os << type << " id " << id << ": write '" << tarname << "'"; }, [this, id, used_id, &bitmap_sequence, &tarname]{
				tar_writer tar(tarname);
				std::size_t cam = param.camera_start;
				for(auto& sequence: bitmap_sequence){
					std::size_t pos = param.sequence_start;
					for(auto& bitmap: sequence){
						auto filename = (*param.big_pattern)(used_id, cam, pos);
						disposer::log([this, &tarname, &filename, id](log::info& os){ os << type << " id " << id << ": write '" << tarname << "/" << filename << "'"; }, [&tar, &bitmap, &filename]{
							tar.write(filename, [&bitmap](std::ostream& os){
								big::write(bitmap.get(), os);
							});
						});
						++pos;
					}
					++cam;
				}
			});
		}else{
			std::size_t cam = param.camera_start;
			for(auto& sequence: bitmap_sequence){
				std::size_t pos = param.sequence_start;
				for(auto& bitmap: sequence){
					auto filename = param.dir + "/" + (*param.big_pattern)(used_id, cam, pos);
					disposer::log([this, &filename, id](log::info& os){ os << type << " id " << id << ": write '" << filename << "'"; }, [&bitmap, &filename]{
						big::write(bitmap.get(), filename);
					});
					++pos;
				}
				++cam;
			}
		}
	}


	template < typename T >
	void module< T >::trigger_sequence(std::size_t id){
		for(auto const& pair: sequence.get(id)){
			auto id = pair.first;
			auto& bitmap_sequence = pair.second.data();

			save_type data;
			for(auto& sequence: bitmap_sequence){
				data.emplace_back();
				for(auto& bitmap: sequence){
					data.back().emplace_back(bitmap);
				}
			}

			save(id, data);
		}
	}

	template < typename T >
	void module< T >::trigger_vector(std::size_t id){
		auto sequences = vector.get(id);
		auto from = sequences.begin();

		while(from != sequences.end()){
			auto id = from->first;
			auto to = sequences.upper_bound(id);

			save_type data;
			for(auto iter = from; iter != to; ++iter){
				data.emplace_back();
				for(auto& bitmap: iter->second.data()){
					data.back().emplace_back(bitmap);
				}
			}

			save(id, data);

			from = to;
		}
	}

	template < typename T >
	void module< T >::trigger_image(std::size_t id){
		auto images = image.get(id);
		auto from = images.begin();

		while(from != images.end()){
			auto id = from->first;
			auto to = images.upper_bound(id);

			std::size_t pos = 0;
			save_type data;
			for(auto iter = from; iter != to; ++iter){
				if(pos == 0) data.emplace_back();
				++pos;

				if(pos == param.sequence_count) pos = 0;

				data.back().emplace_back(iter->second.data());
			}

			if(pos != 0){
				throw std::runtime_error(type + ": single image count does not match parameter 'sequence_count'");
			}

			save(id, data);

			from = to;
		}
	}


	template < typename T >
	void module< T >::trigger(std::size_t id){
		switch(param.input){
			case input_t::sequence:
				trigger_sequence(id);
			break;
			case input_t::vector:
				trigger_vector(id);
			break;
			case input_t::image:
				trigger_image(id);
			break;
		}
	}


} }
