//-----------------------------------------------------------------------------
// Copyright (c) 2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "bitmap_vector.hpp"

#include <bitmap/pixel.hpp>

#include <disposer/module.hpp>
#include <disposer/type_name.hpp>

#include <boost/spirit/home/x3.hpp>
#include <boost/hana.hpp>
#include <boost/dll.hpp>

#include <cstdint>
#include <limits>



namespace{


	template < typename T >
	auto get_parser(){
		static_assert(
			std::is_floating_point_v< T > ||
			std::is_unsigned_v< T > ||
			std::is_signed_v< T >
		);

		namespace x3 = boost::spirit::x3;

		if constexpr(std::is_floating_point_v< T >){
			return x3::double_;
		}else if constexpr(std::is_unsigned_v< T >){
			return x3::uint64;
		}else{
			return x3::int64;
		}
	}


	template < typename T, typename Value >
	T protected_cast(Value v){
		if constexpr(
			!std::is_floating_point_v< T > && !std::is_same_v< T, Value >
		){
			if(
				std::numeric_limits< T >::min() > v ||
				std::numeric_limits< T >::max() < v
			){
				std::ostringstream os;
				os << "value '" << v << "' does not match in type [" <<
					disposer::type_name< T >() << "]";
				throw std::out_of_range(os.str());
			}
		}

		return static_cast< T >(v);
	}


}


template < typename T >
struct disposer::parameter_cast< ::bitmap::pixel::basic_ga< T > >{
	::bitmap::pixel::basic_ga< T > operator()(std::string const& value)const{
		namespace x3 = boost::spirit::x3;

		::bitmap::pixel::basic_ga< T > px;
		auto g = [&px](auto& ctx){ px.g = protected_cast< T >(_attr(ctx)); };
		auto a = [&px](auto& ctx){ px.a = protected_cast< T >(_attr(ctx)); };

		auto vparser = get_parser< T >();

		auto first = value.begin();
		auto const last = value.end();
		bool const match = x3::phrase_parse(first, last,
			(x3::lit('{')
				>> vparser[g] >> x3::lit(',')
				>> vparser[a] >> x3::lit('}')),
			x3::space);

		if(!match || first != last){
			throw std::logic_error(
				"expected '{gray, alpha}' but got '" + value + "'");
		}

		return px;
	}
};

template < typename T >
struct disposer::parameter_cast< ::bitmap::pixel::basic_rgb< T > >{
	::bitmap::pixel::basic_rgb< T > operator()(std::string const& value)const{
		namespace x3 = boost::spirit::x3;

		::bitmap::pixel::basic_rgb< T > px;
		auto r = [&px](auto& ctx){ px.r = protected_cast< T >(_attr(ctx)); };
		auto g = [&px](auto& ctx){ px.g = protected_cast< T >(_attr(ctx)); };
		auto b = [&px](auto& ctx){ px.b = protected_cast< T >(_attr(ctx)); };

		auto vparser = get_parser< T >();

		auto first = value.begin();
		auto const last = value.end();
		bool const match = x3::phrase_parse(first, last,
			(x3::lit('{')
				>> vparser[r] >> x3::lit(',')
				>> vparser[g] >> x3::lit(',')
				>> vparser[b] >> x3::lit('}')),
			x3::space);

		if(!match || first != last){
			throw std::logic_error(
				"expected '{red, green, blue}' but got '" + value + "'");
		}

		return px;
	}
};

template < typename T >
struct disposer::parameter_cast< ::bitmap::pixel::basic_rgba< T > >{
	::bitmap::pixel::basic_rgba< T > operator()(std::string const& value)const{
		namespace x3 = boost::spirit::x3;

		::bitmap::pixel::basic_rgba< T > px;
		auto r = [&px](auto& ctx){ px.r = protected_cast< T >(_attr(ctx)); };
		auto g = [&px](auto& ctx){ px.g = protected_cast< T >(_attr(ctx)); };
		auto b = [&px](auto& ctx){ px.b = protected_cast< T >(_attr(ctx)); };
		auto a = [&px](auto& ctx){ px.a = protected_cast< T >(_attr(ctx)); };

		auto vparser = get_parser< T >();

		auto first = value.begin();
		auto const last = value.end();
		bool const match = x3::phrase_parse(first, last,
			(x3::lit('{')
				>> vparser[r] >> x3::lit(',')
				>> vparser[g] >> x3::lit(',')
				>> vparser[b] >> x3::lit(',')
				>> vparser[a] >> x3::lit('}')),
			x3::space);

		if(!match || first != last){
			throw std::logic_error(
				"expected '{red, green, blue, alpha}' but got '" + value + "'");
		}

		return px;
	}
};


namespace disposer_module{ namespace bitmap_vector_join{


	namespace hana = boost::hana;
	namespace pixel = ::bitmap::pixel;


	using disposer::type_position_v;

