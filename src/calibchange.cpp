//-----------------------------------------------------------------------------
// Copyright (c) 2017-2018 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <disposer/module.hpp>

#include <boost/asio.hpp>

#include <boost/dll.hpp>


namespace disposer_module::calibchange{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;


	void init(std::string const& name, declarant& disposer){
		auto init = generate_module(
			"A BAD HACK: send binary packages to a InfraTec VarioCam HD "
			"to set a calibration (handshake, set CalibSelect, "
			"exec CalibChange)",
			module_configure(
				make("ip"_param, free_type_c< std::string >,
					"ID-Address of the camera"),
				make("value"_param, free_type_c< std::uint16_t >,
					"value of the CalibSelect feature")
			),
			exec_fn([](auto module){
				namespace asio = boost::asio;
				using asio::ip::udp;

				asio::io_service io_service;

				udp::resolver resolver(io_service);
				udp::resolver::query query(
					udp::v4(), module("ip"_param), "3956");
				udp::endpoint receiver_endpoint = *resolver.resolve(query);

				udp::socket socket(io_service);
				socket.open(udp::v4());


				udp::endpoint sender_endpoint;
				std::array< std::uint8_t, 12 > res_buffer;

				std::array< std::uint8_t, 16 > open_cam_req = {{
					0x42, 0x01, 0x00, 0x82, 0x00, 0x08, 0x03, 0xe8,
					0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x01 }};

				std::array< std::uint8_t, 12 > open_cam_res = {{
					0x00, 0x00, 0x00, 0x83, 0x00, 0x04, 0x03, 0xe8,
					0x00, 0x00, 0x00, 0x01 }};

				socket.send_to(asio::buffer(open_cam_req), receiver_endpoint);

				auto length = socket.receive_from(
					asio::buffer(res_buffer), sender_endpoint);
				if(length != 12 || res_buffer != open_cam_res){
					throw std::runtime_error("OpenCamera failed");
				}


				std::uint16_t const value = module("value"_param);
				std::array< std::uint8_t, 16 > calib_select_req = {{
					0x42, 0x01, 0x00, 0x82, 0x00, 0x08, 0x03, 0xe9,
					0x00, 0x00, 0xc0, 0x3c, 0x00, 0x00,
					static_cast< std::uint8_t >(value >> 8),
					static_cast< std::uint8_t >(value)}};

				std::array< std::uint8_t, 12 > calib_select_res = {{
					0x00, 0x00, 0x00, 0x83, 0x00, 0x04, 0x03, 0xe9,
					0x00, 0x00, 0x00, 0x01 }};

				socket.send_to(asio::buffer(calib_select_req),
					receiver_endpoint);

				length = socket.receive_from(
					asio::buffer(res_buffer), sender_endpoint);
				if(length != 12 || res_buffer != calib_select_res){
					throw std::runtime_error("CalibSelect failed");
				}


				std::array< std::uint8_t, 16 > calib_change_req = {{
					0x42, 0x01, 0x00, 0x82, 0x00, 0x08, 0x03, 0xea,
					0x00, 0x00, 0xc0, 0x0c, 0x00, 0x00, 0x00, 0x04 }};

				std::array< std::uint8_t, 12 > calib_change_res = {{
					0x00, 0x00, 0x00, 0x83, 0x00, 0x04, 0x03, 0xea,
					0x00, 0x00, 0x00, 0x01 }};

				socket.send_to(asio::buffer(calib_change_req),
					receiver_endpoint);

				length = socket.receive_from(
					asio::buffer(res_buffer), sender_endpoint);
				if(length != 12 || res_buffer != calib_change_res){
					throw std::runtime_error("CalibChange failed");
				}


				module.log([](logsys::stdlogb& os){ os << "success"; });
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
