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
#include "camera_sequence.hpp"
#include "name_generator.hpp"
#include "big_read.hpp"
#include "tar.hpp"

#include <disposer/module_base.hpp>


namespace disposer_module{ namespace big_loader{


	enum class output_t{
		camera_sequence,
		sequence,
		image
	};

	struct parameter{
		bool tar;

		std::size_t sequence_count;
		std::size_t camera_count;

		std::size_t sequence_start;
		std::size_t camera_start;

		std::string dir;

		boost::optional< std::size_t > fixed_id;

		output_t output;

		boost::optional< name_generator< std::size_t > > tar_pattern;
		boost::optional< name_generator< std::size_t, std::size_t, std::size_t > > big_pattern;
	};

	template < typename T >
	struct module: disposer::module_base{
		module(std::string const& type, std::string const& chain, std::string const& name, parameter&& param):
			disposer::module_base(type, chain, name),
			param(std::move(param)){
				outputs = disposer::make_output_list(camera_sequence, sequence, image);
			}

		disposer::output< camera_pointer_sequence< T > > camera_sequence{"camera_sequence"};
		disposer::output< bitmap_pointer_sequence< T > > sequence{"sequence"};
		disposer::output< bitmap< T > > image{"image"};

		std::string tar_name(std::size_t id);
		std::string big_name(std::size_t cam, std::size_t pos);
		std::string big_name(std::size_t id, std::size_t cam, std::size_t pos);

		void trigger_camera_sequence(std::size_t id);
		void trigger_sequence(std::size_t id);
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

		void load_bitmap(bitmap< T >& bitmap, std::size_t id, std::size_t used_id, std::size_t cam, std::size_t pos);
		void load_bitmap(bitmap< T >& bitmap, tar_reader& tar, std::string const& tarname, std::size_t id, std::size_t used_id, std::size_t cam, std::size_t pos);
	};

