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

#include <m3api/xiApi.h>
#undef align


// == note to  realtime priority ==
//
// sudo setcap cap_sys_nice+ep ./executbale
//

namespace disposer_module::camera_ximea{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;
	using namespace hana::literals;
	using hana::type_c;

	namespace pixel = ::bitmap::pixel;
	using ::bitmap::bitmap;


	constexpr auto types = hana::tuple_t<
			std::uint8_t,
			std::uint16_t,
			pixel::rgb8u
		>;


	void verify(XI_RETURN xi_return){
		// Conversion to enum XI_RET to get a warning if ximea extended the enum
		switch(XI_RET(xi_return)){
			case XI_OK:
				return;
			case XI_INVALID_HANDLE:
				throw std::runtime_error("Invalid handle");
			case XI_READREG:
				throw std::runtime_error("Register read error");
			case XI_WRITEREG:
				throw std::runtime_error("Register write error");
			case XI_FREE_RESOURCES:
				throw std::runtime_error("Freeing resources error");
			case XI_FREE_CHANNEL:
				throw std::runtime_error("Freeing channel error");
			case XI_FREE_BANDWIDTH:
				throw std::runtime_error("Freeing bandwith error");
			case XI_READBLK:
				throw std::runtime_error("Read block error");
			case XI_WRITEBLK:
				throw std::runtime_error("Write block error");
			case XI_NO_IMAGE:
				throw std::runtime_error("No image");
			case XI_TIMEOUT:
				throw std::runtime_error("Timeout");
			case XI_INVALID_ARG:
				throw std::runtime_error("Invalid arguments supplied");
			case XI_NOT_SUPPORTED:
				throw std::runtime_error("Not supported");
			case XI_ISOCH_ATTACH_BUFFERS:
				throw std::runtime_error("Attach buffers error");
			case XI_GET_OVERLAPPED_RESULT:
				throw std::runtime_error("Overlapped result");
			case XI_MEMORY_ALLOCATION:
				throw std::runtime_error("Memory allocation error");
			case XI_DLLCONTEXTISNULL:
				throw std::runtime_error("DLL context is NULL");
			case XI_DLLCONTEXTISNONZERO:
				throw std::runtime_error("DLL context is non zero");
			case XI_DLLCONTEXTEXIST:
				throw std::runtime_error("DLL context exists");
			case XI_TOOMANYDEVICES:
				throw std::runtime_error("Too many devices connected");
			case XI_ERRORCAMCONTEXT:
				throw std::runtime_error("Camera context error");
			case XI_UNKNOWN_HARDWARE:
				throw std::runtime_error("Unknown hardware");
			case XI_INVALID_TM_FILE:
				throw std::runtime_error("Invalid TM file");
			case XI_INVALID_TM_TAG:
				throw std::runtime_error("Invalid TM tag");
			case XI_INCOMPLETE_TM:
				throw std::runtime_error("Incomplete TM");
			case XI_BUS_RESET_FAILED:
				throw std::runtime_error("Bus reset error");
			case XI_NOT_IMPLEMENTED:
				throw std::runtime_error("Not implemented");
			case XI_SHADING_TOOBRIGHT:
				throw std::runtime_error("Shading is too bright");
			case XI_SHADING_TOODARK:
				throw std::runtime_error("Shading is too dark");
			case XI_TOO_LOW_GAIN:
				throw std::runtime_error("Gain is too low");
			case XI_INVALID_BPL:
				throw std::runtime_error("Invalid bad pixel list");
			case XI_BPL_REALLOC:
				throw std::runtime_error("Bad pixel list realloc error");
			case XI_INVALID_PIXEL_LIST:
				throw std::runtime_error("Invalid pixel list");
			case XI_INVALID_FFS:
				throw std::runtime_error("Invalid Flash File System");
			case XI_INVALID_PROFILE:
				throw std::runtime_error("Invalid profile");
			case XI_INVALID_CALIBRATION:
				throw std::runtime_error("Invalid calibration");
			case XI_INVALID_BUFFER:
				throw std::runtime_error("Invalid buffer");
			case XI_INVALID_DATA:
				throw std::runtime_error("Invalid data");
			case XI_TGBUSY:
				throw std::runtime_error("Timing generator is busy");
			case XI_IO_WRONG:
				throw std::runtime_error(
					"Wrong operation open/write/read/close");
			case XI_ACQUISITION_ALREADY_UP:
				throw std::runtime_error("Acquisition already started");
			case XI_OLD_DRIVER_VERSION:
				throw std::runtime_error(
					"Old version of device driver installed to the system.");
			case XI_GET_LAST_ERROR:
				throw std::runtime_error(
					"To get error code please call GetLastError function.");
			case XI_CANT_PROCESS:
				throw std::runtime_error("Data cannot be processed");
			case XI_ACQUISITION_STOPED:
				throw std::runtime_error(
					"Acquisition is stopped. It needs to be started to perform"
					" operation.");
			case XI_ACQUISITION_STOPED_WERR:
				throw std::runtime_error(
					"Acquisition has been stopped with an error.");
			case XI_INVALID_INPUT_ICC_PROFILE:
				throw std::runtime_error(
					"Input ICC profile missing or corrupted");
			case XI_INVALID_OUTPUT_ICC_PROFILE:
				throw std::runtime_error(
					"Output ICC profile missing or corrupted");
			case XI_DEVICE_NOT_READY:
				throw std::runtime_error("Device not ready to operate");
			case XI_SHADING_TOOCONTRAST:
				throw std::runtime_error("Shading is too contrast");
			case XI_ALREADY_INITIALIZED:
				throw std::runtime_error("Module already initialized");
			case XI_NOT_ENOUGH_PRIVILEGES:
				throw std::runtime_error(
					"Application does not have enough privileges (one or more"
					" app)");
			case XI_NOT_COMPATIBLE_DRIVER:
				throw std::runtime_error(
					"Installed driver is not compatible with current software");
			case XI_TM_INVALID_RESOURCE:
				throw std::runtime_error(
					"TM file was not loaded successfully from resources");
			case XI_DEVICE_HAS_BEEN_RESETED:
				throw std::runtime_error(
					"Device has been reset, abnormal initial state");
			case XI_NO_DEVICES_FOUND:
				throw std::runtime_error("No Devices Found");
			case XI_RESOURCE_OR_FUNCTION_LOCKED:
				throw std::runtime_error(
					"Resource (device) or function locked by mutex");
			case XI_BUFFER_SIZE_TOO_SMALL:
				throw std::runtime_error(
					"Buffer provided by user is too small");
			case XI_COULDNT_INIT_PROCESSOR:
				throw std::runtime_error("Couldnt initialize processor.");
			case XI_NOT_INITIALIZED:
				throw std::runtime_error(
					"The object/module/procedure/process being referred to has"
					" not been started.");
			case XI_RESOURCE_NOT_FOUND:
				throw std::runtime_error(
					"Resource not found(could be processor, file, item...).");
			case XI_UNKNOWN_PARAM:
				throw std::runtime_error("Unknown parameter");
			case XI_WRONG_PARAM_VALUE:
				throw std::runtime_error("Wrong parameter value");
			case XI_WRONG_PARAM_TYPE:
				throw std::runtime_error("Wrong parameter type");
			case XI_WRONG_PARAM_SIZE:
				throw std::runtime_error("Wrong parameter size");
			case XI_BUFFER_TOO_SMALL:
				throw std::runtime_error("Input buffer is too small");
			case XI_NOT_SUPPORTED_PARAM:
				throw std::runtime_error("Parameter is not supported");
			case XI_NOT_SUPPORTED_PARAM_INFO:
				throw std::runtime_error("Parameter info not supported");
			case XI_NOT_SUPPORTED_DATA_FORMAT:
				throw std::runtime_error("Data format is not supported");
			case XI_READ_ONLY_PARAM:
				throw std::runtime_error("Read only parameter");
			case XI_BANDWIDTH_NOT_SUPPORTED:
				throw std::runtime_error(
					"This camera does not support currently available"
					" bandwidth");
			case XI_INVALID_FFS_FILE_NAME:
				throw std::runtime_error(
					"FFS file selector is invalid or NULL");
			case XI_FFS_FILE_NOT_FOUND:
				throw std::runtime_error("FFS file not found");
			case XI_PARAM_NOT_SETTABLE:
				throw std::runtime_error(
					"Parameter value cannot be set (might be out of range or"
					" invalid).");
			case XI_SAFE_POLICY_NOT_SUPPORTED:
				throw std::runtime_error(
					"Safe buffer policy is not supported. E.g. when transport"
					" target is set to GPU (GPUDirect).");
			case XI_GPUDIRECT_NOT_AVAILABLE:
				throw std::runtime_error(
					"GPUDirect is not available. E.g. platform isn't supported"
					" or CUDA toolkit isn't installed.");
			case XI_PROC_OTHER_ERROR:
				throw std::runtime_error("Processing error - other");
			case XI_PROC_PROCESSING_ERROR:
				throw std::runtime_error("Error while image processing.");
			case XI_PROC_INPUT_FORMAT_UNSUPPORTED:
				throw std::runtime_error(
					"Input format is not supported for processing.");
			case XI_PROC_OUTPUT_FORMAT_UNSUPPORTED:
				throw std::runtime_error(
					"Output format is not supported for processing.");
		}
		throw std::logic_error("unknown XI_RETURN");
	}


