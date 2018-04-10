//-----------------------------------------------------------------------------
// Copyright (c) 2017-2018 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "vimba.hpp"

#include <bitmap/subbitmap.hpp>

#include <io_tools/range_to_string.hpp>

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
			Component const component,
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

					if(
						width != component_("width"_param) ||
						height != component_("height"_param)
					){
						throw std::runtime_error(
							"Image size is " + std::to_string(width) + "×" +
							std::to_string(height) + " but " +
							std::to_string(component_("width"_param)) + "×" +
							std::to_string(component_("height"_param)) +
							" was expected");
					}

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

	struct range{
		double min;
		double max;
	};

	template < typename Component >
	class cam_init{
	public:
		cam_init(Component const component)
			: component_(component)
			, system_(vimba_system::get())
			, cam_(open_camera(*system_, component_("ip"_param).c_str()))
			, frames_(1)
			, observer_(std::make_shared< FrameObserver< Component > >
				(component_, cam_))
			, distance([this]{
					AVT::VmbAPI::FeaturePtr feature = nullptr;
					verify(cam_->GetFeatureByName("FocDistM", feature));
					range value;
					verify(feature->GetRange(value.min, value.max));
					return value;
				}())
		{
			AVT::VmbAPI::FeaturePtr feature = nullptr;

			// TODO; Set image size
// 			verify(cam_->GetFeatureByName("Width", feature));
// 			VmbInt64_t width = 0;
// 			verify(feature->GetValue(width));
//
// 			verify(cam_->GetFeatureByName("Height", feature));
// 			VmbInt64_t height = 0;
// 			verify(feature->GetValue(height));
//
// 			if(
// 				width != component_("width"_param) ||
// 				height != component_("height"_param)
// 			){
// 				throw std::runtime_error(
// 					"Image size is " + std::to_string(width) + "×" +
// 					std::to_string(height) + " but " +
// 					std::to_string(component_("width"_param)) + "×" +
// 					std::to_string(component_("height"_param)) +
// 					" was expected");
// 			}

			VmbInt64_t width = component_("width"_param);
			VmbInt64_t height = component_("height"_param);

			for(auto& frame: frames_){
				frame.reset(new AVT::VmbAPI::Frame(width * height * 2));
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

		bitmap< std::uint16_t > get_image(){
			return observer_->get_image();
		}

		AVT::VmbAPI::CameraPtr cam(){
			return cam_;
		}

	private:
		Component const component_;
		std::shared_ptr< AVT::VmbAPI::VimbaSystem > system_;
		AVT::VmbAPI::CameraPtr cam_;
		AVT::VmbAPI::FramePtrVector frames_;
		std::shared_ptr< FrameObserver< Component > > observer_;

	public:
		range const distance;
	};

	constexpr std::array< std::string_view, 4 > type_list{{
			"bool",
			"int",
			"float",
			"string"
		}};

	std::string type_list_string(){
		std::ostringstream os;
		bool first = true;
		for(auto name: type_list){
			if(first){
				first = false;
			}else{
				os << ", ";
			}
			os << name;
		}
		return os.str();
	}

	struct settings_state{
		AVT::VmbAPI::FeaturePtr value_feature = nullptr;
	};

	void init(std::string const& name, declarant& declarant){
		auto init = generate_component(
			"controller for InfraTec GigE-Vision cameras via AVT Vimba",
			component_configure(
				make("ip"_param, free_type_c< std::string >,
					"IP-Address of the camera"),
				make("width"_param, free_type_c< std::size_t >,
					"expected image width while image acquisition"),
				make("height"_param, free_type_c< std::size_t >,
					"expected image height while image acquisition")
			),
			component_init_fn([](auto const component){
				return cam_init(component);
			}),
			component_modules(
				make("capture"_module, generate_module(
					"capture an image from the camera",
					module_configure(
						make("image"_out,
							free_type_c< bitmap< std::uint16_t > >,
							"the captured image")
					),
					exec_fn([](auto module){
						module("image"_out)
							.push(module.component.state().get_image());
					}),
					no_overtaking
				)),
				make("focus"_module, generate_module(
					"set camera focus",
					module_configure(
						make("distance_in_m"_param, free_type_c< double >,
							"the focus in meter",
							verify_value_fn(
								[](double value, auto const module){
									auto& state = module.component.state();
									if(
										value >= state.distance.min &&
										value <= state.distance.max
									) return;

									throw std::out_of_range(
										io_tools::make_string(
										"distance is out of range "
										"(value: ", value,
										", min: ", state.distance.min,
										", max: ", state.distance.max,
										")"));
								}))
					),
					exec_fn([](auto module){
						AVT::VmbAPI::FeaturePtr feature = nullptr;

						verify(module.component.state().cam()
							->GetFeatureByName("FocDistM", feature));
						verify(feature->SetValue(
							module("distance_in_m"_param)));
					}),
					no_overtaking
				)),
				make("setting"_module, generate_module(
					"set a feature to a value",
					dimension_list{
						dimension_c<
							bool,
							VmbInt64_t,
							double,
							std::string
						>
					},
					module_configure(
						make("name"_param, free_type_c< std::string >,
							"name of the feature"),
						make("type"_param, free_type_c< std::size_t >,
							"type of the feature, valid values are: "
								+ io_tools::range_to_string(type_list),
							parser_fn([](std::string_view data){
								auto iter = std::find(type_list.begin(),
									type_list.end(), data);
								if(iter == type_list.end()){
									std::ostringstream os;
									os << "unknown value '" << data
										<< "', valid values are: "
										<< type_list_string();
									throw std::runtime_error(os.str());
								}
								return iter - type_list.begin();
							})),
						set_dimension_fn([](auto const module){
								std::size_t const number =
									module("type"_param);
								return solved_dimensions{
									index_component< 0 >{number}};
							}),
						make("value"_param, type_ref_c< 0 >,
							"the value to be set")),
					module_init_fn([](auto module){
						settings_state state;

						auto const name = "name"_param;
						verify(module.component.state().cam()->GetFeatureByName(
							module(name).c_str(), state.value_feature));

						auto type = module.dimension(hana::size_c< 0 >);
						auto const type_index = module("type"_param);

						VmbFeatureDataType feature_type{};
						verify(state.value_feature
							->GetDataType(feature_type));
						switch(feature_type){
							case VmbFeatureDataUnknown:
								throw std::logic_error("feature '"
									+ detail::to_std_string(name) +
									"' has type 'unknown'"
									" which is not suppored");
							case VmbFeatureDataEnum:
								throw std::logic_error("feature '"
									+ detail::to_std_string(name) +
									"' has type 'enum' which"
									" is currently not suppored");
							break;
							case VmbFeatureDataInt:
								if(type != hana::type_c< VmbInt64_t >){
									throw std::logic_error("feature '"
										+ detail::to_std_string(name) +
										"' has type 'int',"
										" but module expected type '"
										+ std::string(type_list[type_index])
										+ "'");
								}
							break;
							case VmbFeatureDataFloat:
								if(type != hana::type_c< double >){
									throw std::logic_error("feature '"
										+ detail::to_std_string(name) +
										"' has type 'float',"
										" but module expected type '"
										+ std::string(type_list[type_index])
										+ "'");
								}
							break;
							case VmbFeatureDataString:
								if(type != hana::type_c< std::string >){
									throw std::logic_error("feature '"
										+ detail::to_std_string(name) +
										"' has type 'string',"
										" but module expected type '"
										+ std::string(type_list[type_index])
										+ "'");
								}
							break;
							case VmbFeatureDataBool:
								if(type != hana::type_c< bool >){
									throw std::logic_error("feature '"
										+ detail::to_std_string(name) +
										"' has type 'bool',"
										" but module expected type '"
										+ std::string(type_list[type_index])
										+ "'");
								}
							break;
							case VmbFeatureDataCommand:
								throw std::logic_error("feature '"
									+ detail::to_std_string(name) +
									"' has type 'command' which is"
									" currently not suppored");
							break;
							case VmbFeatureDataRaw:
								throw std::logic_error("feature '"
									+ detail::to_std_string(name) +
									"' has type 'raw' which is currently"
									" not suppored");
							break;
							case VmbFeatureDataNone:
								throw std::logic_error("feature '"
									+ detail::to_std_string(name) +
									"' has type 'none' which is currently"
									" not suppored");
							break;
						}

						return state;
					}),
					exec_fn([](auto module){
						auto& state = module.state();
						auto type = module.dimension(hana::size_c< 0 >);
						if constexpr(type == hana::type_c< std::string >){
							verify(state.value_feature->SetValue(
								module("value"_param).c_str()));
						}else{
							verify(state.value_feature->SetValue(
								module("value"_param)));
						}
					}),
					no_overtaking
				))
			)
		);

		init(name, declarant);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