	template < typename T >
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
			outputs.find("camera_sequence") != outputs.end(),
			outputs.find("sequence") != outputs.end(),
			outputs.find("image") != outputs.end()
		}};

		std::size_t output_count = std::count(use_output.begin(), use_output.end(), true);

		if(output_count > 1){
			throw std::logic_error(type + ": Can only use one output ('image', 'sequence' or 'camera_sequence')");
		}

		if(output_count == 0){
			throw std::logic_error(type + ": No output defined (use 'image', 'sequence' or 'camera_sequence')");
		}

		parameter param;

		params.set(param.tar, "tar", false);

		params.set(param.sequence_count, "sequence_count");
		params.set(param.camera_count, "camera_count");

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

		if(use_output[0]){
			param.output = output_t::camera_sequence;
		}else if(use_output[1]){
			param.output = output_t::sequence;
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

		return std::make_unique< module< T > >(type, chain, name, std::move(param));
	}

	void init(){
		add_module_maker("big_loader_int8_t", &make_module< std::int8_t >);
		add_module_maker("big_loader_uint8_t", &make_module< std::uint8_t >);
		add_module_maker("big_loader_int16_t", &make_module< std::int16_t >);
		add_module_maker("big_loader_uint16_t", &make_module< std::uint16_t >);
		add_module_maker("big_loader_int32_t", &make_module< std::int32_t >);
		add_module_maker("big_loader_uint32_t", &make_module< std::uint32_t >);
		add_module_maker("big_loader_int64_t", &make_module< std::int64_t >);
		add_module_maker("big_loader_uint64_t", &make_module< std::uint64_t >);
		add_module_maker("big_loader_float", &make_module< float >);
		add_module_maker("big_loader_double", &make_module< double >);
		add_module_maker("big_loader_long_double", &make_module< long double >);
	}


	template < typename T >
	void module< T >::trigger_camera_sequence(std::size_t id){
		auto used_id = param.fixed_id ? *param.fixed_id : id;

		camera_pointer_sequence< T > result;

		if(param.tar){
			auto tarname = param.dir + "/" + (*param.tar_pattern)(used_id);
			disposer::log(tar_log{*this, tarname, id}, [this, id, used_id, &tarname, &result]{
				tar_reader tar(tarname);

				for(std::size_t cam = param.camera_start; cam < param.camera_count + param.camera_start; ++cam){
					result.emplace_back(std::make_shared< bitmap_pointer_sequence< T > >());
					for(std::size_t pos = param.sequence_start; pos < param.sequence_count + param.sequence_start; ++pos){
						result.back()->emplace_back(std::make_shared< bitmap< T > >());

						load_bitmap(*result.back()->back(), tar, tarname, id, used_id, cam, pos);
					}
				}
			});
		}else{
			for(std::size_t cam = param.camera_start; cam < param.camera_count + param.camera_start; ++cam){
				result.emplace_back(std::make_shared< bitmap_pointer_sequence< T > >());
				for(std::size_t pos = param.sequence_start; pos < param.sequence_count + param.sequence_start; ++pos){
					result.back()->emplace_back(std::make_shared< bitmap< T > >());

					load_bitmap(*result.back()->back(), id, used_id, cam, pos);
				}
			}
		}

		camera_sequence.put(id, std::move(result));
	}

	template < typename T >
	void module< T >::trigger_sequence(std::size_t id){
		auto used_id = param.fixed_id ? *param.fixed_id : id;

		if(param.tar){
			auto tarname = param.dir + "/" + (*param.tar_pattern)(used_id);
			disposer::log(tar_log{*this, tarname, id}, [this, id, used_id, &tarname]{
				tar_reader tar(tarname);

				for(std::size_t cam = param.camera_start; cam < param.camera_count + param.camera_start; ++cam){
					bitmap_pointer_sequence< T > result;

					for(std::size_t pos = param.sequence_start; pos < param.sequence_count + param.sequence_start; ++pos){
						result.emplace_back(std::make_shared< bitmap< T > >());

						load_bitmap(*result.back(), tar, tarname, id, used_id, cam, pos);
					}

					sequence.put(id, std::move(result));
				}
			});
		}else{
			for(std::size_t cam = param.camera_start; cam < param.camera_count + param.camera_start; ++cam){
				bitmap_pointer_sequence< T > result;

				for(std::size_t pos = param.sequence_start; pos < param.sequence_count + param.sequence_start; ++pos){
					result.emplace_back(std::make_shared< bitmap< T > >());

					load_bitmap(*result.back(), id, used_id, cam, pos);
				}

				sequence.put(id, std::move(result));
			}
		}
	}

	template < typename T >
	void module< T >::trigger_image(std::size_t id){
		auto used_id = param.fixed_id ? *param.fixed_id : id;

		if(param.tar){
			auto tarname = param.dir + "/" + (*param.tar_pattern)(used_id);
			disposer::log(tar_log{*this, tarname, id}, [this, id, used_id, &tarname]{
				tar_reader tar(tarname);

				for(std::size_t cam = param.camera_start; cam < param.camera_count + param.camera_start; ++cam){
					for(std::size_t pos = param.sequence_start; pos < param.sequence_count + param.sequence_start; ++pos){
						bitmap< T > result;

						load_bitmap(result, tar, tarname, id, used_id, cam, pos);

						image.put(id, std::move(result));
					}
				}
			});
		}else{
			for(std::size_t cam = param.camera_start; cam < param.camera_count + param.camera_start; ++cam){
				for(std::size_t pos = param.sequence_start; pos < param.sequence_count + param.sequence_start; ++pos){
					bitmap< T > result;

					load_bitmap(result, id, used_id, cam, pos);

					image.put(id, std::move(result));
				}
			}
		}
	}

	template < typename T >
	void module< T >::trigger(std::size_t id){
		switch(param.output){
			case output_t::camera_sequence:
				trigger_camera_sequence(id);
			break;
			case output_t::sequence:
				trigger_sequence(id);
			break;
			case output_t::image:
				trigger_image(id);
			break;
		}
	}

	template < typename T >
	void module< T >::load_bitmap(bitmap< T >& bitmap, std::size_t id, std::size_t used_id, std::size_t cam, std::size_t pos){
		auto filename = param.dir + "/" + (*param.big_pattern)(used_id, cam, pos);
		disposer::log([this, &filename, id](log::info& os){ os << type << " id " << id << ": read '" << filename << "'"; }, [&bitmap, &filename]{
			big::read(bitmap, filename);
		});
	}

	template < typename T >
	void module< T >::load_bitmap(bitmap< T >& bitmap, tar_reader& tar, std::string const& tarname, std::size_t id, std::size_t used_id, std::size_t cam, std::size_t pos){
		auto filename = (*param.big_pattern)(used_id, cam, pos);
		disposer::log([this, &tarname, &filename, id](log::info& os){ os << type << " id " << id << ": read '" << tarname << "'"; }, [&tar, &bitmap, &filename]{
			tar.read(filename, [&bitmap](std::istream& is){
				big::read(bitmap, is);
			});
		});
	}


} }
