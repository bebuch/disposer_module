//-----------------------------------------------------------------------------
// Copyright (c) 2017-2018 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <disposer/module.hpp>

#include <io_tools/name_generator.hpp>
#include <io_tools/range_to_string.hpp>

#include <boost/dll.hpp>

#include <fstream>


namespace disposer_module::load{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;
	using namespace hana::literals;
	using namespace std::literals::string_literals;
	using hana::type_c;
	using io_tools::name_generator;
	using io_tools::make_name_generator;


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

	template < typename Module >
	t1 load(
		Module module,
		ng1 const& name,
		std::size_t id,
		std::size_t subid
	){
		auto const filename = name(id, subid);
		return module.log([&filename](logsys::stdlogb& os){
				os << filename;
			}, [&filename]{
				std::ifstream is(filename.c_str(),
					std::ios::in | std::ios::binary);
				verify_stream(is, filename);
				return std::string(
					std::istreambuf_iterator<char>(is),
					std::istreambuf_iterator<char>());
			});
	}

	template < typename Module >
	t2 load(
		Module module,
		ng2 const& name,
		std::size_t id,
		std::size_t subid,
		std::size_t ic
	){
		std::vector< std::string > result;
		result.reserve(ic);
		for(std::size_t i = 0; i < ic; ++i){
			auto const filename = name(id, subid, i);
			module.log([&filename](logsys::stdlogb& os){
					os << filename;
				}, [&filename, &result]{
					std::ifstream is(filename.c_str(),
						std::ios::in | std::ios::binary);
					verify_stream(is, filename);
					result.emplace_back(
						std::istreambuf_iterator<char>(is),
						std::istreambuf_iterator<char>());
				});
		}
		return result;
	}

