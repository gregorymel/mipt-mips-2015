/**
 * @author Melnikov Grigory
 * Copyright 2015 uArchSim iLab project
 */

//Generic C
#include <assert.h> 
#include <string.h>

//Generic C++
#include <sstream>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstdio>

//uArchSim modules
#include <func_instr.h>

const FuncInstr::ISAEntry FuncInstr::isaTable[] =
{
    // name       opcode  func   format                argset
    { "add ",     0x0,    0x20,  FuncInstr::FORMAT_R,  "dst" },
    { "addu ",    0x0,    0x21,  FuncInstr::FORMAT_R,  "dst" },
    { "sub ",     0x0,    0x22,  FuncInstr::FORMAT_R,  "dst" },
    { "subu ",    0x0,    0x23,  FuncInstr::FORMAT_R,  "dst" },
    { "addi ",    0x8,    0x0,   FuncInstr::FORMAT_I,  "tsC" },
    { "addiu ",   0x9,    0x0,   FuncInstr::FORMAT_I,  "tsC" },
    { "mult ",    0x0,    0x18,  FuncInstr::FORMAT_R,  "st"  },
    { "multu ",   0x0,    0x19,  FuncInstr::FORMAT_R,  "st"  },
    { "div ",     0x0,    0x1A,  FuncInstr::FORMAT_R,  "st"  },
    { "divu ",    0x0,    0x1B,  FuncInstr::FORMAT_R,  "st"  },
    { "mfhi ",    0x0,    0x10,  FuncInstr::FORMAT_R,  "d"   },
    { "mthi ",    0x0,    0x11,  FuncInstr::FORMAT_R,  "s"   },
    { "mflo ",    0x0,    0x12,  FuncInstr::FORMAT_R,  "d"   },
    { "mtlo ",    0x0,    0x13,  FuncInstr::FORMAT_R,  "s"   },
    { "sll ",     0x0,    0x0,   FuncInstr::FORMAT_R,  "dtS" },
    { "srl ",     0x0,    0x2,   FuncInstr::FORMAT_R,  "dtS" },
    { "sra ",     0x0,    0x3,   FuncInstr::FORMAT_R,  "dtS" },
    { "sllv ",    0x0,    0x4,   FuncInstr::FORMAT_R,  "dts" },
    { "srlv ",    0x0,    0x6,   FuncInstr::FORMAT_R,  "dts" },
    { "srav ",    0x0,    0x7,   FuncInstr::FORMAT_R,  "dts" },
    { "lui ",     0xF,    0x0,   FuncInstr::FORMAT_I,  "tC"  },
    { "slt ",     0x0,    0x2A,  FuncInstr::FORMAT_R,  "dts" },
    { "sltu ",    0x0,    0x2B,  FuncInstr::FORMAT_R,  "dts" },
    { "slti ",    0xA,    0x0,   FuncInstr::FORMAT_I,  "stC" },
    { "sltiu ",   0xB,    0x0,   FuncInstr::FORMAT_I,  "stC" },
    { "and ",     0x0,    0x24,  FuncInstr::FORMAT_R,  "dts" },
    { "or ",      0x0,    0x25,  FuncInstr::FORMAT_R,  "dts" },
    { "xor ",     0x0,    0x26,  FuncInstr::FORMAT_R,  "dts" },
    { "nor ",     0x0,    0x27,  FuncInstr::FORMAT_R,  "dts" },
    { "andi ",    0xC,    0x0,   FuncInstr::FORMAT_I,  "stC" },
    { "ori ",     0xD,    0x0,   FuncInstr::FORMAT_I,  "stC" },
    { "xori ",    0xE,    0x0,   FuncInstr::FORMAT_I,  "stC" },
    { "beq ",     0x4,    0x0,   FuncInstr::FORMAT_I,  "stC" },
    { "bne ",     0x5,    0x0,   FuncInstr::FORMAT_I,  "stC" },
    { "blez ",    0x6,    0x0,   FuncInstr::FORMAT_I,  "sC"  },
    { "bgtz ",    0x7,    0x0,   FuncInstr::FORMAT_I,  "sC"  },
    { "j ",       0x2,    0x0,   FuncInstr::FORMAT_J,  "A"   },
    { "jal ",     0x3,    0x0,   FuncInstr::FORMAT_J,  "A"   },
    { "jr ",      0x0,    0x8,   FuncInstr::FORMAT_R,  "s"   },
    { "jalr ",    0x0,    0x9,   FuncInstr::FORMAT_R,  "s"   },
    { "lb ",      0x20,   0x0,   FuncInstr::FORMAT_I,  "tC(s)"},
    { "lh ",      0x21,   0x0,   FuncInstr::FORMAT_I,  "tC(s)"},
    { "lw ",      0x23,   0x0,   FuncInstr::FORMAT_I,  "tC(s)"},
    { "lbu ",     0x24,   0x0,   FuncInstr::FORMAT_I,  "tC(s)"},
    { "lhu ",     0x25,   0x0,   FuncInstr::FORMAT_I,  "tC(s)"},
    { "sb ",      0x28,   0x0,   FuncInstr::FORMAT_I,  "tC(s)"},
    { "sh ",      0x29,   0x0,   FuncInstr::FORMAT_I,  "tC(s)"},
    { "sw ",      0x2B,   0x0,   FuncInstr::FORMAT_I,  "tC(s)"},
    { "syscall ", 0x0,    0xC,   FuncInstr::FORMAT_R,  ""    },
    { "break ",   0x0,    0xD,   FuncInstr::FORMAT_R,  ""    },
    { "trap ",    0x1A,   0x0,   FuncInstr::FORMAT_J,  "A"   },
};


