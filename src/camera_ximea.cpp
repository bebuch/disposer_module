//-----------------------------------------------------------------------------
// Copyright (c) 2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "log.hpp"

#include <disposer/module.hpp>

#include <bitmap/pixel.hpp>

#include <m3api/xiApi.h>
#undef align

#include <boost/dll.hpp>

#include <cstring>

#include "bitmap.hpp"


namespace disposer_module{ namespace camera_ximea{


	namespace pixel = ::bitmap::pixel;


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


	struct module;
	class ximea_cam{
	public:
		ximea_cam(module const& module, std::uint32_t id): ximea_cam(module){
			verify(xiOpenDevice(id, &handle_));
			init();
		}

		ximea_cam(
			module const& module, XI_OPEN_BY open_by, std::string const& str
		):
			ximea_cam(module)
		{
			verify(xiOpenDeviceBy(open_by, str.c_str(), &handle_));
			init();
		}

		~ximea_cam();

		void start_acquisition()const{
			verify(xiStartAcquisition(handle_));
		}

		void stop_acquisition()const{
			verify(xiStopAcquisition(handle_));
		}

		template < typename T >
		bitmap< T > get_image()const{
			XI_IMG image{}; // {} makes the 0-initialization
			image.size = sizeof(XI_IMG);
			verify(xiGetImage(handle_, 2000, &image));

			auto const data = reinterpret_cast< T const* >(image.bp);

			bitmap< T > mosaic(image.width, image.height);
			for(std::size_t y = 0; y < mosaic.height(); ++y){
				for(std::size_t x = 0; x < mosaic.width(); ++x){
					mosaic(x, y) = data[y * image.width + x];
				}
			}

			return mosaic;
		}

		void set_param(std::string const& name, int value)const{
			verify(xiSetParamInt(handle_, name.c_str(), value));
		}

		void set_param(std::string const& name, float value)const{
			verify(xiSetParamFloat(handle_, name.c_str(), value));
		}

		void set_param(std::string const& name, std::string const& value)const{
			verify(xiSetParamString(handle_, name.c_str(),
				const_cast< char* >(value.c_str()), value.size()));
		}

		int get_param_int(std::string const& name)const{
			int value = 0;
			verify(xiGetParamInt(handle_, name.c_str(), &value));
			return value;
		}

		float get_param_float(std::string const& name)const{
			float value = 0;
			verify(xiGetParamFloat(handle_, name.c_str(), &value));
			return value;
		}

		std::string get_param_string(std::string const& name)const{
			char data[4096];
			data[sizeof(data) - 1] = '\0';
			verify(xiGetParamString(
				handle_, name.c_str(), data, sizeof(data) - 1));
			char const* ptr = static_cast< char* >(&data[0]);
			return std::string(ptr, ptr + strlen(ptr));
		}

	private:
		ximea_cam(module const& module): module_(module), handle_(0){}

		void init()const;

		module const& module_;
		HANDLE handle_;
	};


	enum class pixel_format{
		mono8,
		mono16,
		rgb8,
		raw8,
		raw16
	};


	struct parameter{
		bool list_cameras;

		std::size_t id;

		pixel_format format;
	};


	using type_list = disposer::type_list<
		std::uint8_t,
		std::uint16_t,
		pixel::rgb8
	>;


	struct module: disposer::module_base{
		module(disposer::make_data const& data, parameter&& param):
			disposer::module_base(data, {image, info}),
			param(std::move(param))
		{
			if(param.list_cameras) return;
			cam_.emplace(*this, 0);
			cam_->set_param(XI_PRM_EXPOSURE, 10000);
			cam_->start_acquisition();
		}

		~module(){
			if(param.list_cameras) return;
			cam_->stop_acquisition();
		}


		disposer::container_output< bitmap, type_list > image{"image"};

		disposer::output< std::string > info{"info"};


		void exec()override;
		void input_ready()override;


		std::string exec_id_list()const;


		boost::optional< ximea_cam > cam_;

		parameter const param;
	};

	ximea_cam::~ximea_cam(){
		try{
			verify(xiCloseDevice(handle_));
		}catch(std::exception const& e){
			module_.log([&e](log::info& os){ os << e.what(); });
		}catch(...){
			module_.log([](log::info& os){ os << "unknown exception"; });
		}
	}

