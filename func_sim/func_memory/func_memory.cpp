/**
 * func_memory.cpp - the module implementing the concept of
 * programer-visible memory space accesing via memory address.
 * @author Alexander Titov <alexander.igorevich.titov@gmail.com>
 * Copyright 2012 uArchSim iLab project
 */

// Generic C
#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <cassert>
// Generic C++
#include <iostream>
#include <string>
#include <vector>
#include <sstream>

// uArchSim modules
#include <func_memory.h>


enum States
{
	RDWR,    // read or write byte
	CNG_PG,  // change page
	CNG_SET, // change set
	EXIT     // exit failure
};

/*************************************************************************************************/
uint64 getSetIdx( uint64 set_bits, uint64 addr, uint64 addr_size)
{
	uint64 set_idx = addr >> ( addr_size - set_bits);
	
	return set_idx;
}

uint64 getPageIdx( uint64 page_bits, uint64 offset_bits, uint64 addr)
{
	uint64 mask = ( 1 << page_bits) - 1;
	uint64 page_idx;
	
	page_idx = ( addr >> offset_bits) & mask;
	
	return page_idx;	
}

uint64 getOffsetIdx( uint64 offset_bits, uint64 addr)
{
	uint64 mask = ( 1 << offset_bits) - 1;

	return mask & addr;
}
/****************************************************************************************************/

FuncMemory::FuncMemory( const char* executable_file_name,
                        uint64 addr_size,
                        uint64 page_bits,
                        uint64 offset_bits)
{
	addr_bits = addr_size;
    page_num_size = page_bits;
    offset_size = offset_bits;
	sets_array = NULL;
	
    vector<ElfSection> sections_array;
	
	ElfSection::getAllElfSections( executable_file_name, sections_array);
	vector<ElfSection>::iterator section_itr = sections_array.begin();
	
	sets_array_size  = 1 << ( addr_size - page_bits - offset_bits);
	pages_array_size = 1 << page_bits;
	page_size        = 1 << offset_bits;
	
	//create array of pointers to arraies of page
	sets_array = new uint8**[sets_array_size]();
	
	while ( section_itr != sections_array.end())
	{
		uint64 start_addr = section_itr->start_addr;
		
		if ( strcmp( section_itr->name, ".text") == 0)
			text_start_addr = start_addr;
		
		for ( int i = 0; i < section_itr->size; i++)
		{
			uint64 set_idx = getSetIdx( addr_size - page_bits - offset_bits, // set_bits
										section_itr->start_addr + i,         // addr
										addr_size);							 // addr_size
			
			if ( sets_array[ set_idx] == NULL)
				//create array of pointers to pages
				sets_array[ set_idx] = new uint8*[pages_array_size]();
			
			uint64 page_idx = getPageIdx( page_bits,
										  offset_bits,
										  section_itr->start_addr + i);		// addr
										  
			if ( sets_array[ set_idx][ page_idx] == NULL)
				//create array of bytes
				sets_array[ set_idx][ page_idx] = new uint8[page_size]();
			
			uint64 offset_idx = getOffsetIdx( offset_bits, section_itr->start_addr + i);
			sets_array[ set_idx][ page_idx][ offset_idx] = ( section_itr->content)[ i];
			
		}
		section_itr++;
	}
}

FuncMemory::~FuncMemory()
{
    for ( int i = 0; i < sets_array_size; i++)
	{
		if ( sets_array[ i] != NULL)
		{
			for ( int j = 0; j < pages_array_size; j++)
				if ( sets_array[ i][ j] != NULL)
					delete [] this->sets_array[ i][ j];
		
			delete [] this->sets_array[i];
		}
	}
	delete [] this->sets_array;
}

uint64 FuncMemory::startPC() const
{
    return this->text_start_addr;
}

uint64 FuncMemory::read( uint64 addr, unsigned short num_of_bytes) const
{
	if ( num_of_bytes <= 0)
		assert( 0);

    unsigned short bytes_read = 0;
	uint64 number = 0;
	
	uint64 set_idx = getSetIdx( addr_bits - page_num_size - offset_size, // set_bits
								addr,					                 // addr
								addr_bits);						         // addr_size
	
	uint64 page_idx = getPageIdx( page_num_size,
								  offset_size,
								  addr);							 	 // addr
	
	uint64 offset_idx = getOffsetIdx( offset_size, addr);
	
	if ( sets_array == NULL)
		assert( 0);
	
	if ( sets_array[ set_idx] == NULL)
	{
		cerr << "Sigmentation fault";
		assert( 0);
	} else if ( sets_array[ set_idx][ page_idx] == NULL)
	{
		cerr << "Sigmentation fault";
		assert( 0);
	}
			
	States state = RDWR;
	
	//State machine: 
	//State RDWR    - reading bytes
	//State CNG_PG  - changing page
	//State CNG_SET - changing set
	//State EXIT    - exit in case of an error
	while ( bytes_read < num_of_bytes)
	{
		switch( state)
		{
			case RDWR:
				number += ( sets_array[ set_idx][ page_idx][ offset_idx] << ( bytes_read * 8));
				bytes_read++;
				offset_idx++;
				
				if ( offset_idx >= page_size)
				{
					state = CNG_PG;
					page_idx++;
				}
			break;
			
			case CNG_PG:
				offset_idx = 0;
				state = RDWR;
				
				if ( page_idx >= pages_array_size)
				{
					state = CNG_SET;
					set_idx++;
				}
				else if ( sets_array[ set_idx][ page_idx] == NULL)
					state = EXIT;
			break;
			
			case CNG_SET:
				page_idx = 0;
				state = CNG_PG;
				
				
				if ( set_idx >= sets_array_size)
					state = EXIT;
				else if ( sets_array[ set_idx] == NULL)
					state = EXIT;
			break;
			
			case EXIT:
				cerr << "Sigmentation fault1" << endl << endl;
				assert( 0);
			break;
		}
	}

    return number;
}

