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

	template < typename T > struct type_value;
	template <> struct type_value< std::int8_t >{ static constexpr std::size_t value = 0; };
	template <> struct type_value< std::uint8_t >{ static constexpr std::size_t value = 1; };
	template <> struct type_value< std::int16_t >{ static constexpr std::size_t value = 2; };
	template <> struct type_value< std::uint16_t >{ static constexpr std::size_t value = 3; };
	template <> struct type_value< std::int32_t >{ static constexpr std::size_t value = 4; };
	template <> struct type_value< std::uint32_t >{ static constexpr std::size_t value = 5; };
	template <> struct type_value< std::int64_t >{ static constexpr std::size_t value = 6; };
	template <> struct type_value< std::uint64_t >{ static constexpr std::size_t value = 7; };
	template <> struct type_value< float >{ static constexpr std::size_t value = 8; };
	template <> struct type_value< double >{ static constexpr std::size_t value = 9; };
	template <> struct type_value< long double >{ static constexpr std::size_t value = 10; };

	template < typename T >
	constexpr std::size_t type_v = type_value< T >::value;

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


	struct load_files{
		load_files(std::string const& type, parameter const& param, std::size_t id, std::size_t used_id):
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

	struct load_tar{
		load_tar(std::string const& type, parameter const& param, std::size_t id, std::size_t used_id, tar_reader& tar, std::string const& tarname):
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


		disposer::container_output< bitmap_sequence, types > sequence{"sequence"};

		disposer::container_output< bitmap_vector, types > vector{"vector"};

		disposer::container_output< bitmap, types > image{"image"};


		std::size_t get_type(std::istream& is, std::size_t id, std::string const& filename)const;

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

		params.set(param.type[type_v< std::int8_t >], "type_int8", false);
		params.set(param.type[type_v< std::uint8_t >], "type_uint8", false);
		params.set(param.type[type_v< std::int16_t >], "type_int16", false);
		params.set(param.type[type_v< std::uint16_t >], "type_uint16", false);
		params.set(param.type[type_v< std::int32_t >], "type_int32", false);
		params.set(param.type[type_v< std::uint32_t >], "type_uint32", false);
		params.set(param.type[type_v< std::int64_t >], "type_int64", false);
		params.set(param.type[type_v< std::int64_t >], "type_uint64", false);
		params.set(param.type[type_v< float >], "type_float", false);
		params.set(param.type[type_v< double >], "type_double", false);
		params.set(param.type[type_v< long double >], "type_long_double", false);

		if(param.type == std::array< bool, 11 >{{false}}){
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

		auto activate = [&result](auto type_t){
			switch(result->param.output){
				case output_t::sequence:
					result->sequence.activate< typename decltype(type_t)::type >();
				break;
				case output_t::vector:
					result->vector.activate< typename decltype(type_t)::type >();
				break;
				case output_t::image:
					result->image.activate< typename decltype(type_t)::type >();
				break;
			}
		};

		if(result->param.type[type_v< std::int8_t >]) activate(hana::type< std::int8_t >);
		if(result->param.type[type_v< std::uint8_t >]) activate(hana::type< std::uint8_t >);
		if(result->param.type[type_v< std::int16_t >]) activate(hana::type< std::int16_t >);
		if(result->param.type[type_v< std::uint16_t >]) activate(hana::type< std::uint16_t >);
		if(result->param.type[type_v< std::int32_t >]) activate(hana::type< std::int32_t >);
		if(result->param.type[type_v< std::uint32_t >]) activate(hana::type< std::uint32_t >);
		if(result->param.type[type_v< std::int64_t >]) activate(hana::type< std::int64_t >);
		if(result->param.type[type_v< std::int64_t >]) activate(hana::type< std::uint64_t >);
		if(result->param.type[type_v< float >]) activate(hana::type< float >);
		if(result->param.type[type_v< double >]) activate(hana::type< double >);
		if(result->param.type[type_v< long double >]) activate(hana::type< long double >);

		return std::move(result);
	}

	void init(){
		add_module_maker("big_loader", &make_module);
	}


	std::size_t module::get_type(std::istream& is, std::size_t id, std::string const& filename)const{
		return disposer::log([this, id, &filename](log::info& os){ os << type << " id " << id << ": read header of '" << filename << "'"; }, [&is, &filename]{
			auto header = big::read_header(is);
			switch(header.type){
				case big::type_v< std::int8_t >: return type_v< std::int8_t >;
				case big::type_v< std::uint8_t >: return type_v< std::uint8_t >;
				case big::type_v< std::int16_t >: return type_v< std::int16_t >;
				case big::type_v< std::uint16_t >: return type_v< std::uint16_t >;
				case big::type_v< std::int32_t >: return type_v< std::int32_t >;
				case big::type_v< std::uint32_t >: return type_v< std::uint32_t >;
				case big::type_v< std::int64_t >: return type_v< std::int64_t >;
				case big::type_v< std::uint64_t >: return type_v< std::int64_t >;
				case big::type_v< float >: return type_v< float >;
				case big::type_v< double >: return type_v< double >;
				case big::type_v< long double >: return type_v< long double >;
			}

			throw std::runtime_error("file '" + filename + "' has unknown big type");
		});
	}


	void module::trigger(std::size_t id){
		auto used_id = param.fixed_id ? *param.fixed_id : id;

		auto call_worker = [this, id](auto& loader, std::size_t data_type){
			auto worker = [this, id, &loader](bool active, std::string const& name, auto type_t){
				if(active){
					switch(param.output){
						case output_t::sequence:{
							sequence.put< typename decltype(type_t)::type >(id, loader.template load_sequence< typename decltype(type_t)::type >());
						} break;
						case output_t::vector:{
							for(std::size_t cam = param.camera_start; cam < param.camera_count + param.camera_start; ++cam){
								vector.put< typename decltype(type_t)::type >(id, loader.template load_vector< typename decltype(type_t)::type >(cam));
							}
						} break;
						case output_t::image:{
							for(std::size_t cam = param.camera_start; cam < param.camera_count + param.camera_start; ++cam){
								for(std::size_t pos = param.sequence_start; pos < param.sequence_count + param.sequence_start; ++pos){
									image.put< typename decltype(type_t)::type >(id, loader.template load_bitmap< typename decltype(type_t)::type >(cam, pos));
								}
							}
						} break;
					}
				}else{
					throw std::runtime_error(type + ": First file is of type '" + name + "', but parameter 'type_" + name + "' is not true");
				}
			};

			switch(data_type){
				case type_v< std::int8_t >: worker(param.type[type_v< std::int8_t >], "int8", hana::type< std::int8_t >); break;
				case type_v< std::uint8_t >: worker(param.type[type_v< std::uint8_t >], "uint8", hana::type< std::uint8_t >); break;
				case type_v< std::int16_t >: worker(param.type[type_v< std::int16_t >], "int16", hana::type< std::int16_t >); break;
				case type_v< std::uint16_t >: worker(param.type[type_v< std::uint16_t >], "uint16", hana::type< std::uint16_t >); break;
				case type_v< std::int32_t >: worker(param.type[type_v< std::int32_t >], "int32", hana::type< std::int32_t >); break;
				case type_v< std::uint32_t >: worker(param.type[type_v< std::uint32_t >], "uint32", hana::type< std::uint32_t >); break;
				case type_v< std::int64_t >: worker(param.type[type_v< std::int64_t >], "int64", hana::type< std::int64_t >); break;
				case type_v< std::uint64_t >: worker(param.type[type_v< std::uint64_t >], "uint64", hana::type< std::uint64_t >); break;
				case type_v< float >: worker(param.type[type_v< float >], "float", hana::type< float >); break;
				case type_v< double >: worker(param.type[type_v< double >], "double", hana::type< double >); break;
				case type_v< long double >: worker(param.type[type_v< long double >], "long double", hana::type< long double >); break;
			}
		};

		if(param.tar){
			auto tarname = param.dir + "/" + (*param.tar_pattern)(used_id);
			tar_reader tar(tarname);

			load_tar loader(type, param, id, used_id, tar, tarname);

			auto data_type = [this, id, &loader, &tar, &tarname](){
				auto filename = loader.filename(param.camera_start, param.sequence_start);
				return get_type(tar.get(filename), id, tarname + "/" + filename);
			}();

			call_worker(loader, data_type);
		}else{
			load_files loader(type, param, id, used_id);

			auto data_type = [this, id, &loader](){
				auto filename = loader.filename(param.camera_start, param.sequence_start);
				std::ifstream is(filename);
				return get_type(is, id, filename);
			}();

			call_worker(loader, data_type);
		}
	}


} }