	void ximea_cam::init()const{
		set_param(XI_PRM_BUFFER_POLICY, XI_BP_SAFE);

		switch(module_.param.format){
			case pixel_format::mono8:
				set_param(XI_PRM_IMAGE_DATA_FORMAT, XI_MONO8);
				break;
			case pixel_format::raw8:
				set_param(XI_PRM_IMAGE_DATA_FORMAT, XI_RAW8);
				break;
			case pixel_format::mono16:
				set_param(XI_PRM_IMAGE_DATA_FORMAT, XI_MONO16);
				break;
			case pixel_format::raw16:
				set_param(XI_PRM_IMAGE_DATA_FORMAT, XI_RAW16);
				break;
			case pixel_format::rgb8:
				set_param(XI_PRM_IMAGE_DATA_FORMAT, XI_RGB24);
				break;
		}
	}


	disposer::module_ptr make_module(disposer::make_data& data){
		auto& outputs = data.outputs;
		bool output_image = outputs.find("image") != outputs.end();
		bool output_info = outputs.find("info") != outputs.end();

		parameter param{};

		data.params.set(param.list_cameras, "list_cameras", false);

		if(param.list_cameras && !output_info){
			throw std::logic_error(data.type_name +
				": parameter list_cameras == true requires info as output");
		}

		if(param.list_cameras && output_image){
			throw std::logic_error(data.type_name +
				": parameter list_cameras == true forbids image as output");
		}

		if(!param.list_cameras && output_info){
			throw std::logic_error(data.type_name +
				": parameter list_cameras == false forbids info as output");
		}

		if(!param.list_cameras && !output_image){
			throw std::logic_error(data.type_name +
				": parameter list_cameras == false requires image as output");
		}

		if(!param.list_cameras){
			data.params.set(param.id, "id", 0);

			std::string format;
			data.params.set(format, "pixel_format", "mono8");

			std::map< std::string, pixel_format > const map{
				{"mono8", pixel_format::mono8},
				{"mono16", pixel_format::mono16},
				{"rgb8", pixel_format::rgb8},
				{"raw8", pixel_format::raw8},
				{"raw16", pixel_format::raw16}
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

			param.format = iter->second;
		}

		return std::make_unique< module >(data, std::move(param));
	}


	void module::exec(){
		if(param.list_cameras){
			info.put(exec_id_list());
		}else{
			switch(param.format){
				case pixel_format::mono8:
				case pixel_format::raw8:
					image.put< std::uint8_t >
						(cam_->get_image< std::uint8_t >());
					break;
				case pixel_format::mono16:
				case pixel_format::raw16:
					image.put< std::uint16_t >
						(cam_->get_image< std::uint16_t >());
					break;
				case pixel_format::rgb8:
					image.put< pixel::rgb8 >
						(cam_->get_image< pixel::rgb8 >());
					break;
			}
		}
	}

	void module::input_ready(){
		if(param.list_cameras){
			info.activate< std::string >();
		}else{
			switch(param.format){
				case pixel_format::mono8:
				case pixel_format::raw8:
					image.activate< std::uint8_t >();
					break;
				case pixel_format::mono16:
				case pixel_format::raw16:
					image.activate< std::uint16_t >();
					break;
				case pixel_format::rgb8:
					image.activate< pixel::rgb8 >();
					break;
			}
		}
	}

	std::string module::exec_id_list()const{
		std::uint32_t device_nr;
		verify(xiGetNumberDevices(&device_nr));
		std::ostringstream os;
		os << "[";
		for(std::size_t i = 0; i < device_nr; ++i){
			ximea_cam cam(*this, i);
			if(i > 0) os << ",";
			os << "{"
				<< "'name':'"
				<< cam.get_param_string(XI_PRM_DEVICE_NAME) << "',"
				<< "'sn':'"
				<< cam.get_param_string(XI_PRM_DEVICE_SN) << "',"
				<< "'type':'"
				<< cam.get_param_string(XI_PRM_DEVICE_TYPE) << "'}";
		}
		os << "]";

		return os.str();
	}


	void init(disposer::module_adder& add){
		add("camera_ximea", &make_module);
	}

	BOOST_DLL_AUTO_ALIAS(init)


} }
