//-----------------------------------------------------------------------------
// Copyright (c) 2015 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
// TODO: add type in log messages
#include "big_loader.hpp"

#include "log.hpp"
#include "bitmap_sequence.hpp"
#include "name_generator.hpp"
#include "big_read.hpp"
#include "tar.hpp"

#include <disposer/module_base.hpp>

#include <boost/hana.hpp>


namespace disposer_module{ namespace big_loader{


	namespace hana = boost::hana;

	enum class output_t{
		sequence,
		vector,
		image
	};

	struct data{
		enum type{
			int8 = 0,
			uint8,
			int16,
			uint16,
			int32,
			uint32,
			int64,
			uint64,
			float_,
			double_,
			long_double
		};
	};

	struct parameter{
		bool tar;

		std::size_t sequence_count;
		std::size_t camera_count;

		std::size_t sequence_start;
		std::size_t camera_start;

		std::string dir;

		std::array< bool, 11 > type;

		boost::optional< std::size_t > fixed_id;

		output_t output;

		boost::optional< name_generator< std::size_t > > tar_pattern;
		boost::optional< name_generator< std::size_t, std::size_t, std::size_t > > big_pattern;
	};


	struct load{
		load(std::string const& type, parameter const& param, std::size_t id, std::size_t used_id):
			type(type), param(param), id(id), used_id(used_id) {}

		std::string const& type;
		parameter const& param;
		std::size_t id;
		std::size_t used_id;

		std::string filename(std::size_t cam, std::size_t pos)const{
			return param.dir + "/" + (*param.big_pattern)(used_id, cam, pos);
		}

		template < typename T >
		bitmap< T > load_bitmap(std::size_t cam, std::size_t pos)const{
			bitmap< T > result;
			auto name = filename(cam, pos);
			disposer::log([this, &name](log::info& os){ os << type << " id " << id << ": read '" << name << "'"; }, [&result, &name]{
				big::read(result, name);
			});
			return result;
		}

		template < typename T >
		bitmap_vector< T > load_vector(std::size_t cam)const{
			bitmap_vector< T > result;
			result.reserve(param.sequence_count);
			for(std::size_t pos = param.sequence_start; pos < param.sequence_count + param.sequence_start; ++pos){
				result.push_back(load_bitmap< T >(cam, pos));
			}
			return result;
		}

		template < typename T >
		bitmap_sequence< T > load_sequence()const{
			bitmap_sequence< T > result;
			result.reserve(param.camera_count);
			for(std::size_t cam = param.camera_start; cam < param.camera_count + param.camera_start; ++cam){
				result.push_back(load_vector< T >(cam));
			}
			return result;
		}
	};

	struct tar_load{
		tar_load(std::string const& type, parameter const& param, std::size_t id, std::size_t used_id, tar_reader& tar, std::string const& tarname):
			type(type), param(param), id(id), used_id(used_id), tar(tar), tarname(tarname) {}

		std::string const& type;
		parameter const& param;
		std::size_t id;
		std::size_t used_id;
		tar_reader& tar;
		std::string const& tarname;

		std::string filename(std::size_t cam, std::size_t pos)const{
			return (*param.big_pattern)(used_id, cam, pos);
		}

		template < typename T >
		bitmap< T > load_bitmap(std::size_t cam, std::size_t pos)const{
			bitmap< T > result;
			auto name = filename(cam, pos);
			disposer::log([this, &name](log::info& os){ os << type << " id " << id << ": read '" << tarname << "/" << name << "'"; }, [this, &result, &name]{
				big::read(result, tar.get(name));
			});
			return result;
		}

		template < typename T >
		bitmap_vector< T > load_vector(std::size_t cam)const{
			bitmap_vector< T > result;
			result.reserve(param.sequence_count);
			for(std::size_t pos = param.sequence_start; pos < param.sequence_count + param.sequence_start; ++pos){
				result.push_back(load_bitmap< T >(cam, pos));
			}
			return result;
		}

		template < typename T >
		bitmap_sequence< T > load_sequence()const{
			bitmap_sequence< T > result;
			result.reserve(param.camera_count);
			for(std::size_t cam = param.camera_start; cam < param.camera_count + param.camera_start; ++cam){
				result.push_back(load_vector< T >(cam));
			}
			return result;
		}
	};


	struct module: disposer::module_base{
		module(std::string const& type, std::string const& chain, std::string const& name, parameter&& param):
			disposer::module_base(type, chain, name),
			param(std::move(param)){
				outputs = disposer::make_output_list(sequence, vector, image);
			}


		using types = disposer::type_list<
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
		>;


		disposer::container_output< bitmap_sequence, types > sequence{"sequence"};

		disposer::container_output< bitmap_vector, types > vector{"vector"};

		disposer::container_output< bitmap, types > image{"image"};


		data::type get_type(std::istream& is, std::size_t id, std::string const& filename)const;

		void trigger(std::size_t id)override;


		parameter const param;
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

