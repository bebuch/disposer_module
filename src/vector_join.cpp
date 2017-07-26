#include <disposer/module.hpp>

#include <bitmap/bitmap.hpp>

#include <boost/dll.hpp>


namespace disposer_module::vector_join{


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


	struct exec{
		using type = decltype(hana::unpack(
			hana::transform(types, hana::template_< std::vector >),
			hana::template_< std::variant >))::type;

		type list;

		template < typename Module, typename Data >
		void add_image(Module& module, std::vector< Data >& out, Data&& in){
			out.push_back(std::move(in));
			if(out.size() == module("count"_param).get()){
				module("list"_out).put(std::move(out));
				out.clear();
				return;
			}
		}

		template < typename Module >
		void operator()(Module& module, std::size_t /*id*/){
			auto values = module("data"_in).get_values();
			for(auto&& value: values){
				auto&& data = std::move(value);
				std::visit([&](auto& out, auto&& in){
					auto types_equal =
						hana::typeid_(out.front()) == hana::typeid_(in);
					if constexpr(types_equal){
						add_image(module, out, std::move(in));
					}else{
						if(!out.empty()){
							throw std::runtime_error(
								"type change before list complete");
						}

						using new_type = std::vector<
							typename decltype(hana::typeid_(in))::type >;
						auto& new_out = list.emplace< new_type >();
						new_out.reserve(module("count"_param).get());
						add_image(module, new_out, std::move(in));
					}
				}, list, std::move(data));
			}
		}
	};


	void init(std::string const& name, module_declarant& disposer){
		auto init = module_register_fn(
			module_configure(
				"data"_in(types),
				"list"_out(types, template_transform_c< vector >,
					enable_by_types_of("data"_in)
				),
				"count"_param(hana::type_c< std::size_t >,
					value_verify([](auto const& /*iop*/, auto const& value){
						if(value > 0) return;
						throw std::logic_error("must be greater 0");
					})
				)
			),
			module_enable([]{
				return exec{};
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