	template < typename Module >
	class ximea_cam_params{
	public:
		constexpr ximea_cam_params(Module const& module, HANDLE handle)noexcept
			: module_(module)
			, handle_(handle) {}

		void set(std::string const& name, int value)const;
		void set(std::string const& name, float value)const;
		void set(std::string const& name, std::string const& value)const;

		int get_int(std::string const& name)const;
		float get_float(std::string const& name)const;
		std::string get_string(std::string const& name)const;


	private:
		Module const& module_;
		HANDLE handle_;
	};

	template < typename Module >
	void ximea_cam_params< Module >::set(
		std::string const& name,
		int value
	)const{
		module_.log([&](logsys::stdlogb& os){
			os << "set param: " << name << " = " << value;
		}, [&]{
			verify(xiSetParamInt(handle_, name.c_str(), value));
		});
	}

	template < typename Module >
	void ximea_cam_params< Module >::set(
		std::string const& name,
		float value
	)const{
		module_.log([&](logsys::stdlogb& os){
			os << "set param: " << name << " = " << value;
		}, [&]{
			verify(xiSetParamFloat(handle_, name.c_str(), value));
		});
	}

	template < typename Module >
	void ximea_cam_params< Module >::set(
		std::string const& name,
		std::string const& value
	)const{
		module_.log([&](logsys::stdlogb& os){
			os << "set param: " << name << " = " << value;
		}, [&]{
			verify(xiSetParamString(handle_, name.c_str(),
				const_cast< char* >(value.c_str()), value.size()));
		});
	}

