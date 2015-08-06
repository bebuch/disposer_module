//-----------------------------------------------------------------------------
// Copyright (c) 2015 Benjamin Buch
//
// https://github.com/bebuch/disposer
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "big_loader.hpp"

#include "log.hpp"
#include "bitmap_sequence.hpp"
#include "name_generator.hpp"
#include "big_read.hpp"
#include "tar.hpp"

#include <disposer/module_base.hpp>


namespace disposer_module{ namespace big_loader{


	enum class output_t{
		sequence,
		vector,
		image
	};

	struct parameter{
		bool tar;

		std::size_t sequence_count;
		std::size_t camera_count;

		std::size_t sequence_start;
		std::size_t camera_start;

		std::string dir;

		bool type_int8;
		bool type_uint8;
		bool type_int16;
		bool type_uint16;
		bool type_int32;
		bool type_uint32;
		bool type_int64;
		bool type_uint64;
		bool type_float;
		bool type_double;
		bool type_long_double;

		boost::optional< std::size_t > fixed_id;

		output_t output;

		boost::optional< name_generator< std::size_t > > tar_pattern;
		boost::optional< name_generator< std::size_t, std::size_t, std::size_t > > big_pattern;
	};

	struct module: disposer::module_base{
		module(std::string const& type, std::string const& chain, std::string const& name, parameter&& param):
			disposer::module_base(type, chain, name),
			param(std::move(param)){
				outputs = disposer::make_output_list(sequence, vector, image);
			}

		disposer::container_output<
			bitmap_sequence,
				std::int8_t,
				std::uint8_t,
				std::int16_t,
				std::uint16_t,
				std::int32_t,
				std::uint32_t,
				std::int64_t,
				std::uint64_t,
				float,
				double,
				long double
			> sequence{"sequence"};

		disposer::container_output<
			bitmap_vector,
				std::int8_t,
				std::uint8_t,
				std::int16_t,
				std::uint16_t,
				std::int32_t,
				std::uint32_t,
				std::int64_t,
				std::uint64_t,
				float,
				double,
				long double
			> vector{"vector"};

		disposer::container_output<
			bitmap,
				std::int8_t,
				std::uint8_t,
				std::int16_t,
				std::uint16_t,
				std::int32_t,
				std::uint32_t,
				std::int64_t,
				std::uint64_t,
				float,
				double,
				long double
			> image{"image"};


		std::string tar_name(std::size_t id);
		std::string big_name(std::size_t cam, std::size_t pos);
		std::string big_name(std::size_t id, std::size_t cam, std::size_t pos);


		template < typename T >
		void trigger_sequence(std::size_t id);

		template < typename T >
		void trigger_vector(std::size_t id);

		template < typename T >
		void trigger_image(std::size_t id);


		void trigger(std::size_t id)override;


		parameter const param;


		struct tar_log{
			module const& this_;
			std::string const& tarname;
			std::size_t const id;

			void operator()(log::info& os)const{
				os << this_.type << " id " << id << ": read '" << tarname << "'";
			}
		};


		template < typename T >
		void load_bitmap(bitmap< T >& bitmap, std::size_t id, std::size_t used_id, std::size_t cam, std::size_t pos);

		template < typename T >
		void load_bitmap(bitmap< T >& bitmap, tar_reader& tar, std::string const& tarname, std::size_t id, std::size_t used_id, std::size_t cam, std::size_t pos);
	};


