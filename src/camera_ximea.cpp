//-----------------------------------------------------------------------------
// Copyright (c) 2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <bitmap/subbitmap.hpp>

#include <disposer/component.hpp>

#include <boost/dll.hpp>

#include <m3api/xiApi.h>
#undef align


// == note to realtime priority ==
//
// sudo setcap cap_sys_nice+ep ./executbale
//

namespace disposer_module::camera_ximea{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;
	using namespace hana::literals;
	using hana::type_c;

	namespace pixel = ::bmp::pixel;
	using ::bmp::bitmap;


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


	template < typename Component >
	class ximea_cam_params{
	public:
		constexpr ximea_cam_params(
			Component const& component,
			HANDLE handle
		)noexcept
			: component_(component)
			, handle_(handle) {}

		void set(std::string const& name, int value)const;
		void set(std::string const& name, float value)const;
		void set(std::string const& name, std::string const& value)const;

		int get_int(std::string const& name)const;
		float get_float(std::string const& name)const;
		std::string get_string(std::string const& name)const;


	private:
		Component const component_;
		HANDLE handle_;
	};

	template < typename Component >
	void ximea_cam_params< Component >::set(
		std::string const& name,
		int value
	)const{
		component_.log([&](logsys::stdlogb& os){
			os << "set param: " << name << " = " << value;
		}, [&]{
			verify(xiSetParamInt(handle_, name.c_str(), value));
		});
	}

	template < typename Component >
	void ximea_cam_params< Component >::set(
		std::string const& name,
		float value
	)const{
		component_.log([&](logsys::stdlogb& os){
			os << "set param: " << name << " = " << value;
		}, [&]{
			verify(xiSetParamFloat(handle_, name.c_str(), value));
		});
	}

	template < typename Component >
	void ximea_cam_params< Component >::set(
		std::string const& name,
		std::string const& value
	)const{
		component_.log([&](logsys::stdlogb& os){
			os << "set param: " << name << " = " << value;
		}, [&]{
			verify(xiSetParamString(handle_, name.c_str(),
				const_cast< char* >(value.c_str()), value.size()));
		});
	}

	template < typename Component >
	int ximea_cam_params< Component >::get_int(
		std::string const& name
	)const{
		std::optional< int > value;
		component_.log([&](logsys::stdlogb& os){
			os << "get param: " << name;
			if(value) os << " = " << *value;
		}, [&]{
			int v;
			verify(xiGetParamInt(handle_, name.c_str(), &v));
			value = v;
		});
		return *value;
	}

	template < typename Component >
	float ximea_cam_params< Component >::get_float(
		std::string const& name
	)const{
		std::optional< float > value;
		component_.log([&](logsys::stdlogb& os){
			os << "get param: " << name;
			if(value) os << " = " << *value;
		}, [&]{
			float v = 0;
			verify(xiGetParamFloat(handle_, name.c_str(), &v));
			value = v;
		});
		return *value;
	}