void FuncMemory::write( uint64 value, uint64 addr, unsigned short num_of_bytes)
{	
	if ( num_of_bytes <= 0)
		assert( 0);
	
	union
	{
		uint64 number;
		uint8 bytes[8];
	};

    unsigned short bytes_write = 0;
	number = value;
	
	uint64 set_idx = getSetIdx( addr_bits - page_num_size - offset_size, // set_bits
								addr,					                 // addr
								addr_bits);						         // addr_size
		
	uint64 page_idx = getPageIdx( page_num_size,
								  offset_size,
								  addr);							 	 // addr
		
	uint64 offset_idx = getOffsetIdx( offset_size, addr);
	
	if ( sets_array[ set_idx] == NULL)
	{
		sets_array[ set_idx] = new uint8*[ pages_array_size]();
		sets_array[ set_idx][ page_idx] = new uint8[page_size]();
	} else if ( sets_array[ set_idx][ page_idx] == NULL)
		sets_array[ set_idx][ page_idx] = new uint8[page_size]();
	
	//State machine: 
	//State RDWR    - writing bytes
	//State CNG_PG  - changing page
	//State CNG_SET - changing set
	//State EXIT    - exit in case of an error
	States state = RDWR;
	
	while ( bytes_write < num_of_bytes)
	{
		switch( state)
		{
			case RDWR:
				sets_array[ set_idx][ page_idx][ offset_idx] = bytes[ bytes_write];
				bytes_write++;
				
				offset_idx++;
				if ( offset_idx >= page_size)
				{
					state = CNG_PG;
					page_idx++;
				}
			break;
			
			case CNG_PG:
				offset_idx = 0;
				state = RDWR;
				
				if ( page_idx >= pages_array_size)
				{
					state = CNG_SET;
					set_idx++;
				}
				else if ( sets_array[ set_idx][ page_idx] == NULL)
					sets_array[ set_idx][ page_idx] = new uint8[page_size];
			break;
			
			case CNG_SET:
				page_idx = 0;
				state = CNG_PG;
				
				if ( set_idx >= sets_array_size)
					state = EXIT;
				else if ( sets_array[ set_idx] == NULL)
					sets_array[ set_idx] = new uint8*[pages_array_size];
			break;
			
			case EXIT:
				cerr << "Sigmentation fault" << endl;
				assert( 0);
			break;
		}
	}
}

string FuncMemory::dump( string indent) const
{
	ostringstream oss;
	
	oss << indent << "Dump memory:" << endl
		<< indent << " Content:" << endl;
	
	uint64 bytes_count = 0;
	
	uint64 set_idx = 0;
	uint64 page_idx = 0;
	uint64 offset_idx = 0;
	uint64 addr = 0;
	
	uint8 buf[4];
	bool null_was_met = true;
	unsigned short number_of_zero = 0;  // how many zeros were read
	
	//State machine: 
	//State RDWR    - reading bytes
	//State CNG_PG  - changing page
	//State CNG_SET - changing set
	//State EXIT    - exit in case of an error
	States state = CNG_SET;
	
	while ( set_idx < this->sets_array_size)
	{ 
		switch( state)
		{
			case RDWR:
				if ( sets_array[ set_idx][ page_idx][ offset_idx] == 0)
					number_of_zero++;
				buf[ bytes_count] = sets_array[ set_idx][ page_idx][ offset_idx];
				bytes_count++;
				
				if ( bytes_count == 4)
				{
					if ( number_of_zero != 4)
					{
						oss << indent << "    0x" << hex << addr
							<< ":    " ;
						for ( int i = 0; i < 4; i++)
						{
							if ( buf[ i] < 16)
								oss << "0" << hex << ( uint16)buf[ i];
							else
								oss << hex << ( uint16)buf[ i];
						}
						oss << endl;
						null_was_met = false;
					} else
						if ( !null_was_met)
						{
							oss << indent << " ..." << endl;
							null_was_met = true;
						}
						
					addr += 4;
					number_of_zero = 0;
					bytes_count = 0;
				}
			
				offset_idx++;
				
				if ( offset_idx >= page_size)
				{
					state = CNG_PG;
					page_idx++;
				}
			break;
			
			case CNG_PG:
				offset_idx = 0;
				state = RDWR;
				
				if ( page_idx >= pages_array_size)
				{
					state = CNG_SET;
					set_idx++;
				}
				else if ( sets_array[ set_idx][ page_idx] == NULL)
				{
					state = CNG_PG;
					addr += page_size;
					page_idx++;
					if ( !null_was_met)
					{
						oss << indent << " ..." << endl;
						null_was_met = true;
					}
				}
				
			break;
			
			case CNG_SET:
				page_idx = 0;
				state = CNG_PG;
				
				if ( sets_array[ set_idx] == NULL)
				{
					state = CNG_SET;
					addr += pages_array_size * page_size;
					set_idx++;
					if ( !null_was_met)
					{
						oss << indent << " ..." << endl;
						null_was_met = true;
					}
				}
			break;
		}
	}

    return oss.str();

}
