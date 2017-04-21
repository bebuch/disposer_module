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

#include <turbojpeg.h>


namespace disposer_module{ namespace encode_jpg{


	namespace pixel = ::bitmap::pixel;
	using ::bitmap::bitmap;


	struct module: disposer::module_base{
		module(disposer::make_data const& mdata):
			disposer::module_base(mdata, {slots.image}, {signals.data})
			{}


		using types = disposer::type_list<
			std::int8_t,
			std::uint8_t,
			pixel::rgb8,
			pixel::rgb8u
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
		auto to_jpg_image(bitmap< T > const& img){
			// JPEG encoding
			auto compressor_deleter = [](void* data){ tjDestroy(data); };
			std::unique_ptr< void, decltype(compressor_deleter) > compressor(
				tjInitCompress(),
				compressor_deleter
			);
			if(!compressor) throw std::runtime_error("tjInitCompress failed");

			// Image buffer
			std::uint8_t* data = nullptr;
			unsigned long size = 0;

			if(tjCompress2(
				compressor.get(),
				const_cast< std::uint8_t* >(
					reinterpret_cast< std::uint8_t const* >(img.data())),
				static_cast< int >(img.width()),
				static_cast< int >(img.width() * sizeof(T)),
				static_cast< int >(img.height()),
				sizeof(T) == 3 ? TJPF_RGB: TJPF_GRAY,
				&data,
				&size,
				sizeof(T) == 3 ? TJSAMP_420 : TJSAMP_GRAY,
				70,
				0
			) != 0) throw std::runtime_error("tjCompress2 failed");

			auto data_deleter = [](std::uint8_t* data){ tjFree(data); };
			std::unique_ptr< std::uint8_t, decltype(data_deleter) > data_ptr(
				data,
				data_deleter
			);

			// output
			return std::string(data_ptr.get(), data_ptr.get() + size);
		}


	}


	void module::exec(){
		for(auto const& pair: slots.image.get()){
			auto encoded_data = std::visit([](auto const& img){
					return to_jpg_image(img.data());
				}, pair.second);
			signals.data.put(std::move(encoded_data));
		}
	}


	void init(disposer::module_declarant& add){
		add("encode_jpg", &make_module);
	}

	BOOST_DLL_AUTO_ALIAS(init)


} }
