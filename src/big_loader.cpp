//-----------------------------------------------------------------------------
// Copyright (c) 2015-2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "bitmap_sequence.hpp"
#include "name_generator.hpp"

#include <tar/tar.hpp>

#include <big/read.hpp>

#include <disposer/module.hpp>
#include <disposer/type_position.hpp>

#include <boost/hana.hpp>
#include <boost/dll.hpp>


namespace disposer_module{ namespace big_loader{


	namespace hana = boost::hana;

	using namespace hana::literals;


	constexpr auto map_to = [](auto&& set, auto const& value){
		return hana::unpack(
				static_cast< decltype(set) >(set), hana::make_map ^hana::on^
				hana::curry< 2 >(hana::flip(hana::make_pair))(value)
			);
	};



	enum class output_t{
		sequence,
		vector,
		image
	};

	constexpr auto type_map = hana::make_map(
			hana::make_pair(hana::type_c< std::int8_t >, "int8"),
			hana::make_pair(hana::type_c< std::uint8_t >, "uint8"),
			hana::make_pair(hana::type_c< std::int16_t >, "int16"),
			hana::make_pair(hana::type_c< std::uint16_t >, "uint16"),
			hana::make_pair(hana::type_c< std::int32_t >, "int32"),
			hana::make_pair(hana::type_c< std::uint32_t >, "uint32"),
			hana::make_pair(hana::type_c< std::int64_t >, "int64"),
			hana::make_pair(hana::type_c< std::uint64_t >, "uint64"),
			hana::make_pair(hana::type_c< float >, "float32"),
			hana::make_pair(hana::type_c< double >, "float64")
		);

	constexpr auto type_list = hana::to_set(hana::keys(type_map));

	struct parameter{
		bool tar;

		std::size_t sequence_count;
		std::size_t camera_count;

		std::size_t sequence_start;
		std::size_t camera_start;

		std::string dir;

		decltype(map_to(type_list, false)) type;

		std::optional< std::size_t > fixed_id;

		output_t output;

		std::optional< name_generator< std::size_t > > tar_pattern;
		std::optional< name_generator< std::size_t, std::size_t, std::size_t > >
			big_pattern;
	};


	struct module: disposer::module_base{
		module(disposer::make_data const& data, parameter&& param):
			disposer::module_base(data, {sequence, vector, image}),
			param(std::move(param))
			{}


		disposer::container_output< bitmap_sequence, decltype(type_list) >
			sequence{"sequence"};

		disposer::container_output< bitmap_vector, decltype(type_list) >
			vector{"vector"};

		disposer::container_output< bitmap, decltype(type_list) >
			image{"image"};


		std::size_t get_big_type(
			std::istream& is,
			std::string const& filename)const;


		void exec()override;


		void input_ready()override;


		parameter const param;
	};


	disposer::module_ptr make_module(disposer::make_data& data){
		std::array< bool, 3 > const use_output{{
			data.outputs.find("sequence") != data.outputs.end(),
			data.outputs.find("vector") != data.outputs.end(),
			data.outputs.find("image") != data.outputs.end()
		}};

		std::size_t output_count =
			std::count(use_output.begin(), use_output.end(), true);

		if(output_count > 1){
			throw std::logic_error(
				"can only use one output ('image', 'vector' or 'sequence')");
		}

		if(output_count == 0){
			throw std::logic_error(
				"no output defined (use 'image', 'vector' or 'sequence')");
		}

		parameter param;

		data.params.set(param.tar, "tar", false);

		data.params.set(param.sequence_count, "sequence_count");
		data.params.set(param.camera_count, "camera_count");

		data.params.set(param.sequence_start, "sequence_start", 0);
		data.params.set(param.camera_start, "camera_start", 0);

		data.params.set(param.dir, "dir", ".");

		hana::for_each(type_map, [&data, &param](auto type_t){
			data.params.set(
				param.type[hana::first(type_t)],
				std::string("type_") + hana::second(type_t),
				false);
		});

		if(param.type == std::array< bool, hana::length(type_list) >{{false}}){
			throw std::logic_error(
				"no type enabled (set at least one of 'type_int8', "
				"'type_uint8', 'type_int16', 'type_uint16', 'type_int32', "
				"'type_uint32', 'type_int64', 'type_uint64', 'type_float', "
				"'type_double', 'type_long_double' to true)"
			);
		}

		std::size_t id_digits;
		std::size_t camera_digits;
		std::size_t position_digits;
		data.params.set(id_digits, "id_digits", 3);
		data.params.set(camera_digits, "camera_digits", 1);
		data.params.set(position_digits, "position_digits", 3);

		data.params.set(param.fixed_id, "fixed_id");

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
				data.params.get< std::string >("tar_pattern", "${id}.tar"),
				{{true}},
				std::make_pair("id", format{id_digits})
			);
		}

