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


	constexpr auto types = hana::tuple_t<
			std::string,
			std::vector< std::string >,
			std::vector< std::vector< std::string > >
		>;

	using ng1 = name_generator< std::size_t >;
	using ng2 = name_generator< std::size_t, std::size_t >;
	using ng3 = name_generator< std::size_t, std::size_t, std::size_t >;

	constexpr auto name_generator_types = hana::tuple_t< ng1, ng2, ng3 >;

	constexpr auto type_to_name_generator = hana::unpack(hana::zip_with(
		hana::make_pair, types, name_generator_types), hana::make_map);


	enum class data_type{
		file,
		list,
		list_list
	};


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
		std::size_t digits = 2;

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
				"type"_param(hana::type_c< data_type >,
					parser([](auto const& /*iop*/, std::string_view data,
						hana::basic_type< data_type >
					){
						if(data == "file") return data_type::file;
						if(data == "list") return data_type::list;
						if(data == "list_list") return data_type::list_list;
						throw std::runtime_error("unknown value '"
							+ std::string(data) + "', allowed values are: "
							"file, list & list_list");
					}),
					default_values(data_type::file),
					type_as_text(
						hana::make_pair(hana::type_c< data_type >, "type"_s)
					),
					value_verify([](auto const& iop, data_type type){
						auto const& in = iop("content"_in);
						auto file_on =
							in.is_enabled(hana::type_c< std::string >);
						auto list1_on =
							in.is_enabled(hana::type_c<
								std::vector< std::string > >);
						auto list2_on =
							in.is_enabled(hana::type_c<
								std::vector< std::vector< std::string > > >);
						switch(type){
							case data_type::file:
								if(!file_on) throw std::runtime_error(
									"parameter is file, but input content "
									"type std::string is not enabled");
								if(!list1_on && !list2_on) return;
							break;
							case data_type::list:
								if(!list1_on) throw std::runtime_error(
									"parameter is list, but input content "
									"type std::vector< std::string > is not "
									"enabled");
								if(!file_on && !list2_on) return;
							break;
							case data_type::list_list:
								if(!list2_on) throw std::runtime_error(
									"parameter type is list_list, but input "
									"content type "
									"std::vector< std::vector< std::string > > "
									"is not enabled");
								if(!file_on && !list1_on) return;
							break;
						}
						throw std::runtime_error("more than one type of input "
							"content is enabled");
					})
				),
				"id_digits"_param(hana::type_c< std::size_t >,
					default_values(std::size_t(4))),
				"i_digits"_param(hana::type_c< std::size_t >,
					default_values(std::size_t(2))),
				"j_digits"_param(hana::type_c< std::size_t >,
					default_values(std::size_t(2))),
				"name"_param(name_generator_types,
					enable(
						[](auto const& iop, auto type){
							auto ng1_parser = type == hana::type_c< ng1 >;
							auto ng2_parser = type == hana::type_c< ng2 >;
							(void)ng2_parser; // silence GCC
							if constexpr(ng1_parser){
								return iop("content"_in).is_enabled(
									hana::type_c< std::string >);
							}else if constexpr(ng2_parser){
								return iop("content"_in).is_enabled(
									hana::type_c< std::vector< std::string > >);
							}else{
								return iop("content"_in).is_enabled(
									hana::type_c< std::vector<
										std::vector< std::string > > >
								);
							}
						}),
					parser(
						[](auto const& iop, std::string_view data, auto type){
							auto ng1_parser = type == hana::type_c< ng1 >;
							auto ng2_parser = type == hana::type_c< ng2 >;
							(void)ng2_parser; // silence GCC
							if constexpr(ng1_parser){
								return make_name_generator(
									data,
									std::make_pair("id",
										format{iop("id_digits"_param).get()})
								);
							}else if constexpr(ng2_parser){
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
						hana::make_pair(hana::type_c< name_generator<
							std::size_t > >, "file"_s),
						hana::make_pair(hana::type_c< name_generator<
							std::size_t, std::size_t > >, "list"_s),
						hana::make_pair(hana::type_c< name_generator<
							std::size_t, std::size_t, std::size_t > >,
								"list_list"_s)
					)
				)
			),
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
