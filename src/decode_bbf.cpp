//-----------------------------------------------------------------------------
// Copyright (c) 2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <bitmap/binary_read.hpp>

#include <disposer/module.hpp>

#include <boost/dll.hpp>


namespace disposer_module::decode_bbf{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;
	using namespace hana::literals;
	using hana::type_c;

	namespace pixel = ::bmp::pixel;
	using ::bmp::bitmap;


	constexpr auto types = hana::tuple_t<
			bool
			, std::int8_t
			, std::uint8_t
			, std::int16_t
			, std::uint16_t
			, std::int32_t
			, std::uint32_t
			, std::int64_t
			, std::uint64_t
			, float
			, double
			, pixel::ga8
			, pixel::ga16
			, pixel::ga32
			, pixel::ga64
			, pixel::ga8u
			, pixel::ga16u
			, pixel::ga32u
			, pixel::ga64u
			, pixel::ga32f
			, pixel::ga64f
			, pixel::rgb8
			, pixel::rgb16
			, pixel::rgb32
			, pixel::rgb64
			, pixel::rgb8u
			, pixel::rgb16u
			, pixel::rgb32u
			, pixel::rgb64u
			, pixel::rgb32f
			, pixel::rgb64f
			, pixel::rgba8
			, pixel::rgba16
			, pixel::rgba32
			, pixel::rgba64
			, pixel::rgba8u
			, pixel::rgba16u
			, pixel::rgba32u
			, pixel::rgba64u
			, pixel::rgba32f
			, pixel::rgba64f
		>;


	enum class format{
		bool_
		, g8
		, g16
		, g32
		, g64
		, g8u
		, g16u
		, g32u
		, g64u
		, g32f
		, g64f
		, ga8
		, ga16
		, ga32
		, ga64
		, ga8u
		, ga16u
		, ga32u
		, ga64u
		, ga32f
		, ga64f
		, rgb8
		, rgb16
		, rgb32
		, rgb64
		, rgb8u
		, rgb16u
		, rgb32u
		, rgb64u
		, rgb32f
		, rgb64f
		, rgba8
		, rgba16
		, rgba32
		, rgba64
		, rgba8u
		, rgba16u
		, rgba32u
		, rgba64u
		, rgba32f
		, rgba64f
	};

	format text_to_format(std::string const& value){
		static std::unordered_map< std::string, format > const map{
			{"bool", format::bool_}
			, {"g8s", format::g8}
			, {"g16s", format::g16}
			, {"g32s", format::g32}
			, {"g64s", format::g64}
			, {"g8u", format::g8u}
			, {"g16u", format::g16u}
			, {"g32u", format::g32u}
			, {"g64u", format::g64u}
			, {"g32f", format::g32f}
			, {"g64f", format::g64f}
			, {"ga8s", format::ga8}
			, {"ga16s", format::ga16}
			, {"ga32s", format::ga32}
			, {"ga64s", format::ga64}
			, {"ga8u", format::ga8u}
			, {"ga16u", format::ga16u}
			, {"ga32u", format::ga32u}
			, {"ga64u", format::ga64u}
			, {"ga32f", format::ga32f}
			, {"ga64f", format::ga64f}
			, {"rgb8s", format::rgb8}
			, {"rgb16s", format::rgb16}
			, {"rgb32s", format::rgb32}
			, {"rgb64s", format::rgb64}
			, {"rgb8u", format::rgb8u}
			, {"rgb16u", format::rgb16u}
			, {"rgb32u", format::rgb32u}
			, {"rgb64u", format::rgb64u}
			, {"rgb32f", format::rgb32f}
			, {"rgb64f", format::rgb64f}
			, {"rgba8s", format::rgba8}
			, {"rgba16s", format::rgba16}
			, {"rgba32s", format::rgba32}
			, {"rgba64s", format::rgba64}
			, {"rgba8u", format::rgba8u}
			, {"rgba16u", format::rgba16u}
			, {"rgba32u", format::rgba32u}
			, {"rgba64u", format::rgba64u}
			, {"rgba32f", format::rgba32f}
			, {"rgba64f", format::rgba64f}
		};
		auto iter = map.find(value);
		if(iter == map.end()){
			std::ostringstream os;
			os << "unknown value '" + value << "', allowed values are: ";
			bool first = true;
			for(auto const& [name, format]: map){
				if(first){ first = false; }else{ os << ", "; }
				(void)format;
				os << name;
			}
			throw std::runtime_error(os.str());
		}
		return iter->second;
	}