	template < typename Module >
	int ximea_cam_params< Module >::get_int(
		std::string const& name
	)const{
		std::optional< int > value;
		module_.log([&](logsys::stdlogb& os){
			os << "get param: " << name;
			if(value) os << " = " << *value;
		}, [&]{
			int v;
			verify(xiGetParamInt(handle_, name.c_str(), &v));
			value = v;
		});
		return *value;
	}

	template < typename Module >
	float ximea_cam_params< Module >::get_float(
		std::string const& name
	)const{
		std::optional< float > value;
		module_.log([&](logsys::stdlogb& os){
			os << "get param: " << name;
			if(value) os << " = " << *value;
		}, [&]{
			float v = 0;
			verify(xiGetParamFloat(handle_, name.c_str(), &v));
			value = v;
		});
		return *value;
	}

	template < typename Module >
	std::string ximea_cam_params< Module >::get_string(
		std::string const& name
	)const{
		std::optional< std::string > value;
		module_.log([&](logsys::stdlogb& os){
			os << "get param: " << name;
			if(value) os << " = " << *value;
		}, [&]{
			char data[4096];
			data[sizeof(data) - 1] = '\0';
			verify(xiGetParamString(
				handle_, name.c_str(), data, sizeof(data) - 1));
			char const* ptr = static_cast< char* >(&data[0]);
			value = std::string(ptr, ptr + strlen(ptr));
		});
		return *value;
	}


	struct ximea_cam{
		template < typename T, typename Module >
		bitmap< T > get_image(Module const& module, HANDLE handle)const;

