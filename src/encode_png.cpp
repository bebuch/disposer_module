//-----------------------------------------------------------------------------
// Copyright (c) 2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <bitmap/bitmap.hpp>
#include <bitmap/pixel.hpp>

#include <disposer/module.hpp>

#include <boost/dll.hpp>

#include <png++/png.hpp>


namespace disposer_module{ namespace encode_png{


	namespace pixel = ::bitmap::pixel;
	using ::bitmap::bitmap;


	struct module: disposer::module_base{
		module(disposer::make_data const& mdata):
			disposer::module_base(mdata, {slots.image}, {signals.data})
			{}


		using types = disposer::type_list<
			std::uint8_t,
			std::uint16_t,
			pixel::ga8,
			pixel::ga16,
			pixel::rgb8,
			pixel::rgb16,
			pixel::rgba8,
			pixel::rgba16
		>;

		struct{
			disposer::container_input< bitmap, types > image{"image"};
		} slots;

		struct{
			disposer::output< std::string > data{"data"};
		} signals;


		void input_ready()override{
			signals.data.enable< std::string >();
		}


		void exec()override;
	};


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


	disposer::module_ptr make_module(disposer::make_data& data){
		if(data.is_first()) throw disposer::module_not_as_start(data);

		if(data.inputs.find("image") == data.inputs.end()){
			throw std::logic_error("no input defined (use 'image')");
		}

		if(data.outputs.find("data") == data.outputs.end()){
			throw std::logic_error("no output defined (use 'data')");
		}

		return std::make_unique< module >(data);
	}


	namespace{


		template < typename T >
		auto to_png_image(bitmap< T > const& img){
			using png_type = bitmap_to_png_type_t< T >;

			png::image< png_type > png_image(img.width(), img.height());

			for(std::size_t y = 0; y < img.height(); ++y){
				for(std::size_t x = 0; x < img.width(); ++x){
					auto& pixel = img(x, y);
					png_image.set_pixel(x, y,
						*reinterpret_cast< png_type const* >(&pixel));
				}
			}

			return png_image;
		}


	}


	void module::exec(){
		for(auto const& pair: slots.image.get()){
			auto encoded_data = std::visit([](auto const& img){
					std::ostringstream os;
					to_png_image(img.data()).write_stream(os);
					return os.str();
				}, pair.second);
			signals.data.put(std::move(encoded_data));
		}
	}


	void init(disposer::module_declarant& add){
		add("encode_png", &make_module);
	}

	BOOST_DLL_AUTO_ALIAS(init)


} }
