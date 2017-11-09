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

#include <io_tools/time_to_dir_string.hpp>

#include <boost/filesystem.hpp>

#include <boost/dll.hpp>

#include <fstream>


namespace disposer_module::save{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;
	using namespace hana::literals;
	using namespace std::literals::string_literals;
	using hana::type_c;
	namespace filesystem = boost::filesystem;


	using t1 = std::string;
	using t2 = std::vector< std::string >;
	using t3 = std::vector< std::vector< std::string > >;

	using ng1 = name_generator< std::string, std::size_t, std::size_t >;
	using ng2 = name_generator< std::string, std::size_t, std::size_t,
		std::size_t >;
	using ng3 = name_generator< std::string, std::size_t, std::size_t,
		std::size_t, std::size_t >;


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


	void verify_open(std::ofstream& os, std::string const& filename){
		if(os) return;
		throw std::runtime_error("Can not open file '" + filename
			+ "' for write");
	}

	void verify_write(std::ofstream& os, std::string const& filename){
		if(os) return;
		throw std::runtime_error("Can not write to file '" + filename + "'");
	}

	template < typename Module >
	void save(
		Module const& module,
		std::string const& date_time,
		std::size_t id,
		std::size_t subid,
		ng1 const& name,
		std::string const& data
	){
		auto const filename = name(date_time, id, subid);

		module.log([&filename](logsys::stdlogb& os){
				os << "load: " << filename;
			}, [&filename, &data]{
				filesystem::create_directories(
					filesystem::path(filename).remove_filename());

				std::ofstream os(filename.c_str(),
					std::ios::out | std::ios::binary);
				verify_open(os, filename);
				os.write(data.data(), data.size());
				verify_write(os, filename);
			});
	}

	template < typename Module >
	void save(
		Module const& module,
		std::string const& date_time,
		std::size_t id,
		std::size_t subid,
		ng2 const& name,
		std::vector< std::string > const& data
	){
		for(std::size_t i = 0; i < data.size(); ++i){
			auto const filename = name(date_time, id, subid, i);

			module.log([&filename](logsys::stdlogb& os){
					os << "load: " << filename;
				}, [&filename, &data, i]{
					filesystem::create_directories(
						filesystem::path(filename).remove_filename());

					std::ofstream os(filename.c_str(),
						std::ios::out | std::ios::binary);
					verify_open(os, filename);
					os.write(data[i].data(), data[i].size());
					verify_write(os, filename);
				});
		}
	}

	template < typename Module >
	void save(
		Module const& module,
		std::string const& date_time,
		std::size_t id,
		std::size_t subid,
		ng3 const& name,
		std::vector< std::vector< std::string > > const& data
	){
		for(std::size_t i = 0; i < data.size(); ++i){
			for(std::size_t j = 0; j < data.size(); ++j){
				auto const filename = name(date_time, id, subid, i, j);

				module.log([&filename](logsys::stdlogb& os){
						os << "load: " << filename;
					}, [&filename, &data, i, j]{
						filesystem::create_directories(
							filesystem::path(filename).remove_filename());

						std::ofstream os(filename.c_str(),
							std::ios::out | std::ios::binary);
						verify_open(os, filename);
						os.write(data[i][j].data(), data[i][j].size());
						verify_write(os, filename);
					});
			}
		}
	}


	struct nothing{
		std::string operator()(std::string const& value){
			return value;
		}
	};

	struct format{
		std::size_t const digits;
		std::size_t const add;

		std::string operator()(std::size_t value){
			std::ostringstream os;
			os << std::setw(digits) << std::setfill('0') << add + value;
			return os.str();
		}
	};

	struct state{
		std::string date_time;
	};


	void init(std::string const& name, module_declarant& disposer){
		auto init = module_register_fn(
			dimension_list{
				dimension_c<
					std::string,
					std::vector< std::string >,
					std::vector< std::vector< std::string > >
				>
			},
			module_configure(
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
				make("content"_in, type_ref_c< 0 >),
				make("i_digits"_param, wrapped_type_ref_c< i_type, 0 >,
					default_value(2)),
				make("j_digits"_param, wrapped_type_ref_c< j_type, 0 >,
					default_value(2)),
				make("i_add"_param, wrapped_type_ref_c< i_type, 0 >,
					default_value(0)),
				make("j_add"_param, wrapped_type_ref_c< j_type, 0 >,
					default_value(0)),
				make("name"_param, wrapped_type_ref_c< to_name_generator_t, 0 >,
					parser_fn([](
						auto const& iop, std::string_view data, auto type
					){
						if constexpr(type == type_c< ng1 >){
							return make_name_generator(
								data,
								{false, true, false},
								std::make_pair("date_time"s, nothing{}),
								std::make_pair("id"s,
									format{iop("id_digits"_param),
										iop("id_add"_param)}),
								std::make_pair("subid"s,
									format{iop("subid_digits"_param),
										iop("subid_add"_param)})
							);
						}else if constexpr(type == type_c< ng2 >){
							return make_name_generator(
								data,
								{false, true, false, true},
								std::make_pair("date_time"s, nothing{}),
								std::make_pair("id"s,
									format{iop("id_digits"_param),
										iop("id_add"_param)}),
								std::make_pair("subid"s,
									format{iop("subid_digits"_param),
										iop("subid_add"_param)}),
								std::make_pair("i"s,
									format{iop("i_digits"_param),
										iop("i_add"_param)})
							);
						}else{
							return make_name_generator(
								data,
								{false, true, false, true, true},
								std::make_pair("date_time"s, nothing{}),
								std::make_pair("id"s,
									format{iop("id_digits"_param),
										iop("id_add"_param)}),
								std::make_pair("subid"s,
									format{iop("subid_digits"_param),
										iop("subid_add"_param)}),
								std::make_pair("i"s,
									format{iop("i_digits"_param),
										iop("i_add"_param)}),
								std::make_pair("j"s,
									format{iop("j_digits"_param),
										iop("j_add"_param)})
							);
						}
					})
				)
			),
			module_init_fn([](auto const& module){
				return state{io_tools::time_to_dir_string(
					std::chrono::system_clock::now())};
			}),
			exec_fn([](auto& module){
				std::size_t subid = 0;
				for(auto const& img: module("content"_in).references()){
					auto const fixed_id = module("fixed_id"_param);
					auto id = fixed_id ? *fixed_id : module.id();

					auto const id_modulo = module("id_modulo"_param);
					if(id_modulo) id %= *id_modulo;

					save(module, module.state().date_time, id, subid,
						module("name"_param), img);

					++subid;
				}
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
