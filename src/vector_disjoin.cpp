#include <disposer/module.hpp>

#include <bitmap/bitmap.hpp>

#include <boost/dll.hpp>


namespace disposer_module::vector_disjoin{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;


	constexpr auto types = hana::tuple_t<
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


	void init(std::string const& name, module_declarant& disposer){
		auto init = make_register_fn(
			configure(
				"vector"_in(types,
					type_transform([](auto type)noexcept{
						return hana::type_c< std::vector< bitmap::bitmap<
							typename decltype(type)::type > > >;
					}),
					required),
				"image"_out(types,
					template_transform_c< bitmap::bitmap >,
					enable_by_types_of("vector"_in)
				),
				"count"_param(hana::type_c< std::size_t >,
					value_verify([](auto const& /*iop*/, auto const& value){
						if(value > 0) return;
						throw std::logic_error("must be greater 0");
					})
				)
			),
			normal_id_increase(),
			[](auto const& /*module*/){
				return [](auto& module, std::size_t /*id*/){
					auto values = module("vector"_in).get_values();
					for(auto&& pair: values){
						std::visit([&module](auto&& vector){
							auto const count = module("count"_param).get();
							if(vector.size() != count){
								throw std::runtime_error("vector size is "
									"different from parameter count ("
									+ std::to_string(vector.size()) + " != "
									+ std::to_string(count) + ")");
							}

							for(auto&& image: std::move(vector)){
								module("image"_out).put(std::move(image));
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
