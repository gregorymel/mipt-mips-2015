/*
 * main.cpp - mips performance simulator
 * @author Grigory Melnikov grigoriy.melnikov@phystech.edu
 * Copyright 2016 MIPT-MIPS 
 */

#include <iostream>
#include <cstdlib>

#include <perf_sim.h>

int main( int argc, char* argv[])
{
    if ( argc < 3)
    {
        std::cout << "2 arguments required: mips_exe filename and amount of instrs to run" << endl;
        std::exit(EXIT_FAILURE);
    }
    
    bool silent = false;
	if ( argc == 4)
	{
		std::string arg = std::string( argv[ 1]);
		if ( arg == "-d")
			silent = true;
	}
	
    PerfMIPS* mips = new PerfMIPS();
    mips->run(std::string(argv[2]), atoi(argv[3]), silent);
    delete mips;

    return 0;
}