	constexpr auto type_as_format = hana::make_map(
		hana::make_pair(hana::type_c< bool >, format::bool_)
		, hana::make_pair(hana::type_c< std::int8_t >, format::g8)
		, hana::make_pair(hana::type_c< std::uint8_t >, format::g16)
		, hana::make_pair(hana::type_c< std::int16_t >, format::g32)
		, hana::make_pair(hana::type_c< std::uint16_t >, format::g64)
		, hana::make_pair(hana::type_c< std::int32_t >, format::g8u)
		, hana::make_pair(hana::type_c< std::uint32_t >, format::g16u)
		, hana::make_pair(hana::type_c< std::int64_t >, format::g32u)
		, hana::make_pair(hana::type_c< std::uint64_t >, format::g64u)
		, hana::make_pair(hana::type_c< float >, format::g32f)
		, hana::make_pair(hana::type_c< double >, format::g64f)
		, hana::make_pair(hana::type_c< pixel::ga8 >, format::ga8)
		, hana::make_pair(hana::type_c< pixel::ga16 >, format::ga16)
		, hana::make_pair(hana::type_c< pixel::ga32 >, format::ga32)
		, hana::make_pair(hana::type_c< pixel::ga64 >, format::ga64)
		, hana::make_pair(hana::type_c< pixel::ga8u >, format::ga8u)
		, hana::make_pair(hana::type_c< pixel::ga16u >, format::ga16u)
		, hana::make_pair(hana::type_c< pixel::ga32u >, format::ga32u)
		, hana::make_pair(hana::type_c< pixel::ga64u >, format::ga64u)
		, hana::make_pair(hana::type_c< pixel::ga32f >, format::ga32f)
		, hana::make_pair(hana::type_c< pixel::ga64f >, format::ga64f)
		, hana::make_pair(hana::type_c< pixel::rgb8 >, format::rgb8)
		, hana::make_pair(hana::type_c< pixel::rgb16 >, format::rgb16)
		, hana::make_pair(hana::type_c< pixel::rgb32 >, format::rgb32)
		, hana::make_pair(hana::type_c< pixel::rgb64 >, format::rgb64)
		, hana::make_pair(hana::type_c< pixel::rgb8u >, format::rgb8u)
		, hana::make_pair(hana::type_c< pixel::rgb16u >, format::rgb16u)
		, hana::make_pair(hana::type_c< pixel::rgb32u >, format::rgb32u)
		, hana::make_pair(hana::type_c< pixel::rgb64u >, format::rgb64u)
		, hana::make_pair(hana::type_c< pixel::rgb32f >, format::rgb32f)
		, hana::make_pair(hana::type_c< pixel::rgb64f >, format::rgb64f)
		, hana::make_pair(hana::type_c< pixel::rgba8 >, format::rgba8)
		, hana::make_pair(hana::type_c< pixel::rgba16 >, format::rgba16)
		, hana::make_pair(hana::type_c< pixel::rgba32 >, format::rgba32)
		, hana::make_pair(hana::type_c< pixel::rgba64 >, format::rgba64)
		, hana::make_pair(hana::type_c< pixel::rgba8u >, format::rgba8u)
		, hana::make_pair(hana::type_c< pixel::rgba16u >, format::rgba16u)
		, hana::make_pair(hana::type_c< pixel::rgba32u >, format::rgba32u)
		, hana::make_pair(hana::type_c< pixel::rgba64u >, format::rgba64u)
		, hana::make_pair(hana::type_c< pixel::rgba32f >, format::rgba32f)
		, hana::make_pair(hana::type_c< pixel::rgba64f >, format::rgba64f)
	);


	template < typename T >
	auto decode(std::string const& data){
		bitmap< T > result;
		binary_read(result, data);
		return result;
	}

