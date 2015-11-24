/**
 * @author Grigory Melnikov
 */

// protection from multi-include
#ifndef FUNC_INSTR__FUNC_INSTR_H
#define FUNC_INSTR__FUNC_INSTR_H

// Generic C++
#include <string>

// uArchSim modules
#include <types.h>

class FuncInstr
{
	enum Format
    {
        FORMAT_R,
        FORMAT_I,
        FORMAT_J
    } format;
	
	enum Register
	{
		ZERO = 0, AT, V0, V1, A0, A1, A2, A3,
		T0,   T1, T2, T3, T4, T5, T6, T7,
		S0,   S1, S2, S3, S4, S5, S6, S7,
		T8,   T9, K0, K1, GP, SP, S8, RA
	};
	
	union
    {
        struct
        {
            unsigned imm:16;
            unsigned t:5;
            unsigned s:5;
            unsigned opcode:6;
        } asI;
		
        struct
        {
            unsigned funct:6;
			unsigned shamt:5;
			unsigned d:5;
			unsigned t:5;
			unsigned s:5;
			unsigned opcode:6;
        } asR;
		
        struct
        {
			unsigned addr:26;
			unsigned opcode:6;
        } asJ;
		
        uint32 raw;
    } bytes;
	
	struct ISAEntry
    {
        const char* name;

        uint8 opcode;
        uint8 funct;

        FuncInstr::Format format;
		const char* argset;
    };
	
    static const ISAEntry isaTable[];
	
	//   PRIVATE FUNCTION
	void initFormat( uint32 bytes);
	void parseR( void);
	void parseI( void);
	void parseJ( void);
	std::string defReg( unsigned char reg) const;
	void parseArgSet( std::ostringstream& oss, const char* argset);
	
	std::string cmd_str;
  
    public:
        FuncInstr( uint32 bytes);
        std::string Dump( std::string indent = " ") const;
};

#endif // #ifndef FUNC_INSTR__FUNC_INSTR_H