		params.set(param.type[data::int8], "type_int8", false);
		params.set(param.type[data::uint8], "type_uint8", false);
		params.set(param.type[data::int16], "type_int16", false);
		params.set(param.type[data::uint16], "type_uint16", false);
		params.set(param.type[data::int32], "type_int32", false);
		params.set(param.type[data::uint32], "type_uint32", false);
		params.set(param.type[data::int64], "type_int64", false);
		params.set(param.type[data::uint64], "type_uint64", false);
		params.set(param.type[data::float_], "type_float", false);
		params.set(param.type[data::double_], "type_double", false);
		params.set(param.type[data::long_double], "type_long_double", false);

		if(
			!param.type[data::int8] &&
			!param.type[data::uint8] &&
			!param.type[data::int16] &&
			!param.type[data::uint16] &&
			!param.type[data::int32] &&
			!param.type[data::uint32] &&
			!param.type[data::int64] &&
			!param.type[data::uint64] &&
			!param.type[data::float_] &&
			!param.type[data::double_] &&
			!param.type[data::long_double]
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
				if(param.type[data::int8]) result->sequence.activate< std::int8_t >();
				if(param.type[data::uint8]) result->sequence.activate< std::uint8_t >();
				if(param.type[data::int16]) result->sequence.activate< std::int16_t >();
				if(param.type[data::uint16]) result->sequence.activate< std::uint16_t >();
				if(param.type[data::int32]) result->sequence.activate< std::int32_t >();
				if(param.type[data::uint32]) result->sequence.activate< std::uint32_t >();
				if(param.type[data::int64]) result->sequence.activate< std::int64_t >();
				if(param.type[data::uint64]) result->sequence.activate< std::uint64_t >();
				if(param.type[data::float_]) result->sequence.activate< float >();
				if(param.type[data::double_]) result->sequence.activate< double >();
				if(param.type[data::long_double]) result->sequence.activate< long double >();
			break;
			case output_t::vector:
				if(param.type[data::int8]) result->vector.activate< std::int8_t >();
				if(param.type[data::uint8]) result->vector.activate< std::uint8_t >();
				if(param.type[data::int16]) result->vector.activate< std::int16_t >();
				if(param.type[data::uint16]) result->vector.activate< std::uint16_t >();
				if(param.type[data::int32]) result->vector.activate< std::int32_t >();
				if(param.type[data::uint32]) result->vector.activate< std::uint32_t >();
				if(param.type[data::int64]) result->vector.activate< std::int64_t >();
				if(param.type[data::uint64]) result->vector.activate< std::uint64_t >();
				if(param.type[data::float_]) result->vector.activate< float >();
				if(param.type[data::double_]) result->vector.activate< double >();
				if(param.type[data::long_double]) result->vector.activate< long double >();
			break;
			case output_t::image:
				if(param.type[data::int8]) result->image.activate< std::int8_t >();
				if(param.type[data::uint8]) result->image.activate< std::uint8_t >();
				if(param.type[data::int16]) result->image.activate< std::int16_t >();
				if(param.type[data::uint16]) result->image.activate< std::uint16_t >();
				if(param.type[data::int32]) result->image.activate< std::int32_t >();
				if(param.type[data::uint32]) result->image.activate< std::uint32_t >();
				if(param.type[data::int64]) result->image.activate< std::int64_t >();
				if(param.type[data::uint64]) result->image.activate< std::uint64_t >();
				if(param.type[data::float_]) result->image.activate< float >();
				if(param.type[data::double_]) result->image.activate< double >();
				if(param.type[data::long_double]) result->image.activate< long double >();
			break;
		}

		return std::move(result);
	}

	void init(){
		add_module_maker("big_loader", &make_module);
	}


	data::type module::get_type(std::istream& is, std::size_t id, std::string const& filename)const{
		return disposer::log([this, id, &filename](log::info& os){ os << type << " id " << id << ": read header of '" << filename << "'"; }, [&is, &filename]{
			auto header = big::read_header(is);
			switch(header.type){
				case big::type_v< std::int8_t >: return data::int8;
				case big::type_v< std::uint8_t >: return data::uint8;
				case big::type_v< std::int16_t >: return data::int16;
				case big::type_v< std::uint16_t >: return data::uint16;
				case big::type_v< std::int32_t >: return data::int32;
				case big::type_v< std::uint32_t >: return data::uint32;
				case big::type_v< std::int64_t >: return data::int64;
				case big::type_v< std::uint64_t >: return data::uint64;
				case big::type_v< float >: return data::float_;
				case big::type_v< double >: return data::double_;
				case big::type_v< long double >: return data::long_double;
			}

			throw std::runtime_error("file '" + filename + "' has unknown big type");
		});
	}


