//Generic C
#include <cstring>
#include <cassert>

//Generic C++
#include <iostream>
#include <vector>

//uArchSim modules
#include <elf_parser.h>
#include <func_memory.h>
#include <func_instr.h>

#define num_args 3

using namespace std;

int main( int argc, char* argv[])
{
	if ( argc > num_args)
	{
		cerr << "ERROR. Too many arguments" << endl;
		return -1;
	} else if ( argc < num_args)
	{
		cerr << "ERROR. Not enough arguments" << endl;
		return -1;
	}
	
	const char* section_name = argv[ 2]; 
	const char* file_name = argv[ 1];
	assert( file_name);
	
	std::vector<ElfSection> sections_array;
    ElfSection::getAllElfSections( file_name, sections_array);

	uint64 start_addr = 0, section_size = 0;
	for ( vector<ElfSection>::iterator it = sections_array.begin(); it != sections_array.end(); ++it)
    {
        if ( !strcmp( ".text", it->name))
		{
			start_addr = it->start_addr;
			section_size = it->size;
			break;
		}
    }
    	
    FuncMemory func_mem( file_name, 32, 10, 12);
  
    for ( unsigned long i = 0; i < section_size / 4; ++i)
    {
        uint32 word = ( uint32)func_mem.read( start_addr + i * 4, 4);
        FuncInstr instr( word);
        cout << instr.Dump("    ") << endl;
    }
  
    return 0;
}