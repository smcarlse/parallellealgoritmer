
#include "bsp.hpp"

#include <iostream>

class Hello_World: public mcbsp::BSP_program {

	protected:

		virtual void spmd( void );

		virtual BSP_program * newInstance();

	public:

		Hello_World();
};

void Hello_World::spmd() {
	std::cout << "Hello world from thread " << bsp_pid() << "\n";
}

mcbsp::BSP_program * Hello_World::newInstance() {
	return new Hello_World();
}

Hello_World::Hello_World() {}

int main() {
	Hello_World p;
	p.begin( 2 );
	std::cout << "In-between parallel part..." << std::endl;
	p.begin();
	return 0;
}

