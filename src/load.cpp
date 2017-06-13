//-----------------------------------------------------------------------------
// Copyright (c) 2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "name_generator.hpp"

#include <disposer/module.hpp>

#include <boost/dll.hpp>

#include <fstream>


namespace disposer_module::load{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;
	using namespace hana::literals;
	using hana::type_c;


	using t1 = std::string;
	using t2 = std::vector< std::string >;
	using t3 = std::vector< std::vector< std::string > >;

	constexpr auto types = hana::tuple_t< t1, t2, t3 >;

	using ng1 = name_generator< std::size_t >;
	using ng2 = name_generator< std::size_t, std::size_t >;
	using ng3 = name_generator< std::size_t, std::size_t, std::size_t >;

	constexpr auto name_generator_types = hana::tuple_t< ng1, ng2, ng3 >;


	t1 load(ng1 const& name, std::size_t id){
		std::ifstream is(name(id).c_str(),
			std::ios::in | std::ios::binary);
		return std::string(
			std::istreambuf_iterator<char>(is),
			std::istreambuf_iterator<char>());
	}

	t2 load(ng2 const& name, std::size_t id, std::size_t ic){
		std::vector< std::string > result;
		result.reserve(ic);
		for(std::size_t i = 0; i < ic; ++i){
			std::ifstream is(name(id, i).c_str(),
				std::ios::in | std::ios::binary);
			result.emplace_back(
				std::istreambuf_iterator<char>(is),
				std::istreambuf_iterator<char>());
		}
		return result;
	}

	t3 load(ng3 const& name, std::size_t id, std::size_t ic, std::size_t jc){
		std::vector< std::vector< std::string > > result;
		result.reserve(ic);
		for(std::size_t i = 0; i < ic; ++i){
			result.emplace_back().reserve(ic);
			for(std::size_t j = 0; j < jc; ++j){
				std::ifstream is(name(id, i, j).c_str(),
					std::ios::in | std::ios::binary);
				result.back().emplace_back(
					std::istreambuf_iterator<char>(is),
					std::istreambuf_iterator<char>());
			}
		}
		return result;
	}


	struct format{
		std::size_t const digits;

		std::string operator()(std::size_t value){
			std::ostringstream os;
			os << std::setw(digits) << std::setfill('0') << value;
			return os.str();
		}
	};

	enum class data_type{
		file,
		file_list,
		file_list_list
	};


	void init(std::string const& name, module_declarant& disposer){
		auto init = make_module_register_fn(
			module_configure(
				"type"_param(hana::type_c< data_type >,
					parser([](auto const& /*iop*/, std::string_view data,
						hana::basic_type< data_type >
					){
						if(data == "file"){
							return data_type::file;
						}
						if(data == "file_list"){
							return data_type::file_list;
						}
						if(data == "file_list_list"){
							return data_type::file_list_list;
						}
						throw std::runtime_error("unknown value '"
							+ std::string(data) + "', allowed values are: "
							"file, file_list & file_list_list");
					}),
					default_values(data_type::file),
					type_as_text(
						hana::make_pair(hana::type_c< data_type >, "type"_s)
					)
				),
				"content"_out(types,
					enable([](auto const& iop, auto type)->bool{
						switch(iop("type"_param).get()){
							case data_type::file:
								return type == type_c< t1 >;
							case data_type::file_list:
								return type == type_c< t2 >;
							case data_type::file_list_list:
								return type == type_c< t3 >;
						}

						static_assert(
							type == type_c< t1 > ||
							type == type_c< t2 > ||
							type == type_c< t3 >
						);

						return false;
					})),
				"fixed_id"_param(type_c< std::optional< std::size_t > >),
				"id_modulo"_param(type_c< std::optional< std::size_t > >),
				"id_digits"_param(type_c< std::size_t >,
					default_values(std::size_t(4))),
				"i_digits"_param(type_c< std::size_t >,
					default_values(std::size_t(2))),
				"j_digits"_param(type_c< std::size_t >,
					default_values(std::size_t(2))),
				"name"_param(name_generator_types,
					enable([](auto const& iop, auto type){
						auto const& out = iop("content"_out);
						if constexpr(type == type_c< ng1 >){
							return out.is_enabled(type_c< t1 >);
						}else if constexpr(type == type_c< ng2 >){
							return out.is_enabled(type_c< t2 >);
						}else{
							return out.is_enabled(type_c< t3 >);
						}
					}),
					parser([](
						auto const& iop, std::string_view data, auto type
					){
						if constexpr(type == type_c< ng1 >){
							return make_name_generator(
								data,
								std::make_pair("id",
									format{iop("id_digits"_param).get()})
							);
						}else if constexpr(type == type_c< ng2 >){
							return make_name_generator(
								data,
								std::make_pair("id",
									format{iop("id_digits"_param).get()}),
								std::make_pair("i",
									format{iop("i_digits"_param).get()})
							);
						}else{
							return make_name_generator(
								data,
								std::make_pair("id",
									format{iop("id_digits"_param).get()}),
								std::make_pair("i",
									format{iop("i_digits"_param).get()}),
								std::make_pair("j",
									format{iop("j_digits"_param).get()})
							);
						}
					}),
					type_as_text(
						hana::make_pair(type_c< ng1 >, "file"_s),
						hana::make_pair(type_c< ng2 >, "file_list"_s),
						hana::make_pair(type_c< ng3 >, "file_list_list"_s)
					)
				),
				"i_count"_param(type_c< std::size_t >,
					enable([](auto const& iop, auto /*type*/){
						auto const& out = iop("content"_out);
						return out.is_enabled(type_c< t2 >)
							|| out.is_enabled(type_c< t3 >);
					})),
				"j_count"_param(type_c< std::size_t >,
					enable([](auto const& iop, auto /*type*/){
						auto const& out = iop("content"_out);
						return out.is_enabled(type_c< t3 >);
					}))
			),
			normal_id_increase(),
			[](auto const& /*module*/){
				return [](auto& module, std::size_t id){
					auto fixed_id = module("fixed_id"_param).get();
					if(fixed_id) id = *fixed_id;

					auto const id_modulo = module("id_modulo"_param).get();
					if(id_modulo) id %= *id_modulo;

					auto& out = module("content"_out);
					switch(module("type"_param).get()){
						case data_type::file:
							out.put(load(module("name"_param)
								.get(type_c< ng1 >), id));
							break;
						case data_type::file_list:
							out.put(load(module("name"_param)
								.get(type_c< ng2 >), id,
									module("i_count"_param).get()));
							break;
						case data_type::file_list_list:
							out.put(load(module("name"_param)
								.get(type_c< ng3 >), id,
									module("i_count"_param).get(),
									module("j_count"_param).get()));
							break;
					}
				};
			}
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
