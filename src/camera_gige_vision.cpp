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

#include <arv.h>

#include <thread>


namespace disposer_module::camera_gige_vision{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;
	using namespace hana::literals;
	using hana::type_c;

	namespace pixel = ::bitmap::pixel;
	using ::bitmap::bitmap;


	constexpr auto types = hana::tuple_t<
			std::uint8_t,
			std::uint16_t
		>;


	constexpr std::string_view avr_format_to_text(ArvPixelFormat value){
		switch(value){
			case ARV_PIXEL_FORMAT_BAYER_BG_10:
				return "BAYER_BG_10";
			case ARV_PIXEL_FORMAT_BAYER_BG_12:
				return "BAYER_BG_12";
			case ARV_PIXEL_FORMAT_BAYER_GR_12_PACKED:
				return "GR_12_PACKED";
			case ARV_PIXEL_FORMAT_BAYER_GB_12_PACKED:
				return "GB_12_PACKED";
			case ARV_PIXEL_FORMAT_BAYER_RG_12_PACKED:
				return "RG_12_PACKED";
			case ARV_PIXEL_FORMAT_BAYER_BG_12_PACKED:
				return "BG_12_PACKED";
			case ARV_PIXEL_FORMAT_BAYER_BG_8:
				return "BAYER_BG_8";
			case ARV_PIXEL_FORMAT_BAYER_GB_10:
				return "BAYER_GB_10";
			case ARV_PIXEL_FORMAT_BAYER_GB_12:
				return "BAYER_GB_12";
			case ARV_PIXEL_FORMAT_BAYER_GB_8:
				return "BAYER_GB_8";
			case ARV_PIXEL_FORMAT_BAYER_GR_10:
				return "BAYER_GR_10";
			case ARV_PIXEL_FORMAT_BAYER_GR_12:
				return "BAYER_GR_12";
			case ARV_PIXEL_FORMAT_BAYER_GR_8:
				return "BAYER_GR_8";
			case ARV_PIXEL_FORMAT_BAYER_RG_10:
				return "BAYER_RG_10";
			case ARV_PIXEL_FORMAT_BAYER_RG_12:
				return "BAYER_RG_12";
			case ARV_PIXEL_FORMAT_BAYER_RG_8:
				return "BAYER_RG_8";
			case ARV_PIXEL_FORMAT_BAYER_BG_16:
				return "BAYER_BG_16";
			case ARV_PIXEL_FORMAT_BAYER_GB_16:
				return "BAYER_GB_16";
			case ARV_PIXEL_FORMAT_BAYER_GR_16:
				return "BAYER_GR_16";
			case ARV_PIXEL_FORMAT_BAYER_RG_16:
				return "BAYER_RG_16";
			case ARV_PIXEL_FORMAT_BGRA_8_PACKED:
				return "BGRA_8_PACKED";
			case ARV_PIXEL_FORMAT_BGR_10_PACKED:
				return "BGR_10_PACKED";
			case ARV_PIXEL_FORMAT_BGR_12_PACKED:
				return "BGR_12_PACKED";
			case ARV_PIXEL_FORMAT_BGR_8_PACKED:
				return "BGR_8_PACKED";
			case ARV_PIXEL_FORMAT_CUSTOM_BAYER_BG_12_PACKED:
				return "CUSTOM_BAYER_BG_12_PACKED";
			case ARV_PIXEL_FORMAT_CUSTOM_BAYER_BG_16:
				return "CUSTOM_BAYER_BG_16";
			case ARV_PIXEL_FORMAT_CUSTOM_BAYER_GB_12_PACKED:
				return "CUSTOM_BAYER_GB_12_PACKED";
			case ARV_PIXEL_FORMAT_CUSTOM_BAYER_GB_16:
				return "CUSTOM_BAYER_GB_16";
			case ARV_PIXEL_FORMAT_CUSTOM_BAYER_GR_12_PACKED:
				return "CUSTOM_BAYER_GR_12_PACKED";
			case ARV_PIXEL_FORMAT_CUSTOM_BAYER_GR_16:
				return "CUSTOM_BAYER_GR_16";
			case ARV_PIXEL_FORMAT_CUSTOM_BAYER_RG_12_PACKED:
				return "CUSTOM_BAYER_RG_12_PACKED";
			case ARV_PIXEL_FORMAT_CUSTOM_BAYER_RG_16:
				return "CUSTOM_BAYER_RG_16";
			case ARV_PIXEL_FORMAT_CUSTOM_YUV_422_YUYV_PACKED:
				return "CUSTOM_YUV_422_YUYV_PACKED";
			case ARV_PIXEL_FORMAT_MONO_10:
				return "MONO_10";
			case ARV_PIXEL_FORMAT_MONO_10_PACKED:
				return "MONO_10_PACKED";
			case ARV_PIXEL_FORMAT_MONO_12:
				return "MONO_12";
			case ARV_PIXEL_FORMAT_MONO_12_PACKED:
				return "MONO_12_PACKED";
			case ARV_PIXEL_FORMAT_MONO_14:
				return "MONO_14";
			case ARV_PIXEL_FORMAT_MONO_16:
				return "MONO_16";
			case ARV_PIXEL_FORMAT_MONO_8:
				return "MONO_8";
			case ARV_PIXEL_FORMAT_MONO_8_SIGNED:
				return "MONO_8_SIGNED";
			case ARV_PIXEL_FORMAT_RGBA_8_PACKED:
				return "RGBA_8_PACKED";
			case ARV_PIXEL_FORMAT_RGB_10_PACKED:
				return "RGB_10_PACKED";
			case ARV_PIXEL_FORMAT_RGB_10_PLANAR:
				return "RGB_10_PLANAR";
			case ARV_PIXEL_FORMAT_RGB_12_PACKED:
				return "RGB_12_PACKED";
			case ARV_PIXEL_FORMAT_RGB_12_PLANAR:
				return "RGB_12_PLANAR";
			case ARV_PIXEL_FORMAT_RGB_16_PLANAR:
				return "RGB_16_PLANAR";
			case ARV_PIXEL_FORMAT_RGB_8_PACKED:
				return "RGB_8_PACKED";
			case ARV_PIXEL_FORMAT_RGB_8_PLANAR:
				return "RGB_8_PLANAR";
			case ARV_PIXEL_FORMAT_YUV_411_PACKED:
				return "YUV_411_PACKED";
			case ARV_PIXEL_FORMAT_YUV_422_PACKED:
				return "YUV_422_PACKED";
			case ARV_PIXEL_FORMAT_YUV_422_YUYV_PACKED:
				return "YUV_422_YUYV_PACKED";
			case ARV_PIXEL_FORMAT_YUV_444_PACKED:
				return "YUV_444_PACKED";
		}
		throw std::runtime_error("unknown avr ArvPixelFormat "
			+ std::to_string(value));
	}


