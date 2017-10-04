//-----------------------------------------------------------------------------
// Copyright (c) 2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "vimba.hpp"

#include <bitmap/subbitmap.hpp>

#include <disposer/component.hpp>

#include <boost/dll.hpp>


namespace disposer_module::camera_infratec{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;
	using namespace hana::literals;
	using hana::type_c;

	namespace pixel = ::bmp::pixel;
	using ::bmp::bitmap;


	template < typename Component >
	class FrameObserver: public AVT::VmbAPI::IFrameObserver{
	public:
		FrameObserver(
			Component const& component,
			AVT::VmbAPI::CameraPtr const& pCamera
		)
			: AVT::VmbAPI::IFrameObserver(pCamera)
			, component_(component) {}

		virtual ~FrameObserver()override = default;

		virtual void FrameReceived(AVT::VmbAPI::FramePtr frame)override{
			component_.exception_catching_log(
				[](logsys::stdlogb& os){ os << "received frame"; },
				[this, &frame]{
					VmbFrameStatusType eReceiveStatus;
					verify(frame->GetReceiveStatus(eReceiveStatus));
					verify(eReceiveStatus);

					VmbPixelFormatType format{};
					verify(frame->GetPixelFormat(format));
					if(format != VmbPixelFormatMono16){
						throw std::runtime_error("wrong pixel format");
					}

					VmbUint32_t width = 0;
					VmbUint32_t height = 0;
					verify(frame->GetWidth(width));
					verify(frame->GetHeight(height));

					VmbUchar_t* buffer = nullptr;
					verify(frame->GetImage(buffer));

					auto image = std::make_unique< bitmap< std::uint16_t > >(
						width, height);
					std::copy(buffer, buffer + (width * height * 2),
						reinterpret_cast< VmbUchar_t* >(image->data()));

					set_image(std::move(image));
				});

			// requeue frame
			m_pCamera->QueueFrame(frame);
		}

		bitmap< std::uint16_t > get_image(){
			std::lock_guard lock(mutex_);
			if(!image_) throw std::runtime_error("no image captured");
			return *image_;
		}

	private:
		void set_image(std::unique_ptr< bitmap< std::uint16_t > >&& image){
			std::lock_guard lock(mutex_);
			image_ = std::move(image);
		}

		Component const component_;
		std::unique_ptr< bitmap< std::uint16_t > > image_;
		std::mutex mutex_;
	};

	AVT::VmbAPI::CameraPtr open_camera(
		AVT::VmbAPI::VimbaSystem& system,
		std::string_view ip
	){
		AVT::VmbAPI::CameraPtr result = nullptr;

		verify(system.OpenCameraByID(ip.data(), VmbAccessModeFull, result));

		return result;
	}

	template < typename Component >
	class cam_init{
	public:
		cam_init(Component const& component)
			: component_(component)
			, system_(vimba_system::get())
			, cam_(open_camera(*system_, component("ip"_param).c_str()))
			, frames_(1)
			, observer_(std::make_shared< FrameObserver< Component > >
				(component, cam_))
		{
			AVT::VmbAPI::FeaturePtr feature = nullptr;
			verify(cam_->GetFeatureByName("Width", feature));
			VmbInt64_t width = 0;
			verify(feature->GetValue(width));
			component.log([width](logsys::stdlogb& os){
				os << "width: " << width;
			});

			VmbInt64_t height = 0;
			verify(cam_->GetFeatureByName("Height", feature));
			verify(feature->GetValue(height));
			component.log([height](logsys::stdlogb& os){
				os << "height: " << height;
			});

			VmbInt64_t payload_size = 0;
			verify(cam_->GetFeatureByName("PayloadSize", feature));
			verify(feature->GetValue(payload_size));
			component.log([payload_size](logsys::stdlogb& os){
				os << "payload_size: " << payload_size;
			});

			for(auto& frame: frames_){
				frame.reset(new AVT::VmbAPI::Frame(payload_size));
				verify(frame->RegisterObserver(observer_));
				verify(cam_->AnnounceFrame(frame));
			}

			verify(cam_->StartCapture());
			for(auto& frame: frames_){
				verify(cam_->QueueFrame(frame));
			}

			verify(cam_->GetFeatureByName("AcquisitionStart", feature));
			verify(feature->RunCommand());
		}

		cam_init(cam_init const&) = delete;
		cam_init(cam_init&&) = delete;

		~cam_init(){
			assert(!cam_);
			assert(!system_);
		}

		bitmap< std::uint16_t > get_image(){
			return observer_->get_image();
		}

		void shutdown(){
			if(!cam_) return;

			component_.exception_catching_log(
				[](logsys::stdlogb& os){ os << "acquisition stop"; },
				[this]{
					AVT::VmbAPI::FeaturePtr feature;
					verify(cam_->GetFeatureByName("AcquisitionStop", feature));
					verify(feature->RunCommand());
				});

			component_.exception_catching_log(
				[](logsys::stdlogb& os){ os << "end capture"; },
				[this]{ verify(cam_->EndCapture()); });

			component_.exception_catching_log(
				[](logsys::stdlogb& os){ os << "flush queue"; },
				[this]{ verify(cam_->FlushQueue()); });

			component_.exception_catching_log(
				[](logsys::stdlogb& os){ os << "revoke all frames"; },
				[this]{ verify(cam_->RevokeAllFrames()); });

			for(auto& frame: frames_){
				component_.exception_catching_log(
					[](logsys::stdlogb& os){ os << "unregister observer"; },
					[&frame]{ verify(frame->UnregisterObserver()); });
			}

			component_.exception_catching_log(
				[](logsys::stdlogb& os){ os << "camera close"; },
				[this]{ verify(cam_->Close()); });

			cam_.reset();
			system_.reset();
		}


	private:
		Component const component_;
		std::shared_ptr< AVT::VmbAPI::VimbaSystem > system_;
		AVT::VmbAPI::CameraPtr cam_;
		AVT::VmbAPI::FramePtrVector frames_;
		std::shared_ptr< FrameObserver< Component > > observer_;
	};


	void init(std::string const& name, component_declarant& declarant){
		auto init = component_register_fn(
			component_configure(
				make("ip"_param, free_type_c< std::string >)
			),
			component_modules(
				make("capture"_module, register_fn([](auto& component){
					return module_register_fn(
						module_configure(
							make("image"_out,
								free_type_c< bitmap< std::uint16_t > >)
						),
						exec_fn([&component](auto& module){
							module("image"_out)
								.push(component.state().get_image());
						})
					);
				}))
			),
			component_init_fn([](auto const& component){
				return cam_init(component);
			})
		);

		init(name, declarant);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
