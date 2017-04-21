//-----------------------------------------------------------------------------
// Copyright (c) 2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <disposer/module.hpp>

#include <logsys/stdlogb.hpp>

#include <bitmap/pixel.hpp>

#include <raspicam/raspicam.h>

#include <boost/dll.hpp>

#include <ctime>
#include <sstream>

#include "bitmap.hpp"


namespace disposer_module{ namespace camera_raspicam{


	namespace pixel = ::bitmap::pixel;


	enum class pixel_format{
		gray,
		rgb,
	};


	struct parameter{
		pixel_format format;
	};


	using type_list = disposer::type_list<
		std::uint8_t,
		pixel::rgb8
	>;


	struct module: disposer::module_base{
		module(disposer::make_data const& data, parameter&& param):
			disposer::module_base(data, {image}),
			param(std::move(param))
			{}


		disposer::container_output< bitmap, type_list > image{"image"};


		void enable()override{
			log([](logsys::stdlogb& os){
					os << "Connect to camera";
				}, [this]{
					if(!cam_.open(true)){
						throw std::runtime_error("Can not connect to raspicam");
					}
					auto format = raspicam::RASPICAM_FORMAT_IGNORE;
					switch(param.format){
					case pixel_format::gray:
						format = raspicam::RASPICAM_FORMAT_GRAY;
					break;
					case pixel_format::rgb:
						format = raspicam::RASPICAM_FORMAT_RGB;
					break;
					}
					cam_.setFormat(format);
				});
		}

		void disable()noexcept override{
			exception_catching_log([](logsys::stdlogb& os){
					os << "Disconnect camera";
				}, [this]{
					cam_.release();
				});
		}

		void exec()override;
		void input_ready()override;

		template < typename T >
		bitmap< T > get_image();

		raspicam::RaspiCam cam_;

		parameter const param;
	};


	template < typename T >
	bitmap< T > module::get_image(){
		return log([](logsys::stdlogb& os){ os << "capture image"; },
			[this]{
				cam_.grab();

				bitmap< T > image(cam_.getWidth(), cam_.getHeight());
				auto data = reinterpret_cast< T* >(cam_.getImageBufferData());
				std::copy(data, data + image.point_count(), image.data());
				return image;
			});
	}


	disposer::module_ptr make_module(disposer::make_data& data){
		if(data.outputs.find("image") == data.outputs.end()){
			throw std::logic_error("no input defined (use 'image')");
		}

		std::string format;
		data.params.set(format, "pixel_format", "gray");

		std::map< std::string, pixel_format > const map{
			{"gray", pixel_format::gray},
			{"rgb", pixel_format::rgb}
		};

		auto iter = map.find(format);
		if(iter == map.end()){
			std::ostringstream os;
			os << "parameter pixel_format has unknown value '" << format
				<< "', available formats are: ";
			bool first = true;
			for(auto& pair: map){
				if(first){ first = false; }else{ os << ", "; }
				os << "'" << pair.first << "'";
			}
			throw std::logic_error(os.str());
		}

		parameter param{};
		param.format = iter->second;

		return std::make_unique< module >(data, std::move(param));
	}


	void module::exec(){
		switch(param.format){
			case pixel_format::gray:
				image.put< std::uint8_t >(get_image< std::uint8_t >());
				break;
			case pixel_format::rgb:
				image.put< pixel::rgb8 >(get_image< pixel::rgb8 >());
				break;
		}
	}

	void module::input_ready(){
		switch(param.format){
			case pixel_format::gray:
				image.enable< std::uint8_t >();
				break;
			case pixel_format::rgb:
				image.enable< pixel::rgb8 >();
				break;
		}
	}


	void init(disposer::module_declarant& add){
		add("camera_raspicam", &make_module);
	}

	BOOST_DLL_AUTO_ALIAS(init)


} }