FuncInstr::FuncInstr( uint32 bytes)
{
    this->initFormat( bytes);
    switch (this->format)
    {
        case FORMAT_R:
            this->parseR();
            break;
        case FORMAT_I:
            this->parseI();
            break;
        case FORMAT_J:
            this->parseJ();
            break;
        default:
            assert(0);
    }
};

void FuncInstr::initFormat( uint32 bytes)
{
	this->bytes.raw = bytes;
	
	if ( this->bytes.asR.opcode == 0)
		this->format = FORMAT_R;
	else
	{
		// compare with FORMAT_J's opcode
		if ( this->bytes.asJ.opcode == 0x1A
			|| this->bytes.asJ.opcode == 0x2
			|| this->bytes.asJ.opcode == 0x3)
		{
			this->format = FORMAT_J;
		} else
			this->format = FORMAT_I;
	}
};

void FuncInstr::parseI( void)
{
	std::ostringstream oss;

	unsigned size = sizeof( isaTable) / sizeof( isaTable[ 0]);
	for ( unsigned i = 0; i < size; i++)
	{
		if ( this->bytes.asI.opcode == FuncInstr::isaTable[ i].opcode
			&& FORMAT_I == FuncInstr::isaTable[ i].format)
		{
			oss << isaTable[ i].name;
			this->parseArgSet( oss, isaTable[ i].argset);
			break;
		}
		
 		if ( i == size - 1)
		{	
			std::cerr << "ERROR. FAILED TO PARSE FORMAT I";
			exit( EXIT_FAILURE);
		}
	}
	
	this->cmd_str = oss.str();
};

void FuncInstr::parseJ( void)
{	
	std::ostringstream oss;
	
	unsigned size = sizeof( isaTable) / sizeof( isaTable[ 0]);
	for ( unsigned i = 0; i < size; i++)
	{
		if ( this->bytes.asI.opcode == isaTable[ i].opcode
			&& FORMAT_J == isaTable[ i].format)
		{
			oss << isaTable[ i].name;
			this->parseArgSet( oss, isaTable[ i].argset);
			break;
		}
		
		if ( i == ( size - 1))
		{	
			std::cerr << "ERROR. FAILED TO PARSE FORMAT J";
			exit( EXIT_FAILURE);
		}
	}
	
	this->cmd_str = oss.str();
};

void FuncInstr::parseR( void)
{
	std::ostringstream oss;
	
	unsigned size = sizeof( isaTable) / sizeof( isaTable[ 0]);
	for ( unsigned i = 0; i < size; i++)
	{
		if ( this->bytes.asR.funct == isaTable[ i].funct
			&& FORMAT_R == isaTable[ i].format)
		{
			oss << isaTable[ i].name;
			this->parseArgSet( oss, isaTable[ i].argset);
			break;
		}
		
		if ( i == ( size - 1))
		{	
			std::cerr << "ERROR. FAILED TO PARSE FORMAT R";
			exit( EXIT_FAILURE);
		}
	}

	this->cmd_str = oss.str();
};


void FuncInstr::parseArgSet( std::ostringstream& oss, const char* argset)
{
	std::string istr = ""; // intermediate symbols (symbols between args)
		
	for ( unsigned i = 0; i < strlen( argset); i++)
	{
		switch( argset[ i])
		{
			case '(':
				oss << "(";
				istr = "";
				break;
			case ')':
				oss << ")";
				break;
			case 'd':
				oss << istr << defReg( (unsigned char)this->bytes.asR.d);
				istr = ", ";
				break;
			case 't':
				oss << istr << defReg( (unsigned char)this->bytes.asR.t);
				istr = ", ";
				break;
			case 's':
				oss << istr << defReg( (unsigned char)this->bytes.asR.s);
				istr = ", ";
				break;
			case 'S':
				oss << istr << "0x" << std::hex << this->bytes.asR.shamt;
				break;
			case 'A':
				oss << "0x" << std::hex << this->bytes.asJ.addr;
				break;
			case 'C':
				oss << istr << "0x" << std::hex << this->bytes.asI.imm;
				break;
			default:
				assert( 0);
		}
	}
};

// define register
std::string FuncInstr::defReg( unsigned char reg) const
{
    switch( reg)
    {
        case ZERO: return "$zero";
        case AT: return "$at";
        case V0: return "$v0";
        case V1: return "$v1";
        case A0: return "$a0";
        case A1: return "$a1";
        case A2: return "$a2";
        case A3: return "$a3";
        case T0: return "$t0";
        case T1: return "$t1";
        case T2: return "$t2";
        case T3: return "$t3";
        case T4: return "$t4";
        case T5: return "$t5";
        case T6: return "$t6";
        case T7: return "$t7";
        case S0: return "$s0";
        case S1: return "$s1";
        case S2: return "$s2";
        case S3: return "$s3";
        case S4: return "$s4";
        case S5: return "$s5";
        case S6: return "$s6";
        case S7: return "$s7";
        case T8: return "$t8";
        case T9: return "$t9";
        case K0: return "$k0";
        case K1: return "$k1";
        case GP: return "$gp";
        case SP: return "$sp";
        case S8: return "$s8";
        case RA: return "$ra";
        default: assert( 0); break;
    }
};

std::string FuncInstr::Dump(std::string indent) const
{
	std::ostringstream oss;
	oss << indent << this->cmd_str;
	
	return oss.str();
}


std::ostream& operator<< ( std::ostream& out, const FuncInstr& instr)
{	
	out << instr.Dump("");
	return out;
}