	disposer::module_ptr make_module(
		std::string const& type,
		std::string const& chain,
		std::string const& name,
		disposer::io_list const&,
		disposer::io_list const& outputs,
		disposer::parameter_processor& params,
		bool
	){
		std::array< bool, 3 > const use_output{{
			outputs.find("sequence") != outputs.end(),
			outputs.find("vector") != outputs.end(),
			outputs.find("image") != outputs.end()
		}};

		std::size_t output_count = std::count(use_output.begin(), use_output.end(), true);

		if(output_count > 1){
			throw std::logic_error(type + ": Can only use one output ('image', 'vector' or 'sequence')");
		}

		if(output_count == 0){
			throw std::logic_error(type + ": No output defined (use 'image', 'vector' or 'sequence')");
		}

		parameter param;

		params.set(param.tar, "tar", false);

		params.set(param.sequence_count, "sequence_count");
		params.set(param.camera_count, "camera_count");

		params.set(param.sequence_start, "sequence_start", 0);
		params.set(param.camera_start, "camera_start", 0);

		params.set(param.dir, "dir", ".");

		params.set(param.type_int8, "type_int8", false);
		params.set(param.type_uint8, "type_uint8", false);
		params.set(param.type_int16, "type_int16", false);
		params.set(param.type_uint16, "type_uint16", false);
		params.set(param.type_int32, "type_int32", false);
		params.set(param.type_uint32, "type_uint32", false);
		params.set(param.type_int64, "type_int64", false);
		params.set(param.type_uint64, "type_uint64", false);
		params.set(param.type_float, "type_float", false);
		params.set(param.type_double, "type_double", false);
		params.set(param.type_long_double, "type_long_double", false);

		if(
			!param.type_int8 &&
			!param.type_uint8 &&
			!param.type_int16 &&
			!param.type_uint16 &&
			!param.type_int32 &&
			!param.type_uint32 &&
			!param.type_int64 &&
			!param.type_uint64 &&
			!param.type_float &&
			!param.type_double &&
			!param.type_long_double
		){
			throw std::logic_error(
				type + 
				": No type active (set at least one of 'type_int8', 'type_uint8', 'type_int16', 'type_uint16', 'type_int32', 'type_uint32', 'type_int64', 'type_uint64', 'type_float', 'type_double', 'type_long_double' to true)"
			);
		}

		std::size_t id_digits;
		std::size_t camera_digits;
		std::size_t position_digits;
		params.set(id_digits, "id_digits", 3);
		params.set(camera_digits, "camera_digits", 1);
		params.set(position_digits, "position_digits", 3);

		params.set(param.fixed_id, "fixed_id");

		if(use_output[0]){
			param.output = output_t::sequence;
		}else if(use_output[1]){
			param.output = output_t::vector;
		}else{
			param.output = output_t::image;
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

		auto result = std::make_unique< module >(type, chain, name, std::move(param));

		switch(param.output){
			case output_t::sequence:
				if(param.type_int8) result->sequence.activate< std::int8_t >();
				if(param.type_uint8) result->sequence.activate< std::uint8_t >();
				if(param.type_int16) result->sequence.activate< std::int16_t >();
				if(param.type_uint16) result->sequence.activate< std::uint16_t >();
				if(param.type_int32) result->sequence.activate< std::int32_t >();
				if(param.type_uint32) result->sequence.activate< std::uint32_t >();
				if(param.type_int64) result->sequence.activate< std::int64_t >();
				if(param.type_uint64) result->sequence.activate< std::uint64_t >();
				if(param.type_float) result->sequence.activate< float >();
				if(param.type_double) result->sequence.activate< double >();
				if(param.type_long_double) result->sequence.activate< long double >();
			break;
			case output_t::vector:
				if(param.type_int8) result->vector.activate< std::int8_t >();
				if(param.type_uint8) result->vector.activate< std::uint8_t >();
				if(param.type_int16) result->vector.activate< std::int16_t >();
				if(param.type_uint16) result->vector.activate< std::uint16_t >();
				if(param.type_int32) result->vector.activate< std::int32_t >();
				if(param.type_uint32) result->vector.activate< std::uint32_t >();
				if(param.type_int64) result->vector.activate< std::int64_t >();
				if(param.type_uint64) result->vector.activate< std::uint64_t >();
				if(param.type_float) result->vector.activate< float >();
				if(param.type_double) result->vector.activate< double >();
				if(param.type_long_double) result->vector.activate< long double >();
			break;
			case output_t::image:
				if(param.type_int8) result->image.activate< std::int8_t >();
				if(param.type_uint8) result->image.activate< std::uint8_t >();
				if(param.type_int16) result->image.activate< std::int16_t >();
				if(param.type_uint16) result->image.activate< std::uint16_t >();
				if(param.type_int32) result->image.activate< std::int32_t >();
				if(param.type_uint32) result->image.activate< std::uint32_t >();
				if(param.type_int64) result->image.activate< std::int64_t >();
				if(param.type_uint64) result->image.activate< std::uint64_t >();
				if(param.type_float) result->image.activate< float >();
				if(param.type_double) result->image.activate< double >();
				if(param.type_long_double) result->image.activate< long double >();
			break;
		}

		return std::move(result);
	}

	void init(){
		add_module_maker("big_loader", &make_module);
	}


	template < typename T >
	void module::trigger_sequence(std::size_t id){
		auto used_id = param.fixed_id ? *param.fixed_id : id;

		bitmap_sequence< T > result;

		if(param.tar){
			auto tarname = param.dir + "/" + (*param.tar_pattern)(used_id);
			disposer::log(tar_log{*this, tarname, id}, [this, id, used_id, &tarname, &result]{
				tar_reader tar(tarname);

				for(std::size_t cam = param.camera_start; cam < param.camera_count + param.camera_start; ++cam){
					result.emplace_back();
					for(std::size_t pos = param.sequence_start; pos < param.sequence_count + param.sequence_start; ++pos){
						result.back().emplace_back();

						load_bitmap(result.back().back(), tar, tarname, id, used_id, cam, pos);
					}
				}
			});
		}else{
			for(std::size_t cam = param.camera_start; cam < param.camera_count + param.camera_start; ++cam){
				result.emplace_back();
				for(std::size_t pos = param.sequence_start; pos < param.sequence_count + param.sequence_start; ++pos){
					result.back().emplace_back();

					load_bitmap(result.back().back(), id, used_id, cam, pos);
				}
			}
		}

		sequence.put< bitmap_sequence< T > >(id, std::move(result));
	}

	template < typename T >
	void module::trigger_vector(std::size_t id){
		auto used_id = param.fixed_id ? *param.fixed_id : id;

		if(param.tar){
			auto tarname = param.dir + "/" + (*param.tar_pattern)(used_id);
			disposer::log(tar_log{*this, tarname, id}, [this, id, used_id, &tarname]{
				tar_reader tar(tarname);

				for(std::size_t cam = param.camera_start; cam < param.camera_count + param.camera_start; ++cam){
					bitmap_vector< T > result;

					for(std::size_t pos = param.sequence_start; pos < param.sequence_count + param.sequence_start; ++pos){
						result.emplace_back();

						load_bitmap(result.back(), tar, tarname, id, used_id, cam, pos);
					}

					vector.put< bitmap_vector< T > >(id, std::move(result));
				}
			});
		}else{
			for(std::size_t cam = param.camera_start; cam < param.camera_count + param.camera_start; ++cam){
				bitmap_vector< T > result;

				for(std::size_t pos = param.sequence_start; pos < param.sequence_count + param.sequence_start; ++pos){
					result.emplace_back();

					load_bitmap(result.back(), id, used_id, cam, pos);
				}

				vector.put< bitmap_vector< T > >(id, std::move(result));
			}
		}
	}

	template < typename T >
	void module::trigger_image(std::size_t id){
		auto used_id = param.fixed_id ? *param.fixed_id : id;

		if(param.tar){
			auto tarname = param.dir + "/" + (*param.tar_pattern)(used_id);
			disposer::log(tar_log{*this, tarname, id}, [this, id, used_id, &tarname]{
				tar_reader tar(tarname);

				for(std::size_t cam = param.camera_start; cam < param.camera_count + param.camera_start; ++cam){
					for(std::size_t pos = param.sequence_start; pos < param.sequence_count + param.sequence_start; ++pos){
						bitmap< T > result;

						load_bitmap(result, tar, tarname, id, used_id, cam, pos);

						image.put< bitmap< T > >(id, std::move(result));
					}
				}
			});
		}else{
			for(std::size_t cam = param.camera_start; cam < param.camera_count + param.camera_start; ++cam){
				for(std::size_t pos = param.sequence_start; pos < param.sequence_count + param.sequence_start; ++pos){
					bitmap< T > result;

					load_bitmap(result, id, used_id, cam, pos);

					image.put< bitmap< T > >(id, std::move(result));
				}
			}
		}
	}


	void module::trigger(std::size_t id){
		// TODO: First load and analyse header, then call the version with corresponding (and active) type
		switch(param.output){
			case output_t::sequence:
				if(param.type_int8) trigger_sequence< std::int8_t >(id);
				if(param.type_uint8) trigger_sequence< std::uint8_t >(id);
				if(param.type_int16) trigger_sequence< std::int16_t >(id);
				if(param.type_uint16) trigger_sequence< std::uint16_t >(id);
				if(param.type_int32) trigger_sequence< std::int32_t >(id);
				if(param.type_uint32) trigger_sequence< std::uint32_t >(id);
				if(param.type_int64) trigger_sequence< std::int64_t >(id);
				if(param.type_uint64) trigger_sequence< std::uint64_t >(id);
				if(param.type_float) trigger_sequence< float >(id);
				if(param.type_double) trigger_sequence< double >(id);
				if(param.type_long_double) trigger_sequence< long double >(id);
			break;
			case output_t::vector:
				if(param.type_int8) trigger_vector< std::int8_t >(id);
				if(param.type_uint8) trigger_vector< std::uint8_t >(id);
				if(param.type_int16) trigger_vector< std::int16_t >(id);
				if(param.type_uint16) trigger_vector< std::uint16_t >(id);
				if(param.type_int32) trigger_vector< std::int32_t >(id);
				if(param.type_uint32) trigger_vector< std::uint32_t >(id);
				if(param.type_int64) trigger_vector< std::int64_t >(id);
				if(param.type_uint64) trigger_vector< std::uint64_t >(id);
				if(param.type_float) trigger_vector< float >(id);
				if(param.type_double) trigger_vector< double >(id);
				if(param.type_long_double) trigger_vector< long double >(id);
			break;
			case output_t::image:
				if(param.type_int8) trigger_image< std::int8_t >(id);
				if(param.type_uint8) trigger_image< std::uint8_t >(id);
				if(param.type_int16) trigger_image< std::int16_t >(id);
				if(param.type_uint16) trigger_image< std::uint16_t >(id);
				if(param.type_int32) trigger_image< std::int32_t >(id);
				if(param.type_uint32) trigger_image< std::uint32_t >(id);
				if(param.type_int64) trigger_image< std::int64_t >(id);
				if(param.type_uint64) trigger_image< std::uint64_t >(id);
				if(param.type_float) trigger_image< float >(id);
				if(param.type_double) trigger_image< double >(id);
				if(param.type_long_double) trigger_image< long double >(id);
			break;
		}
	}


	template < typename T >
	void module::load_bitmap(bitmap< T >& bitmap, std::size_t id, std::size_t used_id, std::size_t cam, std::size_t pos){
		auto filename = param.dir + "/" + (*param.big_pattern)(used_id, cam, pos);
		disposer::log([this, &filename, id](log::info& os){ os << type << " id " << id << ": read '" << filename << "'"; }, [&bitmap, &filename]{
			big::read(bitmap, filename);
		});
	}

	template < typename T >
	void module::load_bitmap(bitmap< T >& bitmap, tar_reader& tar, std::string const& tarname, std::size_t id, std::size_t used_id, std::size_t cam, std::size_t pos){
		auto filename = (*param.big_pattern)(used_id, cam, pos);
		disposer::log([this, &tarname, &filename, id](log::info& os){ os << type << " id " << id << ": read '" << tarname << "'"; }, [&tar, &bitmap, &filename]{
			tar.read(filename, [&bitmap](std::istream& is){
				big::read(bitmap, is);
			});
		});
	}


} }