		std::size_t const width = 0;
		std::size_t const height = 0;
		std::size_t const full_width = 0;
		std::size_t const full_height = 0;

		/// \\brief Image data buffer size in bytes
		std::size_t const payload_size = 0;
		bool const payload_pass = false;
	};

	template < typename T, typename Module >
	bitmap< T > ximea_cam::get_image(Module const& module, HANDLE handle)const{
		bitmap< T > mosaic(width, height);
		if(payload_pass){
			XI_IMG image{}; // {} makes the 0-initialization
			image.size = sizeof(XI_IMG);
			image.bp = static_cast< void* >(mosaic.data());
			image.bp_size = mosaic.point_count() * sizeof(T);

			verify(xiGetImage(handle, 2000, &image));

			assert(image.padding_x == 0);
		}else{
			std::vector< std::uint8_t > buffer(payload_size);

			XI_IMG image{}; // {} makes the 0-initialization
			image.size = sizeof(XI_IMG);
			image.bp = static_cast< void* >(buffer.data());
			image.bp_size = payload_size;

			verify(xiGetImage(handle, 2000, &image));

			assert(image.padding_x > 0);

			auto line_width = mosaic.width() * sizeof(T) + image.padding_x;
			for(std::size_t y = 0; y < mosaic.height(); ++y){
				auto line_start = buffer.data() + y * line_width;
				auto line_end = line_start + mosaic.width() * sizeof(T);
				auto out_line_start = reinterpret_cast< std::uint8_t* >(
					mosaic.data() + y * mosaic.width()
				);
				std::copy(line_start, line_end, out_line_start);
			}
		}

		if(module("use_camera_region"_param).get()) return mosaic;

		auto rect = ::bitmap::make_rect(
			::bitmap::make_point(
				module("x_offset"_param).get(),
				module("y_offset"_param).get()
			),
			::bitmap::make_size(
				module("width"_param).get(),
				module("height"_param).get()
			)
		);

		return mosaic.subbitmap(rect);
	}


	enum class pixel_format{
		mono8,
		mono16,
		rgb8,
		raw8,
		raw16
	};


	HANDLE open_ximea_cam(std::uint32_t cam_id){
		HANDLE handle = nullptr;
		verify(xiOpenDevice(cam_id, &handle));
		return handle;
	}

	template < typename Module >
	ximea_cam make_ximea_cam(Module const& module, HANDLE handle){
		ximea_cam_params< Module > params(module, handle);
		params.set(XI_PRM_BUFFER_POLICY, XI_BP_SAFE);

		switch(module("format"_param).get()){
			case pixel_format::mono8:
				params.set(XI_PRM_IMAGE_DATA_FORMAT, XI_MONO8);
				break;
			case pixel_format::raw8:
				params.set(XI_PRM_IMAGE_DATA_FORMAT, XI_RAW8);
				break;
			case pixel_format::mono16:
				params.set(XI_PRM_IMAGE_DATA_FORMAT, XI_MONO16);
				break;
			case pixel_format::raw16:
				params.set(XI_PRM_IMAGE_DATA_FORMAT, XI_RAW16);
				break;
			case pixel_format::rgb8:
				params.set(XI_PRM_IMAGE_DATA_FORMAT, XI_RGB24);
				break;
		}

		auto const cam_full_width =
			static_cast< std::size_t >(params.get_int(XI_PRM_WIDTH));
		auto const cam_full_height =
			static_cast< std::size_t >(params.get_int(XI_PRM_HEIGHT));
		auto cam_width = cam_full_width;
		auto cam_height = cam_full_height;

		auto const x = module("x_offset"_param).get();
		auto const y = module("y_offset"_param).get();
		auto const width = module("width"_param).get();
		auto const height = module("height"_param).get();
		if(x + width > cam_full_width || y + height > cam_full_height){
			std::ostringstream os;
			os << "parameter region is out of range (x: " << x << ", y: "
				<< y << ", width: " << width << ", height: " << height
				<< "; sensor resolution: "
				<< cam_full_width << "x" << cam_full_height << ")";
			throw std::logic_error(os.str());
		}

		if(module("use_camera_region"_param).get()){
			params.set(XI_PRM_REGION_SELECTOR, 0);
			params.set(XI_PRM_HEIGHT, static_cast< int >(height));
			params.set(XI_PRM_WIDTH, static_cast< int >(width));
			params.set(XI_PRM_OFFSET_X, static_cast< int >(x));
			params.set(XI_PRM_OFFSET_Y, static_cast< int >(y));
			// // Don't enable reagion 0?
			// params.set(XI_PRM_REGION_MODE, 1);
			cam_width = width;
			cam_height = height;
		}

		auto const payload_size = static_cast< std::size_t >(
			params.get_int(XI_PRM_IMAGE_PAYLOAD_SIZE));
		auto const payload_pass = (width * height == payload_size);

		params.set(XI_PRM_EXPOSURE,
			static_cast< int >(module("exposure_time_ns"_param).get()));
		verify(xiStartAcquisition(handle));

		return ximea_cam{
			cam_width,
			cam_height,
			cam_full_width,
			cam_full_height,
			payload_size,
			payload_pass
		};
	}

