/*
 * perf_sim.cpp - mips performance simulator
 * @author Grigory Melnikov grigoriy.melnikov@phystech.edu
 * Copyright 2016 MIPT-MIPS 
 */

#ifndef PERF_SIM_H
#define PERF_SIM_H

#include <func_instr.h>
#include <func_memory.h>
#include <rf.h>		
#include <ports.h>
#include <iostream>
#include <sstream>


class MIPS
{
    protected:
        RF* rf;
        uint32 PC;
        FuncMemory* mem;

        uint32 fetch() const { return mem->read(PC); }
        void read_src(FuncInstr& instr) const {
            rf->read_src1(instr); 
            rf->read_src2(instr); 
	    }

        void load(FuncInstr& instr) const {
            instr.set_v_dst(mem->read(instr.get_mem_addr(), instr.get_mem_size()));
        }

        void store(const FuncInstr& instr) {
            mem->write(instr.get_v_src2(), instr.get_mem_addr(), instr.get_mem_size());
        }

	    void load_store(FuncInstr& instr) {
            if (instr.is_load())
                load(instr);
            else if (instr.is_store())
                store(instr);
        }

        void wb(const FuncInstr& instr) {
            rf->write_dst(instr);
        }
   public:
        MIPS();
        void run(const std::string& tr, uint32 instrs_to_run);
        ~MIPS();
};


class PerfMIPS : public MIPS
{
	bool PC_is_valid;
	uint32 executed_instrs;
	
	bool silent;
	std::ostringstream fetch_dump;
	std::ostringstream decode_dump;
	std::ostringstream exec_dump;
	std::ostringstream mem_dump;
	std::ostringstream wb_dump;
	std::ostringstream silent_dump;
	
	//fetch
	WritePort<uint32>* wp_fetch_2_dec;
	ReadPort<bool>* rp_dec_2_fetch_stall;
	
	//decode
	ReadPort<uint32>* rp_fetch_2_dec;
	WritePort<FuncInstr>* wp_dec_2_exec;
	WritePort<bool>* wp_dec_2_fetch_stall;
	ReadPort<bool>* rp_exec_2_dec_stall;
	
	//execute
	ReadPort<FuncInstr>* rp_dec_2_exec;
	WritePort<FuncInstr>* wp_exec_2_mem;
	WritePort<bool>* wp_exec_2_dec_stall;
	ReadPort<bool>* rp_mem_2_exec_stall;
	
	//memmory
	ReadPort<FuncInstr>* rp_exec_2_mem;
	WritePort<FuncInstr>* wp_mem_2_wb;
	WritePort<bool>* wp_mem_2_exec_stall;
	ReadPort<bool>* rp_wb_2_mem_stall;
	
	//writeback
	ReadPort<FuncInstr>* rp_mem_2_wb;
	WritePort<bool>* wp_wb_2_mem_stall;
	
	bool isAvalaible( FuncInstr& instr) { return rf->check( instr.get_src1_num()) && 
												 rf->check( instr.get_src2_num()); }
	//uint32 bytes_for_decode;
	bool toDecode;
public:
	//fetch
	void clock_fetch( int cycle);
	//decode
	void clock_decode( int cycle);
	//execute
	void clock_exec( int cycle);
	//memmory
	void clock_mem( int cycle);
	//writeback
	void clock_wb( int cycle);
	
    PerfMIPS();
	void run(const std::string& tr, uint32 instrs_to_run, bool silent);
};


#endif // PERF_SIM_H 
 