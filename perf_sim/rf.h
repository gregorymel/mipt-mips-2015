/*
 * rf.h - mips register file
 * @author Pavel Kryukov pavel.kryukov@phystech.edu
 * Copyright 2015 MIPT-MIPS 
 */

#ifndef RF_H
#define RF_H

#include <func_instr.h>

class RF
{
		struct Reg
		{
			uint32 value;
			bool is_valid;
			Reg() : value(0ull), is_valid(true) {}
		} array[REG_NUM_MAX];
    public:
		uint32 read( RegNum num) const
   		{
        	assert( num != REG_NUM_MAX);
        	return array[num].value;
    	}

    	bool check( RegNum num) const
    	{
        	assert( num != REG_NUM_MAX);
        	return array[num].is_valid;
    	}

    	void invalidate( RegNum num)
    	{
        	assert( num != REG_NUM_MAX);
        	if ( num == REG_NUM_ZERO)
            	return;
        	array[num].is_valid = false;
    	}

    	void write( RegNum num, uint32 val)
    	{
        	assert( num != REG_NUM_MAX);
        	if ( REG_NUM_ZERO == num)
            	return;
        	array[num].value = val;
        	assert( check( num) == false);
        	array[num].is_valid = true;
    	}	


        inline void read_src1( FuncInstr& instr) const
        {
           size_t reg_num = instr.get_src1_num();
           instr.set_v_src1( array[reg_num].value);
        }

        inline void read_src2( FuncInstr& instr) const
        {
           size_t reg_num = instr.get_src2_num();
           instr.set_v_src2( array[reg_num].value);
        }

        inline void write_dst( const FuncInstr& instr)
        {
            size_t reg_num = instr.get_dst_num();
            if ( REG_NUM_ZERO != reg_num)
			{
                array[reg_num].value = instr.get_v_dst();
				array[reg_num].is_valid = true;
			}
        }

        inline void reset( RegNum reg)
        {
            array[ reg].value = 0;
        	array[ reg].is_valid = true;
        }
 		
        RF()
        {
            for ( size_t i = 0; i < REG_NUM_MAX; ++i)
                reset((RegNum)i);
        }
		
};
          
#endif
 