	template < typename Module >
	class ximea_cam_init{
	public:
		ximea_cam_init(Module const& module)
			: module_(module)
			, handle_(open_ximea_cam(module("cam_id"_param).get()))
			, cam_(make_ximea_cam(module, handle_)) {}

		ximea_cam_init(ximea_cam_init const&) = delete;

		ximea_cam_init(ximea_cam_init&& other):
			module_(other.module_),
			handle_(other.handle_),
			cam_(std::move(other.cam_))
		{
			other.handle_ = nullptr;
		}

		~ximea_cam_init(){
			if(handle_ == nullptr) return;

			try{
				verify(xiStopAcquisition(handle_));
				verify(xiCloseDevice(handle_));
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
				case pixel_format::raw8:
					module("image"_out).put(get_image< std::uint8_t >());
					break;
				case pixel_format::mono16:
				case pixel_format::raw16:
					module("image"_out).put(get_image< std::uint16_t >());
					break;
				case pixel_format::rgb8:
					module("image"_out).put(get_image< pixel::rgb8u >());
					break;
			}
		}


	private:
		template < typename T >
		bitmap< T > get_image()const{
			return cam_.get_image< T >(
				static_cast< to_exec_accessory_t< Module > const& >(module_),
				handle_
			);
		}

		Module const& module_;
		HANDLE handle_ = nullptr;
		ximea_cam cam_;
	};


	void init(std::string const& name, module_declarant& disposer){
		auto init = make_register_fn(
			configure(
				"format"_param(type_c< pixel_format >,
					parser([](
						auto const& /*iop*/,
						std::string_view data,
						hana::basic_type< pixel_format >
					){
						if(data == "mono8") return pixel_format::mono8;
						if(data == "mono16") return pixel_format::mono16;
						if(data == "rgb8") return pixel_format::rgb8;
						if(data == "raw8") return pixel_format::raw8;
						if(data == "raw16") return pixel_format::raw16;
						throw std::runtime_error("unknown value '"
							+ std::string(data) + "', allowed values are: "
							"mono8, mono16, raw8, raw16 & rgb8");

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
							(type == type_c< std::uint8_t > && (
								format == pixel_format::mono8 ||
								format == pixel_format::raw8)) ||
							(type == type_c< std::uint16_t > && (
								format == pixel_format::mono16 ||
								format == pixel_format::raw16)) ||
							(type == type_c< pixel::rgb8u > && (
								format == pixel_format::rgb8));
					})
				),
				"cam_id"_param(type_c< std::uint32_t >,
					default_values(std::uint32_t(0))
				),
				"use_camera_region"_param(type_c< bool >,
					default_values(false)
				),
				"x_offset"_param(type_c< std::size_t >,
					default_values(std::size_t(0))
				),
				"y_offset"_param(type_c< std::size_t >,
					default_values(std::size_t(0))
				),
				"width"_param(type_c< std::size_t >),
				"height"_param(type_c< std::size_t >),
				"exposure_time_ns"_param(type_c< std::size_t >,
					default_values(std::size_t(10000))
				)
			),
			normal_id_increase(),
			[](auto const& module){
				return ximea_cam_init< std::decay_t< decltype(module) > >(
					module
				);
			}
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
