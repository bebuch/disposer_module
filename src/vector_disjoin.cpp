#include <disposer/module.hpp>

#include <bitmap/bitmap.hpp>

#include <boost/dll.hpp>


namespace disposer_module::vector_disjoin{


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
				make("count"_param, free_type_c< std::size_t >,
					verify_value_fn([](auto const& value){
						if(value > 0) return;
						throw std::logic_error("must be greater 0");
					})),
				make("list"_in, wrapped_type_ref_c< std::vector, 0 >),
				make("data"_out, type_ref_c< 0 >)
			),
			exec_fn([](auto module){
				auto const count = module("count"_param);
				for(auto&& list: module("list"_in).values()){
					if(list.size() != count){
						throw std::runtime_error("list size is "
							"different from parameter count ("
							+ std::to_string(list.size()) + " != "
							+ std::to_string(count) + ")");
					}

					for(auto&& data: std::move(list)){
						module("data"_out).push(std::move(data));
					}
				}
			})
		);

		init(name, disposer);
	}

	BOOST_DLL_AUTO_ALIAS(init)


}
