#include "vimba.hpp"

#include <logsys/log.hpp>
#include <logsys/stdlogb.hpp>

#include <stdexcept>


namespace disposer_module{


	std::shared_ptr< AVT::VmbAPI::VimbaSystem > vimba_system::get(){
		std::lock_guard lock(mutex());
		auto ptr = instance().lock();
		if(ptr){
			return ptr;
		}else{
			auto& system = AVT::VmbAPI::VimbaSystem::GetInstance();
			logsys::log(
				[](logsys::stdlogb& os){ os << "startup vimba"; },
				[&system]{ verify(system.Startup()); });
			std::shared_ptr< AVT::VmbAPI::VimbaSystem > new_ptr(
				&system, [](AVT::VmbAPI::VimbaSystem* system){
					logsys::exception_catching_log(
						[](logsys::stdlogb& os){ os << "shutdown vimba"; },
						[system]{ verify(system->Shutdown()); });
				});
			instance() = new_ptr;
			return new_ptr;
		}
	}

	std::mutex& vimba_system::mutex(){
		static std::mutex mutex;
		return mutex;
	}

	std::weak_ptr< AVT::VmbAPI::VimbaSystem >& vimba_system::instance(){
		static std::weak_ptr< AVT::VmbAPI::VimbaSystem > instance;
		return instance;
	}


	void verify(VmbErrorType state){
		switch(state){
			case VmbErrorSuccess:
				return;
			case VmbErrorInternalFault:
				throw std::runtime_error("VmbErrorInternalFault: "
					"Unexpected fault in VimbaC or driver");
			case VmbErrorApiNotStarted:
				throw std::runtime_error("VmbErrorApiNotStarted: "
					"VmbStartup() was not called before the current command");
			case VmbErrorNotFound:
				throw std::runtime_error("VmbErrorNotFound: "
					"The designated instance (camera, feature etc.) cannot be "
					"found");
			case VmbErrorBadHandle:
				throw std::runtime_error("VmbErrorBadHandle: "
					"The given handle is not valid");
			case VmbErrorDeviceNotOpen:
				throw std::runtime_error("VmbErrorDeviceNotOpen: "
					"Device was not opened for usage");
			case VmbErrorInvalidAccess:
				throw std::runtime_error("VmbErrorInvalidAccess: "
					"Operation is invalid with the current access mode");
			case VmbErrorBadParameter:
				throw std::runtime_error("VmbErrorBadParameter: "
					"One of the parameters is invalid "
					"(usually an illegal pointer)");
			case VmbErrorStructSize:
				throw std::runtime_error("VmbErrorStructSize: "
					"The given struct size is not valid for this version of "
					"the API");
			case VmbErrorMoreData:
				throw std::runtime_error("VmbErrorMoreData: "
					"More data available in a string/list than space is "
					"provided");
			case VmbErrorWrongType:
				throw std::runtime_error("VmbErrorWrongType: "
					"Wrong feature type for this access function");
			case VmbErrorInvalidValue:
				throw std::runtime_error("VmbErrorInvalidValue: "
					"The value is not valid; either out of bounds or not an "
					"increment of the minimum");
			case VmbErrorTimeout:
				throw std::runtime_error("VmbErrorTimeout: "
					"Timeout during wait");
			case VmbErrorOther:
				throw std::runtime_error("VmbErrorOther: "
					"Other error");
			case VmbErrorResources:
				throw std::runtime_error("VmbErrorResources: "
					"Resources not available (e.g. memory)");
			case VmbErrorInvalidCall:
				throw std::runtime_error("VmbErrorInvalidCall: "
					"Call is invalid in the current context (e.g. callback)");
			case VmbErrorNoTL:
				throw std::runtime_error("VmbErrorNoTL: "
					"No transport layers are found (did you install Vimba?)");
			case VmbErrorNotImplemented:
				throw std::runtime_error("VmbErrorNotImplemented: "
					"API feature is not implemented");
			case VmbErrorNotSupported:
				throw std::runtime_error("VmbErrorNotSupported: "
					"API feature is not supported");
			case VmbErrorIncomplete:
				throw std::runtime_error("VmbErrorIncomplete: "
					"A multiple registers read or write is partially "
					"completed");
		}
		throw std::logic_error("unknown Vimba VmbErrorType");
	}

	void verify(VmbFrameStatusType state){
		switch(state){
			case VmbFrameStatusComplete:
				return;
			case VmbFrameStatusIncomplete:
				throw std::runtime_error("VmbFrameStatusIncomplete: "
					"Frame could not be filled to the end");
			case VmbFrameStatusTooSmall:
				throw std::runtime_error("VmbFrameStatusTooSmall: "
					"Frame buffer was too small");
			case VmbFrameStatusInvalid:
				throw std::runtime_error("VmbFrameStatusInvalid: "
					"Frame buffer was invalid");
		}
		throw std::logic_error("unknown Vimba VmbFrameStatusType");
	}


}