	struct gige_vision_cam{
		gige_vision_cam(GMainLoop* main_loop, gint payload, ArvStream* stream)
			: last_buffer(nullptr)
			, main_loop(main_loop)
			, payload(payload)
			, stream(stream) {}

		gige_vision_cam(gige_vision_cam&& other)
			: last_buffer(nullptr)
			, main_loop(other.main_loop)
			, payload(other.payload)
			, stream(other.stream) {}

		template < typename T >
		bitmap< T > get_image();

		std::condition_variable cv;
		std::mutex mutex;
		ArvBuffer* last_buffer;

		GMainLoop* main_loop;

		unsigned const payload;
		ArvStream* const stream;
	};

	template < typename T >
	bitmap< T > gige_vision_cam::get_image(){
		ArvBuffer* buffer = nullptr;

		{
			std::unique_lock< std::mutex > lock(mutex);
			using namespace std::literals::chrono_literals;
			cv.wait_for(lock, 2s);
			buffer = last_buffer;
			last_buffer = nullptr;
		}

		if(!(ARV_IS_BUFFER(buffer))){
			throw std::runtime_error("ARV_IS_BUFFER failed");
		}

		if(arv_buffer_get_status(buffer) != ARV_BUFFER_STATUS_SUCCESS){
			throw std::runtime_error(
				"arv_buffer_get_status is not ARV_BUFFER_STATUS_SUCCESS");
		}

		auto format = arv_buffer_get_image_pixel_format(buffer);
		if(!(
			(std::is_same_v< T, std::uint8_t >
				&& format == ARV_PIXEL_FORMAT_MONO_8) ||
			(std::is_same_v< T, std::uint16_t >
				&& format == ARV_PIXEL_FORMAT_MONO_16)
		)){
			throw std::runtime_error("wrong image format: "
				 + std::string(avr_format_to_text(format)));
		}

		auto const width =
			static_cast< std::size_t >(arv_buffer_get_image_width(buffer));
		auto const height =
			static_cast< std::size_t >(arv_buffer_get_image_height(buffer));

		std::size_t size = 0;
		auto data = reinterpret_cast< T const* >(
			arv_buffer_get_data(buffer, &size));

		if(size != width * height * sizeof(T)){
			throw std::runtime_error("buffer size is "
				 + std::to_string(size) + ", but expected "
				 + std::to_string(width) + "×" + std::to_string(height) + "×"
				 + std::to_string(sizeof(T)));
		}

		bitmap< T > result(width, height);
		for(std::size_t y = 0; y < height; ++y){
			for(std::size_t x = 0; x < width; ++x){
				result(x, y) = *data++;
			}
		}

		return result;
	}