	template < typename Module >
	t3 load(
		Module module,
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
				module.log([&filename](logsys::stdlogb& os){
						os << filename;
					}, [&filename, &result]{
						std::ifstream is(filename.c_str(),
							std::ios::in | std::ios::binary);
						verify_stream(is, filename);
						result.back().emplace_back(
							std::istreambuf_iterator<char>(is),
							std::istreambuf_iterator<char>());
				});
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

	constexpr std::array< std::string_view, 3 > list{{
			"file",
			"file_list",
			"file_list_list"
		}};

	constexpr auto dim = dimension_c<
			std::string,
			std::vector< std::string >,
			std::vector< std::vector< std::string > >
		>;

	std::string format_description(){
		static_assert(dim.type_count == list.size());
		std::ostringstream os;
		std::size_t i = 0;
		hana::for_each(dim.types, [&os, &i](auto t){
				os << "\n* " << list[i] << " => "
					<< ct_pretty_name< typename decltype(t)::type >();
				++i;
			});
		return os.str();
	}


	void init(std::string const& name, declarant& disposer){
		auto expect_greater_0 =
			verify_value_fn([](std::size_t value){
				if(value == 0){
					throw std::logic_error("must be greater 0");
				}
			});

		auto init = generate_module(
			"load files from hard disk, the filenames are generated by "
			"parameters and runtime variables, see parameter name for details",
			dimension_list{
				dim
			},
			module_configure(
				make("type"_param, free_type_c< std::size_t >,
					"set dimension 1 by value:" + format_description(),
					parser_fn([](std::string_view data){
						auto iter = std::find(list.begin(), list.end(), data);
						if(iter == list.end()){
							throw std::runtime_error("unknown value '"
								+ std::string(data)
								+ "', valid values are: "
								+ io_tools::range_to_string(list));
						}
						return iter - list.begin();
					}),
					default_value(0)),
				make("fixed_id"_param,
					free_type_c< std::optional< std::size_t > >,
					"used instead of the exec ID if set"),
				make("id_modulo"_param,
					free_type_c< std::optional< std::size_t > >,
					"ID is exec ID modulo id_modulo if set "
					"(id_add is later added)"),
				make("id_digits"_param, free_type_c< std::size_t >,
					"minimal digit count via leading zeros",
					default_value(4)),
				make("subid_digits"_param, free_type_c< std::size_t >,
					"minimal digit count via leading zeros",
					default_value(1)),
				make("id_add"_param, free_type_c< std::size_t >,
					"value is added to ID",
					default_value(0)),
				make("subid_add"_param, free_type_c< std::size_t >,
					"value is added to sub ID",
					default_value(0)),
				make("subid_count"_param, free_type_c< std::size_t >,
					"sub ID count, see parameter name for details",
					default_value(1),
					expect_greater_0),
				set_dimension_fn([](auto const module){
					std::size_t const number = module("type"_param);
					return solved_dimensions{index_component< 0 >{number}};
				}),
				make("i_add"_param, wrapped_type_ref_c< i_type, 0 >,
					"value is added to i",
					default_value(0)),
				make("j_add"_param, wrapped_type_ref_c< j_type, 0 >,
					"value is added to j",
					default_value(0)),
				make("i_digits"_param, wrapped_type_ref_c< i_type, 0 >,
					"minimal digit count via leading zeros",
					default_value(2)),
				make("j_digits"_param, wrapped_type_ref_c< j_type, 0 >,
					"minimal digit count via leading zeros",
					default_value(2)),
				make("i_count"_param, wrapped_type_ref_c< i_type, 0 >,
					"i count, see parameter name for details",
					expect_greater_0),
				make("j_count"_param, wrapped_type_ref_c< j_type, 0 >,
					"j count, see parameter name for details",
					expect_greater_0),
				make("content"_out, type_ref_c< 0 >,
					"the loaded data"),
				make("name"_param,
					wrapped_type_ref_c< to_name_generator_t, 0 >,
					"pattern to generate file names, "
					"(the path is treated as part of the file name)\n"
					"you can use some variables via ${variable}\n"
					"* ${id} is the exec ID\n"
					"* ${subid} is the sub ID, in the save module it can "
					"happen that multiple data is processed with the same "
					"exec ID, these runs are numbered with the sub ID\n"
					"* ${i} does only exist if type is file_list or "
					"file_list_list, it numbers the (outer) vector\n"
					"* ${j} does only exist if type is file_list_list, it "
					"numbers the inner vector",
					parser_fn([](
						std::string_view data,
						auto const type,
						auto const module
					){
						if constexpr(type == type_c< ng1 >){
							return make_name_generator(
								std::string(data),
								{true, false},
								std::make_pair("id"s,
									format{module("id_digits"_param),
										module("id_add"_param)}),
								std::make_pair("subid"s,
									format{module("subid_digits"_param),
										module("subid_add"_param)})
							);
						}else if constexpr(type == type_c< ng2 >){
							return make_name_generator(
								std::string(data),
								{true, false, true},
								std::make_pair("id"s,
									format{module("id_digits"_param),
										module("id_add"_param)}),
								std::make_pair("subid"s,
									format{module("subid_digits"_param),
										module("subid_add"_param)}),
								std::make_pair("i"s,
									format{module("i_digits"_param),
										module("i_add"_param)})
							);
						}else{
							return make_name_generator(
								std::string(data),
								{true, false, true, true},
								std::make_pair("id"s,
									format{module("id_digits"_param),
										module("id_add"_param)}),
								std::make_pair("subid"s,
									format{module("subid_digits"_param),
										module("subid_add"_param)}),
								std::make_pair("i"s,
									format{module("i_digits"_param),
										module("i_add"_param)}),
								std::make_pair("j"s,
									format{module("j_digits"_param),
										module("j_add"_param)})
							);
						}
					})
				)
			),
			exec_fn([](auto module){
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
						out.push(load(module, module("name"_param), id, subid));
					}else if constexpr(std::is_same_v< type, t2 >){
						out.push(load(module, module("name"_param), id, subid,
							module("i_count"_param)));
					}else if constexpr(std::is_same_v< type, t3 >){
						out.push(load(module, module("name"_param), id, subid,
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
