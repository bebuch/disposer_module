//-----------------------------------------------------------------------------
// Copyright (c) 2015-2017 Benjamin Buch
//
// https://github.com/bebuch/disposer_module
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "bitmap.hpp"

#include <disposer/module.hpp>

#include <boost/dll.hpp>

#include <cstdint>


namespace disposer_module{ namespace phase_unwrapp_by_index{


	using phase_type_list = disposer::type_list<
		float,
		double
	>;

	using index_type_list = disposer::type_list<
		std::uint8_t,
		std::uint16_t,
		std::uint32_t
	>;


	struct module: disposer::module_base{
		module(disposer::make_data const& data):
			disposer::module_base(
				data,
				{slots.raw_phase_image, slots.index_image},
				{signals.fine_phase_image}
			)
			{}


		template < typename PhaseT, typename IndexT >
		void unwrapp(bitmap< PhaseT > phase, bitmap< IndexT > const& index);


		struct{
			disposer::container_input< bitmap, phase_type_list >
				raw_phase_image{"raw_phase_image"};

			disposer::container_input< bitmap, index_type_list >
				index_image{"index_image"};
		} slots;

		struct{
			disposer::container_output< bitmap, phase_type_list >
				fine_phase_image{"fine_phase_image"};
		} signals;


		void exec()override;


		void input_ready()override{
			signals.fine_phase_image.enable_types(
				slots.raw_phase_image.enabled_types());
		}
	};


	disposer::module_ptr make_module(disposer::make_data& data){
		if(data.is_first()) throw disposer::module_not_as_start(data);

		return std::make_unique< module >(data);
	}


	template < typename PhaseT, typename IndexT >
	void module::unwrapp(bitmap< PhaseT > phase, bitmap< IndexT > const& index){
		std::transform(
			phase.begin(), phase.end(), index.begin(), phase.begin(),
			[](PhaseT p, IndexT i){
				p = std::abs(p);
				return (2 * M_PI) * (i / 2) + (i % 2 == 0 ? -p : p);
			});

		signals.fine_phase_image.put< PhaseT >(std::move(phase));
	}


	void module::exec(){
		// get all the data
		auto raw_phase_data = slots.raw_phase_image.get();
		auto index_data = slots.index_image.get();

		// check count of puts in previous modules
		if(raw_phase_data.size() != index_data.size()){
			std::ostringstream os;
			os << "different count of input data ("
				<< slots.raw_phase_image.name << ": " << raw_phase_data.size()
				<< ", "
				<< slots.index_image.name << ": " << index_data.size()
				<< ")";

			throw std::logic_error(os.str());
		}

		// combine data in a single variant type
		auto index_iter = index_data.begin();
		for(
			auto raw_phase_iter = raw_phase_data.begin();
			raw_phase_iter != raw_phase_data.end();
			++raw_phase_iter, ++index_iter
		){
			auto&& [raw_phase_id, raw_phase_img] = *raw_phase_iter;
			auto&& [index_id, index_img] = *index_iter;

			// a very excotic error that should only appeare
			// if a previous module did set output.put() id's wrong
			if(raw_phase_id != index_id){
				throw std::logic_error("id mismatch");
			}

			std::visit([this](auto const& phase_image, auto const& index_image){
					unwrapp(phase_image.data(), index_image.data());
				}, raw_phase_img, index_img);
		}
	}


	void init(disposer::module_declarant& add){
		add("phase_unwrapp_by_index", &make_module);
	}

	BOOST_DLL_AUTO_ALIAS(init)


} }