	enum class pixel_format{
		mono8,
		mono16
	};


	ArvCamera* open_gige_vision_cam(std::optional< std::string > const& name){
		ArvCamera* handle = arv_camera_new(name ? name->c_str() : nullptr);
		if(handle != nullptr) return handle;
		if(name){
			throw std::runtime_error("camera '" + *name + "' not found");
		}else{
			throw std::runtime_error("no camera found");
		}
	}

	auto update_image(ArvStream* stream, gige_vision_cam* data){
		auto buffer = arv_stream_try_pop_buffer(stream);

		std::lock_guard< std::mutex > lock(data->mutex);
		data->last_buffer = buffer;
		data->cv.notify_all();
	}

	template < typename Module >
	gige_vision_cam make_gige_vision_cam(
		Module const& /*module*/,
		ArvCamera* handle
	){
		arv_camera_start_acquisition(handle);

		auto payload = arv_camera_get_payload(handle);
		auto stream = arv_camera_create_stream(handle, nullptr, nullptr);
		for(std::size_t i = 0; i < 50; ++i){
			arv_stream_push_buffer(stream, arv_buffer_new(payload, nullptr));
		}

		auto main_loop = g_main_loop_new(nullptr, FALSE);

		return gige_vision_cam(main_loop, payload, stream);
	}

	template < typename Module >
	class gige_vision_cam_init{
	public:
		gige_vision_cam_init(Module const& module)
			: module_(module)
			, handle_(open_gige_vision_cam(module("cam_name"_param).get()))
			, cam_(make_gige_vision_cam(module, handle_)) {
				g_signal_connect(cam_.stream, "new-buffer",
					G_CALLBACK(update_image), &cam_);
				g_main_loop_.emplace([this]{
					g_main_loop_run(cam_.main_loop);
				});
			}

		gige_vision_cam_init(gige_vision_cam_init const&) = delete;

		gige_vision_cam_init(gige_vision_cam_init&& other):
			module_(other.module_),
			handle_(other.handle_),
			cam_(std::move(other.cam_))
		{
			other.handle_ = nullptr;
		}

		~gige_vision_cam_init(){
			if(handle_ == nullptr) return;

			try{
				g_main_loop_unref(cam_.main_loop);
				g_main_loop_->join();
				arv_camera_stop_acquisition(handle_);
				g_object_unref(cam_.stream);
				g_clear_object(&handle_);
			}catch(std::exception const& e){
				module_.log([&e](logsys::stdlogb& os){
					os << e.what();
				});
			}catch(...){
				module_.log([](logsys::stdlogb& os){
					os << "unknown exception";
				});
			}
		}

		void operator()(to_exec_accessory_t< Module >& module, std::size_t){
			switch(module("format"_param).get()){
				case pixel_format::mono8:
					module("image"_out).put(get_image< std::uint8_t >());
					break;
				case pixel_format::mono16:
					module("image"_out).put(get_image< std::uint16_t >());
					break;
			}
		}


	private:
		template < typename T >
		bitmap< T > get_image(){
			return cam_.get_image< T >();
		}

		Module const& module_;
		ArvCamera* handle_ = nullptr;
		gige_vision_cam cam_;
		std::optional< std::thread > g_main_loop_;
	};


	void init(std::string const& name, module_declarant& disposer){
		auto init = make_module_register_fn(
			module_configure(
				"format"_param(type_c< pixel_format >,
					parser([](
						auto const& /*iop*/,
						std::string_view data,
						hana::basic_type< pixel_format >
					){
						if(data == "mono8") return pixel_format::mono8;
						if(data == "mono16") return pixel_format::mono16;
						throw std::runtime_error("unknown value '"
							+ std::string(data) + "', allowed values are: "
							"mono8, mono16");

					}),
					type_as_text(
						hana::make_pair(type_c< pixel_format >, "format"_s)
					)
				),
				"image"_out(types,
					template_transform_c< bitmap >,
					enable([](auto const& iop, auto type){
						auto const format = iop("format"_param).get();
						return
							(type == type_c< std::uint8_t > &&
								format == pixel_format::mono8) ||
							(type == type_c< std::uint16_t > &&
								format == pixel_format::mono16);
					})
				),
				"cam_name"_param(type_c< std::optional< std::string > >),
				"exposure_time_ns"_param(type_c< std::size_t >,
					default_values(std::size_t(1000))
				)
			),
			normal_id_increase(),
			[](auto const& module){
				return gige_vision_cam_init< std::decay_t< decltype(module) > >(
					module
				);
			}
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