	template < typename Component >
	std::string ximea_cam_params< Component >::get_string(
		std::string const& name
	)const{
		std::optional< std::string > value;
		component_.log([&](logsys::stdlogb& os){
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
		template < typename T, typename Component >
		bitmap< T > get_image(
			Component const& component,
			HANDLE handle
		)const;

		std::size_t const width = 0;
		std::size_t const height = 0;
		std::size_t const full_width = 0;
		std::size_t const full_height = 0;

		/// \\brief Image data buffer size in bytes
		std::size_t const payload_size = 0;
		bool const payload_pass = false;
	};

	template < typename T, typename Component >
	bitmap< T > ximea_cam::get_image(
		Component const& component,
		HANDLE handle
	)const{
		{
			ximea_cam_params< Component > params(component, handle);
			params.set(XI_PRM_TRG_SOFTWARE, 1);
		}

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

		if(component("use_camera_region"_param)) return mosaic;

		auto rect = ::bmp::rect(
			::bmp::point(
				component("x_offset"_param),
				component("y_offset"_param)
			),
			::bmp::size(
				component("width"_param),
				component("height"_param)
			)
		);

		return subbitmap(mosaic, rect);
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

	template < typename Component >
	ximea_cam make_ximea_cam(Component const& component, HANDLE handle){
		ximea_cam_params< Component > params(component, handle);
		params.set(XI_PRM_BUFFER_POLICY, XI_BP_SAFE);

		switch(component("format"_param)){
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

		auto const x = component("x_offset"_param);
		auto const y = component("y_offset"_param);
		auto const width = component("width"_param);
		auto const height = component("height"_param);
		if(x + width > cam_full_width || y + height > cam_full_height){
			std::ostringstream os;
			os << "parameter region is out of range (x: " << x << ", y: "
				<< y << ", width: " << width << ", height: " << height
				<< "; sensor resolution: "
				<< cam_full_width << "x" << cam_full_height << ")";
			throw std::logic_error(os.str());
		}

		if(component("use_camera_region"_param)){
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

		params.set(XI_PRM_TRG_SOURCE, XI_TRG_SOFTWARE);

		auto const payload_size = static_cast< std::size_t >(
			params.get_int(XI_PRM_IMAGE_PAYLOAD_SIZE));
		auto const payload_pass = (cam_width * cam_height == payload_size);

		params.set(XI_PRM_EXPOSURE,
			static_cast< int >(component("exposure_time_us"_param)));
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

	template < typename Component >
	class ximea_cam_init{
	public:
		ximea_cam_init(Component const& component)
			: component_(component)
			, handle_(open_ximea_cam(component("cam_id"_param)))
			, cam_(make_ximea_cam(component, handle_)) {}

		ximea_cam_init(ximea_cam_init const&) = delete;
		ximea_cam_init(ximea_cam_init&&) = delete;

		~ximea_cam_init(){
			if(handle_ == nullptr) return;

			component_.exception_catching_log(
				[](logsys::stdlogb& os){ os << "stop acquisition"; },
				[this]{ verify(xiStopAcquisition(handle_)); });

			component_.exception_catching_log(
				[](logsys::stdlogb& os){ os << "close camera"; },
				[this]{ verify(xiCloseDevice(handle_)); });
		}

		template < typename T >
		bitmap< T > get_image(){
			std::lock_guard lock(mutex);
			return cam_.get_image< T >(component_, handle_);
		}


	private:
		Component const component_;
		HANDLE handle_ = nullptr;
		ximea_cam cam_;
		std::mutex mutex;
	};


	void init(std::string const& name, component_declarant& declarant){
		auto init = generate_component(
			component_configure(
				make("format"_param, free_type_c< pixel_format >,
					parser_fn([](
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
					})),
				make("cam_id"_param, free_type_c< std::uint32_t >,
					default_value(0)),
				make("use_camera_region"_param, free_type_c< bool >,
					default_value(false)),
				make("x_offset"_param, free_type_c< std::size_t >,
					default_value(0)),
				make("y_offset"_param, free_type_c< std::size_t >,
					default_value(0)),
				make("width"_param, free_type_c< std::size_t >),
				make("height"_param, free_type_c< std::size_t >),
				make("exposure_time_us"_param, free_type_c< std::size_t >,
					default_value(10000))
			),
			component_init_fn([](auto const& component){
				return ximea_cam_init(component);
			}),
			component_modules(
				make("capture"_module, generate_fn([](auto& component){
					return generate_module(
						dimension_list{
							dimension_c<
								std::uint8_t,
								std::uint16_t,
								pixel::rgb8u
							>
						},
						module_configure(
							set_dimension_fn([&component]{
								auto const format = component("format"_param);
								switch(format){
									case pixel_format::mono8: [[fallthrough]];
									case pixel_format::raw8:
										return solved_dimensions{
											index_component< 0 >{0}};
									case pixel_format::mono16: [[fallthrough]];
									case pixel_format::raw16:
										return solved_dimensions{
											index_component< 0 >{1}};
									case pixel_format::rgb8:
										return solved_dimensions{
											index_component< 0 >{2}};
								}

								throw std::runtime_error("unknown format");
							}),
							make("image"_out, wrapped_type_ref_c< bitmap, 0 >)
						),
						exec_fn([&component](auto& module){
							module("image"_out).push(component.state()
								.template get_image< typename decltype(
										module.dimension(hana::size_c< 0 >)
									)::type >());
						}),
						no_overtaking
					);
				}))
			)
		);

		init(name, declarant);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