		param.big_pattern = make_name_generator(
			data.params.get("big_pattern",
				param.tar
					? std::string("${cam}_${pos}.big")
					: std::string("${id}_${cam}_${pos}.big")),
				{{!param.tar, true, true}},
				std::make_pair("id", format{id_digits}),
				std::make_pair("cam", format{camera_digits}),
				std::make_pair("pos", format{position_digits})
		);

		return std::make_unique< module >(data, std::move(param));
	}


	void module::input_ready(){
		auto enable = [this](auto type_t){
			switch(param.output){
				case output_t::sequence:
					sequence.enable< typename decltype(type_t)::type >();
				break;
				case output_t::vector:
					vector.enable< typename decltype(type_t)::type >();
				break;
				case output_t::image:
					image.enable< typename decltype(type_t)::type >();
				break;
			}
		};

		hana::for_each(type_list, [this, &enable](auto type_t){
			if(param.type[type_t]) enable(type_t);
		});
	}


	struct load_files{
		load_files(module const& loader, std::size_t used_id):
			loader(loader), used_id(used_id) {}

		module const& loader;
		std::size_t used_id;

		std::string filename(std::size_t cam, std::size_t pos)const{
			return loader.param.dir + "/" +
				(*loader.param.big_pattern)(used_id, cam, pos);
		}

		template < typename T >
		bitmap< T > load_bitmap(std::size_t cam, std::size_t pos)const{
			bitmap< T > result;
			auto name = filename(cam, pos);
			loader.log([this, &name](disposer::log_base& os){
				os << "read '" << name << "'";
			}, [&result, &name]{
				big::read(result, name);
			});
			return result;
		}

		template < typename T >
		bitmap_vector< T > load_vector(std::size_t cam)const{
			bitmap_vector< T > result;
			result.reserve(loader.param.sequence_count);
			for(
				std::size_t pos = loader.param.sequence_start;
				pos < loader.param.sequence_count + loader.param.sequence_start;
				++pos
			){
				result.push_back(load_bitmap< T >(cam, pos));
			}
			return result;
		}

		template < typename T >
		bitmap_sequence< T > load_sequence()const{
			bitmap_sequence< T > result;
			result.reserve(loader.param.camera_count);
			for(
				std::size_t cam = loader.param.camera_start;
				cam < loader.param.camera_count + loader.param.camera_start;
				++cam
			){
				result.push_back(load_vector< T >(cam));
			}
			return result;
		}
	};


	struct load_tar{
		load_tar(
			module const& loader,
			std::size_t used_id,
			::tar::tar_reader& tar,
			std::string const& tarname
		):
			loader(loader), used_id(used_id), tar(tar), tarname(tarname) {}

		module const& loader;
		std::size_t used_id;
		::tar::tar_reader& tar;
		std::string const& tarname;

		std::string filename(std::size_t cam, std::size_t pos)const{
			return (*loader.param.big_pattern)(used_id, cam, pos);
		}

		template < typename T >
		bitmap< T > load_bitmap(std::size_t cam, std::size_t pos)const{
			bitmap< T > result;
			auto name = filename(cam, pos);
			loader.log([this, &name](disposer::log_base& os){
				os << "read '" << tarname << "/" << name << "'";
			}, [this, &result, &name]{
				big::read(result, tar.get(name));
			});
			return result;
		}

		template < typename T >
		bitmap_vector< T > load_vector(std::size_t cam)const{
			bitmap_vector< T > result;
			result.reserve(loader.param.sequence_count);
			for(
				std::size_t pos = loader.param.sequence_start;
				pos < loader.param.sequence_count + loader.param.sequence_start;
				++pos
			){
				result.push_back(load_bitmap< T >(cam, pos));
			}
			return result;
		}

		template < typename T >
		bitmap_sequence< T > load_sequence()const{
			bitmap_sequence< T > result;
			result.reserve(loader.param.camera_count);
			for(
				std::size_t cam = loader.param.camera_start;
				cam < loader.param.camera_count + loader.param.camera_start;
				++cam
			){
				result.push_back(load_vector< T >(cam));
			}
			return result;
		}
	};


	std::size_t module::get_big_type(
		std::istream& is,
		std::string const& filename
	)const{
		return log([this, &filename](disposer::log_base& os){
			os << "read header of '" << filename << "'";
		}, [&is, &filename]{
			auto header = big::read_header(is);
			switch(header.type){
				case big::type_v< std::int8_t >:
				case big::type_v< std::uint8_t >:
				case big::type_v< std::int16_t >:
				case big::type_v< std::uint16_t >:
				case big::type_v< std::int32_t >:
				case big::type_v< std::uint32_t >:
				case big::type_v< std::int64_t >:
				case big::type_v< std::uint64_t >:
				case big::type_v< float >:
				case big::type_v< double >:
					return header.type;
				default:
					throw std::runtime_error(
						"file '" + filename + "' has unsupported big type");
			}
		});
	}


	void module::exec(){
		auto used_id = param.fixed_id ? *param.fixed_id : id;

		auto call_worker = [this](auto& loader, std::size_t big_type){
			auto worker = [this, &loader](auto type_t){
				using data_type = typename decltype(type_t)::type;

				log([this, type_t](disposer::log_base& os){
					os << "data type is '" << type_map[type_t] << "'";
				});

				if(param.type[type_t]){
					switch(param.output){
						case output_t::sequence:{
							sequence.put_by_subtype< data_type >(
								loader.template load_sequence< data_type >());
						} break;
						case output_t::vector:{
							for(
								std::size_t cam = param.camera_start;
								cam < param.camera_count + param.camera_start;
								++cam
							){
								vector.put_by_subtype< data_type >(loader.
									template load_vector< data_type >(cam));
							}
						} break;
						case output_t::image:{
							for(
								std::size_t cam = param.camera_start;
								cam < param.camera_count + param.camera_start;
								++cam
							){
								for(
									std::size_t pos = param.sequence_start;
									pos < param.sequence_count
										+ param.sequence_start;
									++pos
								){
									image.put_by_subtype< data_type >(loader.
										template load_bitmap< data_type >(
											cam, pos));
								}
							}
						} break;
					}
				}else{
					throw std::runtime_error(
						std::string("first file is of type '")
						+ type_map[type_t]
						+ "', but parameter 'type_"
						+ type_map[type_t]
						+ "' is not true");
				}
			};

			switch(big_type){
				case big::type_v< std::int8_t >:
					worker(hana::type_c< std::int8_t >); break;
				case big::type_v< std::uint8_t >:
					worker(hana::type_c< std::uint8_t >); break;
				case big::type_v< std::int16_t >:
					worker(hana::type_c< std::int16_t >); break;
				case big::type_v< std::uint16_t >:
					worker(hana::type_c< std::uint16_t >); break;
				case big::type_v< std::int32_t >:
					worker(hana::type_c< std::int32_t >); break;
				case big::type_v< std::uint32_t >:
					worker(hana::type_c< std::uint32_t >); break;
				case big::type_v< std::int64_t >:
					worker(hana::type_c< std::int64_t >); break;
				case big::type_v< std::uint64_t >:
					worker(hana::type_c< std::uint64_t >); break;
				case big::type_v< float >:
					worker(hana::type_c< float >); break;
				case big::type_v< double >:
					worker(hana::type_c< double >); break;
			}
		};

		if(param.tar){
			auto tarname = param.dir + "/" + (*param.tar_pattern)(used_id);
			::tar::tar_reader tar(tarname);

			load_tar loader(*this, used_id, tar, tarname);

			auto big_type = [this, &loader, &tar, &tarname](){
				auto filename = loader.filename(
					param.camera_start, param.sequence_start);
				return get_big_type(
					tar.get(filename), tarname + "/" + filename);
			}();

			call_worker(loader, big_type);
		}else{
			load_files loader(*this, used_id);

			auto big_type = [this, &loader](){
				auto filename = loader.filename(
					param.camera_start, param.sequence_start);
				std::ifstream is(filename);

				if(!is.is_open()){
					throw big::big_error("can't open file: " + filename);
				}

				return get_big_type(is, filename);
			}();

			call_worker(loader, big_type);
		}
	}


	void init(disposer::module_declarant& add){
		add("big_loader", &make_module);
	}

	BOOST_DLL_AUTO_ALIAS(init)


} }
