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


namespace disposer_module::save{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;
	using namespace hana::literals;
	using hana::type_c;


	using t1 = std::string;
	using t2 = std::vector< std::string >;
	using t3 = std::vector< std::vector< std::string > >;

	constexpr auto types = hana::tuple_t< t1, t2, t3 >;

	using ng1 =
		name_generator< std::size_t, std::size_t >;
	using ng2 =
		name_generator< std::size_t, std::size_t, std::size_t >;
	using ng3 =
		name_generator< std::size_t, std::size_t, std::size_t, std::size_t >;

	constexpr auto name_generator_types = hana::tuple_t< ng1, ng2, ng3 >;

	constexpr auto type_to_name_generator = hana::unpack(hana::zip_with(
		hana::make_pair, types, name_generator_types), hana::make_map);


	void verify_open(std::ofstream& os, std::string const& filename){
		if(os) return;
		throw std::runtime_error("Can not open file '" + filename
			+ "' for write");
	}

	void verify_write(std::ofstream& os, std::string const& filename){
		if(os) return;
		throw std::runtime_error("Can not write to file '" + filename + "'");
	}

	void save(
		std::size_t id,
		std::size_t subid,
		ng1 const& name,
		std::string const& data
	){
		auto const filename = name(id, subid);
		std::ofstream os(filename.c_str(),
			std::ios::out | std::ios::binary);
		verify_open(os, filename);
		os.write(data.data(), data.size());
		verify_write(os, filename);
	}

	void save(
		std::size_t id,
		std::size_t subid,
		ng2 const& name,
		std::vector< std::string > const& data
	){
		for(std::size_t i = 0; i < data.size(); ++i){
			auto const filename = name(id, subid, i);
			std::ofstream os(filename.c_str(),
				std::ios::out | std::ios::binary);
			verify_open(os, filename);
			os.write(data[i].data(), data[i].size());
			verify_write(os, filename);
		}
	}

	void save(
		std::size_t id,
		std::size_t subid,
		ng3 const& name,
		std::vector< std::vector< std::string > > const& data
	){
		for(std::size_t i = 0; i < data.size(); ++i){
			for(std::size_t j = 0; j < data.size(); ++j){
				auto const filename = name(id, subid, i, j);
				std::ofstream os(filename.c_str(),
					std::ios::out | std::ios::binary);
				verify_open(os, filename);
				os.write(data[i][j].data(), data[i][j].size());
				verify_write(os, filename);
			}
		}
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


	void init(std::string const& name, module_declarant& disposer){
		auto init = module_register_fn(
			module_configure(
				"content"_in(types),
				"fixed_id"_param(type_c< std::optional< std::size_t > >),
				"id_modulo"_param(type_c< std::optional< std::size_t > >),
				"id_digits"_param(type_c< std::size_t >,
					default_value([](auto const&, auto){ return 4; })),
				"subid_digits"_param(type_c< std::size_t >,
					default_value([](auto const&, auto){ return 1; })),
				"i_digits"_param(type_c< std::size_t >,
					default_value([](auto const&, auto){ return 2; })),
				"j_digits"_param(type_c< std::size_t >,
					default_value([](auto const&, auto){ return 2; })),
				"id_add"_param(type_c< std::size_t >,
					default_value([](auto const&, auto){ return 0; })),
				"subid_add"_param(type_c< std::size_t >,
					default_value([](auto const&, auto){ return 0; })),
				"i_add"_param(type_c< std::size_t >,
					default_value([](auto const&, auto){ return 0; })),
				"j_add"_param(type_c< std::size_t >,
					default_value([](auto const&, auto){ return 0; })),
				"name"_param(name_generator_types,
					enable([](auto const& iop, auto type){
						auto const& out = iop("content"_in);
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
									format{iop("id_digits"_param).get(),
										iop("id_add"_param).get()}),
								std::make_pair("subid",
									format{iop("subid_digits"_param).get(),
										iop("subid_add"_param).get()})
							);
						}else if constexpr(type == type_c< ng2 >){
							return make_name_generator(
								data,
								std::make_pair("id",
									format{iop("id_digits"_param).get(),
										iop("id_add"_param).get()}),
								std::make_pair("subid",
									format{iop("subid_digits"_param).get(),
										iop("subid_add"_param).get()}),
								std::make_pair("i",
									format{iop("i_digits"_param).get(),
										iop("i_add"_param).get()})
							);
						}else{
							return make_name_generator(
								data,
								std::make_pair("id",
									format{iop("id_digits"_param).get(),
										iop("id_add"_param).get()}),
								std::make_pair("subid",
									format{iop("subid_digits"_param).get(),
										iop("subid_add"_param).get()}),
								std::make_pair("i",
									format{iop("i_digits"_param).get(),
										iop("i_add"_param).get()}),
								std::make_pair("j",
									format{iop("j_digits"_param).get(),
										iop("j_add"_param).get()})
							);
						}
					}),
					type_as_text(
						hana::make_pair(type_c< ng1 >, "file"_s),
						hana::make_pair(type_c< ng2 >, "file_list"_s),
						hana::make_pair(type_c< ng3 >, "file_list_list"_s)
					)
				)
			),
			module_enable([]{
				return [](auto& module, std::size_t exec_id){
					auto values = module("content"_in).get_references();
					std::size_t subid = 0;
					for(auto const& value: values){
						auto const fixed_id = module("fixed_id"_param).get();
						auto id = fixed_id ? *fixed_id : exec_id;

						auto const id_modulo = module("id_modulo"_param).get();
						if(id_modulo) id %= *id_modulo;

						std::visit(
							[&module, id, subid](auto const& data_ref){
								save(
									id, subid,
									module("name"_param).get(
										type_to_name_generator[
											hana::typeid_(data_ref.get())]),
									data_ref.get()
								);
							},
							value);

						++subid;
					}
				};
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
