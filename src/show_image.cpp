//-----------------------------------------------------------------------------
// Copyright (c) 2015-2017 Benjamin Buch
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

#include <CImg.h>



namespace disposer_module::show_image{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;
	using hana::type_c;

	namespace pixel = ::bmp::pixel;
	using ::bmp::bitmap;


	constexpr auto types = hana::tuple_t<
			std::uint8_t,
			std::uint16_t,
			pixel::rgb8u
		>;


	struct parameter{
		std::string window_title;
	};


	using cimg_variant = std::variant<
			cimg_library::CImg< std::uint8_t >,
			cimg_library::CImg< std::uint16_t >
		>;


	struct assign_visitor{
		assign_visitor(cimg_variant& img): img(img){}

		cimg_variant& img;

		void operator()(
			std::reference_wrapper< bitmap< uint8_t > const > const img_ref
		)const{
			auto const& image = img_ref.get();
			img = cimg_library::CImg< std::uint8_t >(
				image.data(), image.width(), image.height()
			);
		}

		void operator()(
			std::reference_wrapper< bitmap< uint16_t > const > const img_ref
		)const{
			auto const& image = img_ref.get();
			img = cimg_library::CImg< std::uint16_t >(
				image.data(), image.width(), image.height()
			);
		}

		void operator()(
			std::reference_wrapper< bitmap< pixel::rgb8u > const > const img_ref
		)const{
			auto const& image = img_ref.get();
			img = cimg_library::CImg< std::uint8_t >(
				reinterpret_cast< std::uint8_t const* >(image.data()),
				image.width(), image.height(), 1, 3
			);
		}
	};


	struct resources{
		resources(std::string const& title):
			display(200, 100, title.c_str()){}

		cimg_library::CImgDisplay display;
		cimg_variant img;
	};


	void init(std::string const& name, module_declarant& disposer){
		auto init = module_register_fn(
			module_configure(
				"image"_in(types,
					template_transform_c< bitmap >,
					required),
				"window_title"_param(hana::type_c< std::string >)
			),
			module_enable([](auto const& module){
				return [data_ = resources(module("window_title"_param).get())]
					(auto& module)mutable
				{
					auto values = module("image"_in).get_references();
					for(auto const& img_data: values){
						std::visit(assign_visitor(data_.img), img_data);
						std::visit([&data_](auto const& img){
							if(
								data_.display.width() != img.width() ||
								data_.display.height() != img.height()
							){
								data_.display.resize(img, false);
							}

							data_.display.display(img);
						}, data_.img);
						data_.display.show();
					}
				};
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)



}
