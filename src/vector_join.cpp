#include <disposer/module.hpp>

#include <bitmap/bitmap.hpp>

#include <boost/dll.hpp>


namespace disposer_module::vector_join{


	using namespace disposer;
	using namespace disposer::literals;
	namespace hana = boost::hana;


	template < typename T >
	using bitmap = ::bmp::bitmap< T >;


	void init(std::string const& name, module_declarant& disposer){
		auto init = generate_module(
			dimension_list{
				dimension_c<
					std::string,
					bitmap< std::int8_t >,
					bitmap< std::int16_t >,
					bitmap< std::int32_t >,
					bitmap< std::int64_t >,
					bitmap< std::uint8_t >,
					bitmap< std::uint16_t >,
					bitmap< std::uint32_t >,
					bitmap< std::uint64_t >,
					bitmap< float >,
					bitmap< double >
				>
			},
			module_configure(
				make("data"_in, type_ref_c< 0 >),
				make("list"_out, wrapped_type_ref_c< std::vector, 0 >),
				make("count"_param, free_type_c< std::size_t >,
					verify_value_fn([](auto const& value){
						if(value > 0) return;
						throw std::logic_error("must be greater 0");
					}))
			),
			module_init_fn([](auto module){
				using type = typename
					decltype(module.dimension(hana::size_c< 0 >))::type;

				std::vector< type > result{};
				result.reserve(module("count"_param));
				return result;
			}),
			exec_fn([](auto module){
				auto& list = module.state();
				for(auto&& data: module("data"_in).values()){
					list.push_back(std::move(data));
					if(list.size() == module("count"_param)){
						module("list"_out).push(std::move(list));
						list.clear();
					}
				}
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