	void module::trigger(std::size_t id){
		auto used_id = param.fixed_id ? *param.fixed_id : id;

		if(param.tar){
			auto tarname = param.dir + "/" + (*param.tar_pattern)(used_id);
			tar_reader tar(tarname);

			tar_load loader(type, param, id, used_id, tar, tarname);
			auto worker = [this, id, &loader](auto type_t){
				switch(param.output){
					case output_t::sequence:{
						sequence.put< typename decltype(type_t)::type >(id, loader.load_sequence< typename decltype(type_t)::type >());
					} break;
					case output_t::vector:{
						for(std::size_t cam = param.camera_start; cam < param.camera_count + param.camera_start; ++cam){
							vector.put< typename decltype(type_t)::type >(id, loader.load_vector< typename decltype(type_t)::type >(cam));
						}
					} break;
					case output_t::image:{
						for(std::size_t cam = param.camera_start; cam < param.camera_count + param.camera_start; ++cam){
							for(std::size_t pos = param.sequence_start; pos < param.sequence_count + param.sequence_start; ++pos){
								image.put< typename decltype(type_t)::type >(id, loader.load_bitmap< typename decltype(type_t)::type >(cam, pos));
							}
						}
					} break;
				}
			};

			auto call_worker = [this, &worker](bool active, std::string const& name, auto type_t){
				if(active){
					worker(type_t);
				}else{
					throw std::runtime_error(type + ": First file is of type '" + name + "', but parameter 'type_" + name + "' is not true");
				}
			};

			auto filename = loader.filename(param.camera_start, param.sequence_start);
			auto data_type = get_type(tar.get(filename), id, tarname + "/" + filename);

			switch(data_type){
				case data::int8: call_worker(param.type[data::int8], "int8", hana::type< std::int8_t >); break;
				case data::uint8: call_worker(param.type[data::uint8], "uint8", hana::type< std::uint8_t >); break;
				case data::int16: call_worker(param.type[data::int16], "int16", hana::type< std::int16_t >); break;
				case data::uint16: call_worker(param.type[data::uint16], "uint16", hana::type< std::uint16_t >); break;
				case data::int32: call_worker(param.type[data::int32], "int32", hana::type< std::int32_t >); break;
				case data::uint32: call_worker(param.type[data::uint32], "uint32", hana::type< std::uint32_t >); break;
				case data::int64: call_worker(param.type[data::int64], "int64", hana::type< std::int64_t >); break;
				case data::uint64: call_worker(param.type[data::uint64], "uint64", hana::type< std::uint64_t >); break;
				case data::float_: call_worker(param.type[data::float_], "float", hana::type< float >); break;
				case data::double_: call_worker(param.type[data::double_], "double", hana::type< double >); break;
				case data::long_double: call_worker(param.type[data::long_double], "long double", hana::type< long double >); break;
			}
		}else{
			load loader(type, param, id, used_id);
			auto worker = [this, id, &loader](auto type_t){
				switch(param.output){
					case output_t::sequence:{
						sequence.put< typename decltype(type_t)::type >(id, loader.load_sequence< typename decltype(type_t)::type >());
					} break;
					case output_t::vector:{
						for(std::size_t cam = param.camera_start; cam < param.camera_count + param.camera_start; ++cam){
							vector.put< typename decltype(type_t)::type >(id, loader.load_vector< typename decltype(type_t)::type >(cam));
						}
					} break;
					case output_t::image:{
						for(std::size_t cam = param.camera_start; cam < param.camera_count + param.camera_start; ++cam){
							for(std::size_t pos = param.sequence_start; pos < param.sequence_count + param.sequence_start; ++pos){
								image.put< typename decltype(type_t)::type >(id, loader.load_bitmap< typename decltype(type_t)::type >(cam, pos));
							}
						}
					} break;
				}
			};

			auto call_worker = [this, &worker](bool active, std::string const& name, auto type_t){
				if(active){
					worker(type_t);
				}else{
					throw std::runtime_error(type + ": First file is of type '" + name + "', but parameter 'type_" + name + "' is not true");
				}
			};

			auto data_type = [this, id, &loader](){
				auto filename = loader.filename(param.camera_start, param.sequence_start);
				std::ifstream is(filename);
				return get_type(is, id, filename);
			}();

			switch(data_type){
				case data::int8: call_worker(param.type[data::int8], "int8", hana::type< std::int8_t >); break;
				case data::uint8: call_worker(param.type[data::uint8], "uint8", hana::type< std::uint8_t >); break;
				case data::int16: call_worker(param.type[data::int16], "int16", hana::type< std::int16_t >); break;
				case data::uint16: call_worker(param.type[data::uint16], "uint16", hana::type< std::uint16_t >); break;
				case data::int32: call_worker(param.type[data::int32], "int32", hana::type< std::int32_t >); break;
				case data::uint32: call_worker(param.type[data::uint32], "uint32", hana::type< std::uint32_t >); break;
				case data::int64: call_worker(param.type[data::int64], "int64", hana::type< std::int64_t >); break;
				case data::uint64: call_worker(param.type[data::uint64], "uint64", hana::type< std::uint64_t >); break;
				case data::float_: call_worker(param.type[data::float_], "float", hana::type< float >); break;
				case data::double_: call_worker(param.type[data::double_], "double", hana::type< double >); break;
				case data::long_double: call_worker(param.type[data::long_double], "long double", hana::type< long double >); break;
			}
		}
	}


} }
