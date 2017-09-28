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

	using ng1 =
		name_generator< std::size_t, std::size_t >;
	using ng2 =
		name_generator< std::size_t, std::size_t, std::size_t >;
	using ng3 =
		name_generator< std::size_t, std::size_t, std::size_t, std::size_t >;

	template < typename T >
	struct type_transform;

	template <>
	struct type_transform< t1 >{
		using name_generator = ng1;
		using i_type = void;
		using j_type = void;
	};

	template <>
	struct type_transform< t2 >{
		using name_generator = ng2;
		using i_type = std::size_t;
		using j_type = void;
	};

	template <>
	struct type_transform< t3 >{
		using name_generator = ng3;
		using i_type = std::size_t;
		using j_type = std::size_t;
	};

	template < typename T >
	using to_name_generator_t = typename type_transform< T >::name_generator;

	template < typename T >
	using i_type = typename type_transform< T >::i_type;

	template < typename T >
	using j_type = typename type_transform< T >::j_type;


	void verify_stream(std::ifstream& is, std::string const& filename){
		if(is) return;
		throw std::runtime_error("Can not read file '" + filename + "'");
	}

	t1 load(
		ng1 const& name,
		std::size_t id,
		std::size_t subid
	){
		auto const filename = name(id, subid);
		std::ifstream is(filename.c_str(),
			std::ios::in | std::ios::binary);
		verify_stream(is, filename);
		return std::string(
			std::istreambuf_iterator<char>(is),
			std::istreambuf_iterator<char>());
	}

	t2 load(
		ng2 const& name,
		std::size_t id,
		std::size_t subid,
		std::size_t ic
	){
		std::vector< std::string > result;
		result.reserve(ic);
		for(std::size_t i = 0; i < ic; ++i){
			auto const filename = name(id, subid, i);
			std::ifstream is(filename.c_str(),
				std::ios::in | std::ios::binary);
			verify_stream(is, filename);
			result.emplace_back(
				std::istreambuf_iterator<char>(is),
				std::istreambuf_iterator<char>());
		}
		return result;
	}

	t3 load(
		ng3 const& name,
		std::size_t id,
		std::size_t subid,
		std::size_t ic,
		std::size_t jc
	){
		std::vector< std::vector< std::string > > result;
		result.reserve(ic);
		for(std::size_t i = 0; i < ic; ++i){
			result.emplace_back().reserve(ic);
			for(std::size_t j = 0; j < jc; ++j){
				auto const filename = name(id, subid, i, j);
				std::ifstream is(filename.c_str(),
					std::ios::in | std::ios::binary);
				verify_stream(is, filename);
				result.back().emplace_back(
					std::istreambuf_iterator<char>(is),
					std::istreambuf_iterator<char>());
			}
		}
		return result;
	}


	struct format{
		std::size_t const digits;
		std::size_t const add;

		std::string operator()(std::size_t value){
			std::ostringstream os;
			os << std::setw(digits) << std::setfill('0') << add + value;
			return os.str();
		}
	};

	enum class data_type{
		file,
		file_list,
		file_list_list
	};


	void init(std::string const& name, module_declarant& disposer){
		auto expect_greater_0 =
			verify_value_fn([](std::size_t value){
				if(value == 0){
					throw std::logic_error("must be greater 0");
				}
			});

		auto init = module_register_fn(
			dimension_list{
				dimension_c<
					std::string,
					std::vector< std::string >,
					std::vector< std::vector< std::string > >
				>
			},
			module_configure(
				make("type"_param, free_type_c< data_type >,
					parser_fn([](auto const& /*iop*/, std::string_view data,
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
					default_value(data_type::file)),
				make("fixed_id"_param,
					free_type_c< std::optional< std::size_t > >),
				make("id_modulo"_param,
					free_type_c< std::optional< std::size_t > >),
				make("id_digits"_param, free_type_c< std::size_t >,
					default_value(4)),
				make("subid_digits"_param, free_type_c< std::size_t >,
					default_value(1)),
				make("id_add"_param, free_type_c< std::size_t >,
					default_value(0)),
				make("subid_add"_param, free_type_c< std::size_t >,
					default_value(0)),
				make("subid_count"_param, free_type_c< std::size_t >,
					default_value(1),
					expect_greater_0),
				set_dimension_fn([](auto const& module){
					auto const type = module("type"_param);
					switch(type){
						case data_type::file:
							return solved_dimensions{index_component< 0 >{0}};
						case data_type::file_list:
							return solved_dimensions{index_component< 0 >{1}};
						case data_type::file_list_list:
							return solved_dimensions{index_component< 0 >{2}};
					}

					throw std::runtime_error("unknown format");
				}),
				make("i_add"_param, wrapped_type_ref_c< i_type, 0 >,
					default_value(0)),
				make("j_add"_param, wrapped_type_ref_c< j_type, 0 >,
					default_value(0)),
				make("i_digits"_param, wrapped_type_ref_c< i_type, 0 >,
					default_value(2)),
				make("j_digits"_param, wrapped_type_ref_c< j_type, 0 >,
					default_value(2)),
				make("i_count"_param, wrapped_type_ref_c< i_type, 0 >,
					expect_greater_0),
				make("j_count"_param, wrapped_type_ref_c< j_type, 0 >,
					expect_greater_0),
				make("content"_out, type_ref_c< 0 >),
				make("name"_param,
					wrapped_type_ref_c< to_name_generator_t, 0 >,
					parser_fn([](
						auto const& iop, std::string_view data, auto type
					){
						if constexpr(type == type_c< ng1 >){
							return make_name_generator(
								data,
								std::make_pair("id",
									format{iop("id_digits"_param),
										iop("id_add"_param)}),
								std::make_pair("subid",
									format{iop("subid_digits"_param),
										iop("subid_add"_param)})
							);
						}else if constexpr(type == type_c< ng2 >){
							return make_name_generator(
								data,
								std::make_pair("id",
									format{iop("id_digits"_param),
										iop("id_add"_param)}),
								std::make_pair("subid",
									format{iop("subid_digits"_param),
										iop("subid_add"_param)}),
								std::make_pair("i",
									format{iop("i_digits"_param),
										iop("i_add"_param)})
							);
						}else{
							return make_name_generator(
								data,
								std::make_pair("id",
									format{iop("id_digits"_param),
										iop("id_add"_param)}),
								std::make_pair("subid",
									format{iop("subid_digits"_param),
										iop("subid_add"_param)}),
								std::make_pair("i",
									format{iop("i_digits"_param),
										iop("i_add"_param)}),
								std::make_pair("j",
									format{iop("j_digits"_param),
										iop("j_add"_param)})
							);
						}
					})
				)
			),
			exec_fn([](auto& module){
				auto id = module.id();

				auto fixed_id = module("fixed_id"_param);
				if(fixed_id) id = *fixed_id;

				auto const id_modulo = module("id_modulo"_param);
				if(id_modulo) id %= *id_modulo;

				auto const subid_count = module("subid_count"_param);

				using type = typename
					decltype(module.dimension(hana::size_c< 0 >))::type;

				auto& out = module("content"_out);
				for(std::size_t subid = 0; subid < subid_count; ++subid){
					if constexpr(std::is_same_v< type, t1 >){
						out.push(load(module("name"_param), id, subid));
					}else if constexpr(std::is_same_v< type, t2 >){
						out.push(load(module("name"_param), id, subid,
							module("i_count"_param)));
					}else if constexpr(std::is_same_v< type, t3 >){
						out.push(load(module("name"_param), id, subid,
							module("i_count"_param),
							module("j_count"_param)));
					}
				}
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
