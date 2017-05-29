#include <disposer/module.hpp>

#include <bitmap/bitmap.hpp>

#include <boost/dll.hpp>


namespace disposer_module::vector_join{


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

	constexpr auto to_bitmap_vector =
		type_transform([](auto type)noexcept{
			return hana::type_c< std::vector< bitmap::bitmap<
				typename decltype(type)::type > > >;
		});

	struct exec{
		using type = decltype(hana::unpack(
			hana::transform(types, to_bitmap_vector),
			hana::template_< std::variant >))::type;

		type vector;

		template < typename Module, typename Image >
		void add_image(Module& module, std::vector< Image >& out, Image&& in){
			out.push_back(std::move(in));
			if(out.size() == module("count"_param).get()){
				module("vector"_out).put(std::move(out));
				out.clear();
				return;
			}
		}

		template < typename Module >
		void operator()(Module& module, std::size_t /*id*/){
			auto values = module("image"_in).get_values();
			for(auto&& pair: values){
				auto&& image = std::move(pair.second);
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
						auto& new_out = vector.emplace< new_type >();
						new_out.reserve(module("count"_param).get());
						add_image(module, new_out, std::move(in));
					}
				}, vector, std::move(image));
			}
		}
	};


	void init(std::string const& name, module_declarant& disposer){
		auto init = make_register_fn(
			configure(
				"image"_in(types,
					template_transform_c< bitmap::bitmap >,
					required),
				"vector"_out(types,
					to_bitmap_vector,
					enable_by_types_of("image"_in)
				),
				"count"_param(hana::type_c< std::size_t >,
					value_verify([](auto const& /*iop*/, auto const& value){
						if(value > 0) return;
						throw std::logic_error("must be greater 0");
					})
				)
			),
			[](auto const& /*module*/){
				return exec{};
			}
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
