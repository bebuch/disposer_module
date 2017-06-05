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

	using ng1 = name_generator< std::size_t >;
	using ng2 = name_generator< std::size_t, std::size_t >;
	using ng3 = name_generator< std::size_t, std::size_t, std::size_t >;

	constexpr auto name_generator_types = hana::tuple_t< ng1, ng2, ng3 >;

	constexpr auto type_to_name_generator = hana::unpack(hana::zip_with(
		hana::make_pair, types, name_generator_types), hana::make_map);


	void save(
		std::size_t id,
		ng1 const& name,
		std::string const& data
	){
		std::ofstream os(name(id).c_str(),
			std::ios::out | std::ios::binary);
		os.write(data.data(), data.size());
	}

	void save(
		std::size_t id,
		ng2 const& name,
		std::vector< std::string > const& data
	){
		for(std::size_t i = 0; i < data.size(); ++i){
			std::ofstream os(name(id, i).c_str(),
				std::ios::out | std::ios::binary);
			os.write(data[i].data(), data.size());
		}
	}

	void save(
		std::size_t id,
		ng3 const& name,
		std::vector< std::vector< std::string > > const& data
	){
		for(std::size_t i = 0; i < data.size(); ++i){
			for(std::size_t j = 0; j < data.size(); ++j){
				std::ofstream os(name(id, i, j).c_str(),
					std::ios::out | std::ios::binary);
				os.write(data[i][j].data(), data.size());
			}
		}
	}


	struct format{
		std::size_t const digits;

		std::string operator()(std::size_t value){
			std::ostringstream os;
			os << std::setw(digits) << std::setfill('0') << value;
			return os.str();
		}
	};


	void init(std::string const& name, module_declarant& disposer){
		auto init = make_register_fn(
			configure(
				"content"_in(types, required),
				"id_digits"_param(type_c< std::size_t >,
					default_values(std::size_t(4))),
				"i_digits"_param(type_c< std::size_t >,
					default_values(std::size_t(2))),
				"j_digits"_param(type_c< std::size_t >,
					default_values(std::size_t(2))),
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
				)
			),
			normal_id_increase(),
			[](auto const& /*module*/){
				return [](auto& module, std::size_t /*id*/){
					auto values = module("content"_in).get_references();
					for(auto const& pair: values){
						auto id = pair.first;
						std::visit(
							[&module, id](auto const& data_ref){
								save(
									id,
									module("name"_param).get(
										type_to_name_generator[
											hana::typeid_(data_ref.get())]),
									data_ref.get()
								);
							},
							pair.second);
					}
				};
			}
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
