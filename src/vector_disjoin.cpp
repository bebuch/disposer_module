#include <disposer/module.hpp>

#include <bitmap/bitmap.hpp>

#include <boost/dll.hpp>


namespace disposer_module::vector_disjoin{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;


	template < typename T >
	using vector = std::vector< T >;

	constexpr auto base_types = hana::tuple_t<
			std::int8_t,
			std::int16_t,
			std::int32_t,
			std::int64_t,
			std::uint8_t,
			std::uint16_t,
			std::uint32_t,
			std::uint64_t,
			float,
			double
		>;

	constexpr auto types = hana::concat(
			hana::tuple_t< std::string >,
			hana::transform(base_types, hana::template_< ::bitmap::bitmap >)
		);


	void init(std::string const& name, module_declarant& disposer){
		auto init = make_register_fn(
			configure(
				"list"_in(types,
					template_transform_c< vector >,
					required),
				"data"_out(types,
					enable_by_types_of("list"_in)
				),
				"count"_param(hana::type_c< std::size_t >,
					value_verify([](auto const& /*iop*/, auto const& value){
						if(value > 0) return;
						throw std::logic_error("must be greater 0");
					})
				)
			),
			[](auto const& module){
				return id_increase_t{module("count"_param).get(), 1};
			},
			[](auto const& /*module*/){
				return [](auto& module, std::size_t /*id*/){
					auto values = module("list"_in).get_values();
					for(auto&& pair: values){
						std::visit([&module](auto&& list){
							auto const count = module("count"_param).get();
							if(list.size() != count){
								throw std::runtime_error("list size is "
									"different from parameter count ("
									+ std::to_string(list.size()) + " != "
									+ std::to_string(count) + ")");
							}

							for(auto&& data: std::move(list)){
								module("data"_out).put(std::move(data));
							}
						}, std::move(pair.second));
					}
				};
			}
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
