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
			hana::transform(base_types, hana::template_< ::bmp::bitmap >)
		);


	void init(std::string const& name, module_declarant& disposer){
		auto init = module_register_fn(
			module_configure(
				"list"_in(types, wrap_in< vector >),
				"data"_out(types, enable_by_types_of("list"_in)),
				"count"_param(hana::type_c< std::size_t >,
					value_verify_fn([](auto const& /*iop*/, auto const& value){
						if(value > 0) return;
						throw std::logic_error("must be greater 0");
					})
				)
			),
			module_enable([]{
				return [](auto& module){
					auto values = module("list"_in).get_values();
					for(auto&& value: values){
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
						}, std::move(value));
					}
				};
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