	using type_list = disposer::type_list<
		std::int8_t,
		std::uint8_t,
		std::int16_t,
		std::uint16_t,
		std::int32_t,
		std::uint32_t,
		std::int64_t,
		std::uint64_t,
		float,
		double,
		long double,
		pixel::ga8,
		pixel::ga16,
		pixel::ga32,
		pixel::ga64,
		pixel::ga8u,
		pixel::ga16u,
		pixel::ga32u,
		pixel::ga64u,
		pixel::ga32f,
		pixel::ga64f,
		pixel::rgb8,
		pixel::rgb16,
		pixel::rgb32,
		pixel::rgb64,
		pixel::rgb8u,
		pixel::rgb16u,
		pixel::rgb32u,
		pixel::rgb64u,
		pixel::rgb32f,
		pixel::rgb64f,
		pixel::rgba8,
		pixel::rgba16,
		pixel::rgba32,
		pixel::rgba64,
		pixel::rgba8u,
		pixel::rgba16u,
		pixel::rgba32u,
		pixel::rgba64u,
		pixel::rgba32f,
		pixel::rgba64f
	>;


	struct parameter{
		std::size_t images_per_line;

		disposer::type_unroll_t< std::tuple, type_list > default_value;
	};


	struct module: disposer::module_base{
		module(disposer::make_data const& data, parameter&& param):
			disposer::module_base(
				data,
				{slots.image_vector},
				{signals.image}
			),
			param(std::move(param))
			{}


		template < typename T >
		bitmap< T > bitmap_vector_join(bitmap_vector< T > const& vectors)const;


		struct{
			disposer::container_input< bitmap_vector, type_list >
				image_vector{"image_vector"};
		} slots;

		struct{
			disposer::container_output< bitmap, type_list >
				image{"image"};
		} signals;


		void exec()override;
		void input_ready()override;


		parameter const param;
	};


	disposer::module_ptr make_module(disposer::make_data& data){
		if(data.is_first()) throw disposer::module_not_as_start(data);

		parameter param;

		data.params.set(param.images_per_line, "images_per_line");
		if(param.images_per_line == 0){
			throw std::logic_error("parameter images_per_line == 0");
		}

		// for integral is NaN defined as 0
		hana::for_each(
			disposer::hana_type_list< type_list >,
			[&data, &param](auto type_t){
				using type = typename decltype(type_t)::type;
				data.params.set(
					std::get< type >(param.default_value),
					"default_value",
					[](){
						if constexpr(std::is_floating_point_v< type >){
							return std::numeric_limits< type >::quiet_NaN();
						}else{
							return type();
						}
					}()
				);
			}
		);

		return std::make_unique< module >(data, std::move(param));
	}


	template < typename T >
	bitmap< T > module::bitmap_vector_join(
		bitmap_vector< T > const& vectors
	)const{
		auto const ips = param.images_per_line;
		auto const& input_size = vectors[0].size();

		std::size_t width =
			std::min(ips, vectors.size()) * input_size.width();
		std::size_t height =
			((vectors.size() - 1) / ips + 1) * input_size.height();

		bitmap< T > result(
			width, height, std::get< T >(param.default_value)
		);

		// parallel version with thread_pool is slower
		for(std::size_t i = 0; i < vectors.size(); ++i){
			auto const vwidth = vectors[i].width();
			auto const x_offset = (i % ips) * input_size.width();
			auto const y_offset = (i / ips) * input_size.height();

			for(std::size_t y = 0; y < input_size.height(); ++y){
				auto const in_start = vectors[i].data() + (y * vwidth);
				auto const in_end = in_start + vwidth;
				auto const out_start =
					result.data() + (y_offset + y) * width + x_offset;

				std::copy(in_start, in_end, out_start);
			}
		}

		return result;
	}


	struct visitor{
		visitor(bitmap_vector_join::module& module, std::size_t id):
			module(module), id(id) {}


		template < typename T >
		void operator()(
			disposer::input_data< bitmap_vector< T > > const& vector
		){
			auto& vectors = vector.data();


			auto iter = vectors.cbegin();
			for(auto size = (iter++)->size(); iter != vectors.cend(); ++iter){
				if(size == iter->size()) continue;
				throw std::runtime_error(
					"bitmaps with differenz sizes");
			}

			module.signals.image.put< T >(module.bitmap_vector_join(vectors));
		}

		bitmap_vector_join::module& module;
		std::size_t const id;
	};

	void module::exec(){
		for(auto const& pair: slots.image_vector.get()){
			visitor visitor(*this, pair.first);
			std::visit(visitor, pair.second);
		}
	}

	void module::input_ready(){
		signals.image.activate_types(
			slots.image_vector.active_types_transformed(
				[](auto type){
					return hana::type_c< typename decltype(type)::type::value_type >;
				}
			)
		);
	}



	void init(disposer::module_adder& add){
		add("bitmap_vector_join", &make_module);
	}

	BOOST_DLL_AUTO_ALIAS(init)


} }
