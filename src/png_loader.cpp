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
#include "tar.hpp"

#include <bitmap/pixel.hpp>

#include <disposer/module.hpp>
#include <disposer/type_position.hpp>

#include <boost/hana.hpp>
#include <boost/dll.hpp>

#include <png++/png.hpp>


namespace disposer_module{ namespace png_loader{


	namespace hana = boost::hana;
	namespace pixel = ::bitmap::pixel;

	using disposer::type_position_v;

	enum class output_t{
		sequence,
		vector,
		image
	};

	using type_list = disposer::type_list<
		std::uint8_t,
		std::uint16_t,
		pixel::ga8,
		pixel::ga16,
		pixel::rgb8,
		pixel::rgb16,
		pixel::rgba8,
		pixel::rgba16
	>;

	constexpr auto hana_type_list = disposer::hana_type_list< type_list >;

	template < typename T >
	constexpr std::size_t type_v = type_position_v< T, type_list >;

	constexpr std::array< char const*, type_list::size > io_type_names{{
		"gray8",
		"gray16",
		"grayalpha8",
		"grayalpha16",
		"rgb8",
		"rgb16",
		"rgba8",
		"rgba16"
	}};


	struct parameter{
		bool tar;

		std::size_t sequence_count;
		std::size_t camera_count;

		std::size_t sequence_start;
		std::size_t camera_start;

		std::string dir;

		std::array< bool, type_list::size > type;

		std::optional< std::size_t > fixed_id;

		output_t output;

		std::optional< name_generator< std::size_t > > tar_pattern;
		std::optional< name_generator< std::size_t, std::size_t, std::size_t > >
			png_pattern;
	};


	struct module: disposer::module_base{
		module(disposer::make_data const& data, parameter&& param):
			disposer::module_base(data, {sequence, vector, image}),
			param(std::move(param))
			{}


		disposer::container_output< bitmap_sequence, type_list >
			sequence{"sequence"};

		disposer::container_output< bitmap_vector, type_list >
			vector{"vector"};

		disposer::container_output< bitmap, type_list >
			image{"image"};