	void init(std::string const& name, module_declarant& disposer){
		auto init = module_register_fn(
			module_configure(
				"data"_in(type_c< std::string >),
				"format"_param(type_c< format >,
					parser([](
						auto const& /*iop*/,
						std::string_view data,
						hana::basic_type< format >
					){
						return text_to_format(std::string(data));
					}),
					type_as_text(
						hana::make_pair(type_c< format >, "format"_s)
					)
				),
				"image"_out(types,
					template_transform_c< bitmap >,
					enable([](auto const& iop, auto type){
						auto const format = iop("format"_param).get();
						return type_as_format[type] == format;
					})
				)
			),
			module_enable([]{
				return [](auto& module){
					auto& out = module("image"_out);
					auto values = module("data"_in).get_references();
					for(auto const& value: values){
						auto const& data = value;
						switch(module("format"_param).get()){
						case format::bool_:
							out.put(decode< bool >(data)); break;
						case format::g8:
							out.put(decode< std::int8_t >(data)); break;
						case format::g16:
							out.put(decode< std::uint8_t >(data)); break;
						case format::g32:
							out.put(decode< std::int16_t >(data)); break;
						case format::g64:
							out.put(decode< std::uint16_t >(data)); break;
						case format::g8u:
							out.put(decode< std::int32_t >(data)); break;
						case format::g16u:
							out.put(decode< std::uint32_t >(data)); break;
						case format::g32u:
							out.put(decode< std::int64_t >(data)); break;
						case format::g64u:
							out.put(decode< std::uint64_t >(data)); break;
						case format::g32f:
							out.put(decode< float >(data)); break;
						case format::g64f:
							out.put(decode< double >(data)); break;
						case format::ga8:
							out.put(decode< pixel::ga8 >(data)); break;
						case format::ga16:
							out.put(decode< pixel::ga16 >(data)); break;
						case format::ga32:
							out.put(decode< pixel::ga32 >(data)); break;
						case format::ga64:
							out.put(decode< pixel::ga64 >(data)); break;
						case format::ga8u:
							out.put(decode< pixel::ga8u >(data)); break;
						case format::ga16u:
							out.put(decode< pixel::ga16u >(data)); break;
						case format::ga32u:
							out.put(decode< pixel::ga32u >(data)); break;
						case format::ga64u:
							out.put(decode< pixel::ga64u >(data)); break;
						case format::ga32f:
							out.put(decode< pixel::ga32f >(data)); break;
						case format::ga64f:
							out.put(decode< pixel::ga64f >(data)); break;
						case format::rgb8:
							out.put(decode< pixel::rgb8 >(data)); break;
						case format::rgb16:
							out.put(decode< pixel::rgb16 >(data)); break;
						case format::rgb32:
							out.put(decode< pixel::rgb32 >(data)); break;
						case format::rgb64:
							out.put(decode< pixel::rgb64 >(data)); break;
						case format::rgb8u:
							out.put(decode< pixel::rgb8u >(data)); break;
						case format::rgb16u:
							out.put(decode< pixel::rgb16u >(data)); break;
						case format::rgb32u:
							out.put(decode< pixel::rgb32u >(data)); break;
						case format::rgb64u:
							out.put(decode< pixel::rgb64u >(data)); break;
						case format::rgb32f:
							out.put(decode< pixel::rgb32f >(data)); break;
						case format::rgb64f:
							out.put(decode< pixel::rgb64f >(data)); break;
						case format::rgba8:
							out.put(decode< pixel::rgba8 >(data)); break;
						case format::rgba16:
							out.put(decode< pixel::rgba16 >(data)); break;
						case format::rgba32:
							out.put(decode< pixel::rgba32 >(data)); break;
						case format::rgba64:
							out.put(decode< pixel::rgba64 >(data)); break;
						case format::rgba8u:
							out.put(decode< pixel::rgba8u >(data)); break;
						case format::rgba16u:
							out.put(decode< pixel::rgba16u >(data)); break;
						case format::rgba32u:
							out.put(decode< pixel::rgba32u >(data)); break;
						case format::rgba64u:
							out.put(decode< pixel::rgba64u >(data)); break;
						case format::rgba32f:
							out.put(decode< pixel::rgba32f >(data)); break;
						case format::rgba64f:
							out.put(decode< pixel::rgba64f >(data)); break;
						}
					}
				};
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
