#include "init.hpp"

#include "name_generator.hpp"

#include <disposer/config.hpp>

#include <iostream>


int main(){
	try{
		disposer_module::init();

		auto chains = disposer::config::load("plan.ini");

		chains.at("serial").trigger();
		chains.at("parallel").trigger();
		chains.at("toTar").trigger();
	}catch(std::exception const& e){
		std::cout << "Exception: " << e.what() << std::endl;
	}catch(...){
		std::cout << "Unknown exception" << std::endl;
	}
}