		std::size_t get_type(
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

		hana::for_each(hana_type_list, [&data, &param](auto type_t){
			using data_type = typename decltype(type_t)::type;
			data.params.set(
				param.type[type_v< data_type >],
				std::string("type_") + io_type_names[type_v< data_type >],
				false);
		});

		if(param.type == std::array< bool, type_list::size >{{false}}){
			throw std::logic_error(
				"no type active (set at least one of 'type_gray8', "
				"'type_gray16', 'type_grayalpha8', 'type_grayalpha16', "
				"'type_rgb8', 'type_rgb16', 'type_rgba8', 'type_rgba16' "
				"to true)"
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

		param.png_pattern = make_name_generator(
			data.params.get("png_pattern",
				param.tar
					? std::string("${cam}_${pos}.png")
					: std::string("${id}_${cam}_${pos}.png")),
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

		hana::for_each(hana_type_list, [this, &enable](auto type_t){
			using data_type = typename decltype(type_t)::type;
			if(param.type[type_v< data_type >]) enable(type_t);
		});
	}


	template < typename T >
	struct bitmap_to_png_type;

	template <> struct bitmap_to_png_type< std::uint8_t >
		{ using type = png::gray_pixel; };

	template <> struct bitmap_to_png_type< std::uint16_t >
		{ using type = png::gray_pixel_16; };

	template <> struct bitmap_to_png_type< pixel::ga8 >
		{ using type = png::ga_pixel; };

	template <> struct bitmap_to_png_type< pixel::ga16 >
		{ using type = png::ga_pixel_16; };

	template <> struct bitmap_to_png_type< pixel::rgb8 >
		{ using type = png::rgb_pixel; };

	template <> struct bitmap_to_png_type< pixel::rgb16 >
		{ using type = png::rgb_pixel_16; };

	template <> struct bitmap_to_png_type< pixel::rgba8 >
		{ using type = png::rgba_pixel; };

	template <> struct bitmap_to_png_type< pixel::rgba16 >
		{ using type = png::rgba_pixel_16; };

	template < typename T >
	using bitmap_to_png_type_t = typename bitmap_to_png_type< T >::type;


	struct load_files{
		load_files(module const& loader, std::size_t used_id):
			loader(loader), used_id(used_id) {}

		module const& loader;
		std::size_t used_id;

		std::string filename(std::size_t cam, std::size_t pos)const{
			return loader.param.dir + "/" +
				(*loader.param.png_pattern)(used_id, cam, pos);
		}

		template < typename T >
		bitmap< T > load_bitmap(std::size_t cam, std::size_t pos)const{
			auto name = filename(cam, pos);
			return loader.log([this, &name](disposer::log_base& os){
				os << "read '" << name << "'";
			}, [&name]{
				png::image< bitmap_to_png_type_t< T > > img(name);
				bitmap< T > result(img.get_width(), img.get_height());

				for(std::size_t y = 0; y < img.get_height(); ++y){
					for(std::size_t x = 0; x < img.get_width(); ++x){
						auto pixel = img.get_pixel(x, y);
						result(x, y) = *reinterpret_cast< T const* >(&pixel);
					}
				}

				return result;
			});
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
			tar_reader& tar,
			std::string const& tarname
		):
			loader(loader), used_id(used_id), tar(tar), tarname(tarname) {}

		module const& loader;
		std::size_t used_id;
		tar_reader& tar;
		std::string const& tarname;

		std::string filename(std::size_t cam, std::size_t pos)const{
			return (*loader.param.png_pattern)(used_id, cam, pos);
		}

		template < typename T >
		bitmap< T > load_bitmap(std::size_t cam, std::size_t pos)const{
			auto name = filename(cam, pos);
			return loader.log([this, &name](disposer::log_base& os){
				os << "read '" << tarname << "/" << name << "'";
			}, [this, &name]{
				png::image< bitmap_to_png_type_t< T > > img(tar.get(name));
				bitmap< T > result(img.get_width(), img.get_height());

				for(std::size_t y = 0; y < img.get_height(); ++y){
					for(std::size_t x = 0; x < img.get_width(); ++x){
						auto pixel = img.get_pixel(x, y);
						result(x, y) = *reinterpret_cast< T const* >(&pixel);
					}
				}

				return result;
			});
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


	std::size_t module::get_type(
		std::istream& is,
		std::string const& filename
	)const{
		return log([this, &filename](disposer::log_base& os){
			os << "read header of '" << filename << "'";
		}, [&is, &filename]{
			png::reader< std::istream > reader(is);
			reader.read_info();
			auto info = reader.get_image_info();

			auto bits = info.get_bit_depth();
			auto color = info.get_color_type();
			if(bits == 1 || bits == 2 || bits == 4 || bits == 8){
				switch(color){
					case png::color_type_none: break;
					case png::color_type_gray:
						return type_v< std::uint8_t >;
					case png::color_type_palette:
						return type_v< pixel::rgba8 >;
					case png::color_type_rgb:
						return type_v< pixel::rgb8 >;
					case png::color_type_rgba:
						return type_v< pixel::rgba8 >;
					case png::color_type_ga:
						return type_v< pixel::ga8 >;
				}
			}else if(bits == 16){
				switch(color){
					case png::color_type_none: break;
					case png::color_type_gray:
						return type_v< std::uint16_t >;
					case png::color_type_rgb:
						return type_v< pixel::rgb16 >;
					case png::color_type_rgba:
						return type_v< pixel::rgba16 >;
					case png::color_type_ga:
						return type_v< pixel::ga16 >;
					case png::color_type_palette:
						break;
				}
			}else{
				throw std::logic_error(
					"unknown png bit depth: " + std::to_string(bits));
			}
			throw std::logic_error(
				"unknown png color type: " + std::to_string(color));
		});
	}


	void module::exec(){
		auto used_id = param.fixed_id ? *param.fixed_id : id;

		auto call_worker = [this](auto& loader, std::size_t data_type){
			log([this, data_type](disposer::log_base& os){
				os << "data type is '" << io_type_names[data_type] << "'";
			});

			auto worker = [this, &loader](auto type_t){
				using data_type = typename decltype(type_t)::type;

				if(param.type[type_v< data_type >]){
					switch(param.output){
						case output_t::sequence:{
							sequence.put< data_type >(
								loader.template load_sequence< data_type >());
						} break;
						case output_t::vector:{
							for(
								std::size_t cam = param.camera_start;
								cam < param.camera_count + param.camera_start;
								++cam
							){
								vector.put< data_type >(loader.template
									load_vector< data_type >(cam));
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
									image.put< data_type >(loader.template
										load_bitmap< data_type >(cam, pos));
								}
							}
						} break;
					}
				}else{
					throw std::runtime_error(
						std::string("first file is of type '")
						+ io_type_names[type_v< data_type >]
						+ "', but parameter 'type_"
						+ io_type_names[type_v< data_type >]
						+ "' is not true");
				}
			};

			switch(data_type){
				case type_v< std::uint8_t >:
					worker(hana::type_c< std::uint8_t >); break;
				case type_v< std::uint16_t >:
					worker(hana::type_c< std::uint16_t >); break;
				case type_v< pixel::ga8 >:
					worker(hana::type_c< pixel::ga8 >); break;
				case type_v< pixel::ga16 >:
					worker(hana::type_c< pixel::ga16 >); break;
			}
		};

		if(param.tar){
			auto tarname = param.dir + "/" + (*param.tar_pattern)(used_id);
			tar_reader tar(tarname);

			load_tar loader(*this, used_id, tar, tarname);

			auto data_type = [this, &loader, &tar, &tarname](){
				auto filename = loader.filename(
					param.camera_start, param.sequence_start);
				return get_type(tar.get(filename), tarname + "/" + filename);
			}();

			call_worker(loader, data_type);
		}else{
			load_files loader(*this, used_id);

			auto data_type = [this, &loader](){
				auto filename = loader.filename(
					param.camera_start, param.sequence_start);
				std::ifstream is(filename);

				if(!is.is_open()){
					throw std::runtime_error("can't open file: " + filename);
				}

				return get_type(is, filename);
			}();

			call_worker(loader, data_type);
		}
	}


	void init(disposer::module_declarant& add){
		add("png_loader", &make_module);
	}

	BOOST_DLL_AUTO_ALIAS(init)


} }
