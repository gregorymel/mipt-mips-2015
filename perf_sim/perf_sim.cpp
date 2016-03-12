#include <iostream>


#include <perf_sim.h>

#define PORT_BW 1
#define PORT_FANOUT 1
#define PORT_LATENCY 1
#define INSTR_SIZE 4
#define WIDTH 10

/************************PRINT FUNCTIONS***********************/

void Dump( uint32 instr_bytes, std::ostringstream &oss)
{
	oss << std::hex << "0x" << instr_bytes << std::dec;
	oss << std::endl;
}

void Dump( FuncInstr& instr, std::ostringstream &oss)
{
    oss << instr;
    oss << std::endl;
}

void Dump( std::ostringstream &oss)
{
	oss << "bubble\n";
}
/*****************************************************************/

MIPS::MIPS()
{
    rf = new RF();
}

void MIPS::run(const std::string& tr, uint32 instrs_to_run)
{
    mem = new FuncMemory(tr.c_str());
    PC = mem->startPC();
    for (uint32 i = 0; i < instrs_to_run; ++i) {
        // fetch
        uint32 instr_bytes = fetch();
   
        // decode
        FuncInstr instr(instr_bytes, PC);

        // read sources
        read_src(instr);

        // execute
        instr.execute();

        // load/store
        load_store(instr);

        // writeback
        wb(instr);
        
        // PC update
        PC = instr.get_new_PC();
        
        // dump
        std::cout << instr << std::endl;
    }
    delete mem;
}

MIPS::~MIPS()
{
    delete rf;
}

PerfMIPS::PerfMIPS() : PC_is_valid( true)
{
	//fetch
	wp_fetch_2_dec = new WritePort<uint32>( "FETCH_2_DECODE", PORT_BW, PORT_FANOUT);
	rp_dec_2_fetch_stall = new ReadPort<bool>( "DECODE_2_FETCH_STALL", PORT_LATENCY);
	
	//decode
	rp_fetch_2_dec = new ReadPort<uint32>( "FETCH_2_DECODE", PORT_LATENCY);
	wp_dec_2_exec = new WritePort<FuncInstr>( "DECODE_2_EXEC", PORT_BW, PORT_FANOUT);
	wp_dec_2_fetch_stall = new WritePort<bool>( "DECODE_2_FETCH_STALL", PORT_BW, PORT_FANOUT);
	rp_exec_2_dec_stall  = new ReadPort<bool>( "EXEC_2_DECODE_STALL", PORT_LATENCY);
	
	//execute
	rp_dec_2_exec = new ReadPort<FuncInstr>( "DECODE_2_EXEC", PORT_LATENCY);
	wp_exec_2_mem = new WritePort<FuncInstr>( "EXEC_2_MEM", PORT_BW, PORT_FANOUT);
	wp_exec_2_dec_stall = new WritePort<bool>( "EXEC_2_DECODE_STALL", PORT_BW, PORT_FANOUT);
	rp_mem_2_exec_stall = new ReadPort<bool>( "MEM_2_EXEC_STALL", PORT_LATENCY);
	
	//memmory
	rp_exec_2_mem = new ReadPort<FuncInstr>( "EXEC_2_MEM", PORT_LATENCY);
	wp_mem_2_wb = new WritePort<FuncInstr>( "MEM_2_WB", PORT_BW, PORT_FANOUT);
	wp_mem_2_exec_stall = new WritePort<bool>( "MEM_2_EXEC_STALL", PORT_BW, PORT_FANOUT);
	rp_wb_2_mem_stall   = new ReadPort<bool>( "WB_2_MEM_STALL", PORT_LATENCY);
	
	//writeback
	rp_mem_2_wb = new ReadPort<FuncInstr>( "MEM_2_WB", PORT_LATENCY);
	wp_wb_2_mem_stall = new WritePort<bool>( "WB_2_MEM_STALL", PORT_BW, PORT_FANOUT);
	
	Port<FuncInstr>::init();
	Port<bool>::init();
	Port<uint32>::init();
}

void PerfMIPS::clock_fetch( int cycle)
{
	bool is_stall = false;
	
	rp_dec_2_fetch_stall->read( &is_stall, cycle);
	if ( is_stall || !PC_is_valid)
	{
		Dump( fetch_dump);
		return;
	}
	
	if ( PC_is_valid)
	{
		uint32 instr_bytes = fetch();
		Dump( instr_bytes, fetch_dump);
		wp_fetch_2_dec->write( instr_bytes, cycle);
	}
}

void PerfMIPS::clock_decode( int cycle)
{
	bool is_stall = false;
	
	rp_exec_2_dec_stall->read( &is_stall, cycle);
	if ( is_stall)
	{
		Dump( decode_dump);
		wp_dec_2_fetch_stall->write( true, cycle);
		return;
	}
	
	static uint32 bytes_for_decode;
	bool isData = rp_fetch_2_dec->read( &bytes_for_decode, cycle);
	
	if ( !isData && !toDecode)
	{
		Dump( decode_dump);
		return;
	}
	
	FuncInstr cur_instr( bytes_for_decode, PC);
	if ( cur_instr.is_jump() && isData)
	{
		PC_is_valid = false;
	}

	if ( isAvalaible( cur_instr))
	{
		read_src( cur_instr);
		rf->invalidate( cur_instr.get_dst_num());
		wp_dec_2_exec->write( cur_instr, cycle);
		
		if ( !cur_instr.is_jump())
			PC += INSTR_SIZE;
		
		toDecode = false;
		Dump( cur_instr, decode_dump);
	}
	else {
		Dump( decode_dump);
		wp_dec_2_fetch_stall->write( true, cycle);
		toDecode = true;
	}
}
/*********************************************************/

void PerfMIPS::clock_exec( int cycle)
{	
	bool is_stall = false;
	rp_mem_2_exec_stall->read( &is_stall, cycle);
	if ( is_stall)
	{
		wp_exec_2_dec_stall->write( true, cycle);
		Dump( exec_dump);
		return;
	}
	
	FuncInstr cur_instr;
	if ( !rp_dec_2_exec->read( &cur_instr, cycle))
	{
		Dump( exec_dump);
		return;
	}
	
	
	cur_instr.execute();
	Dump( cur_instr, exec_dump);
	wp_exec_2_mem->write( cur_instr, cycle);
}

void PerfMIPS::clock_mem( int cycle)
{	
	bool is_stall = false;
	rp_wb_2_mem_stall->read( &is_stall, cycle);
	if ( is_stall)
	{
		Dump( mem_dump);
		wp_mem_2_exec_stall->write( true, cycle);
		return;
	}
	
	FuncInstr cur_instr;
	if ( !rp_exec_2_mem->read( &cur_instr, cycle))
	{
		Dump( mem_dump);
		return;
	}
	
	
	load_store( cur_instr);
	wp_mem_2_wb->write( cur_instr, cycle);
	Dump( cur_instr, mem_dump);
}

void PerfMIPS::clock_wb( int cycle)
{
	FuncInstr cur_instr;
	if ( !rp_mem_2_wb->read( &cur_instr, cycle))
	{
		Dump( wb_dump);
		return;
	}
	
	if ( cur_instr.is_jump())
	{
		PC_is_valid = true;
		PC = cur_instr.get_new_PC();
	}
	
	wb( cur_instr);
	wp_wb_2_mem_stall->write( false, cycle);
	Dump( cur_instr, wb_dump);
	Dump( cur_instr, silent_dump);
	
	executed_instrs++;
}


void PerfMIPS::run(const std::string& tr, uint32 instrs_to_run, bool silent)
{
	mem = new FuncMemory(tr.c_str());
    PC = mem->startPC();
	
    this->silent = silent;
    executed_instrs = 0;
    int cycle = 0;
	toDecode = false;
    while ( executed_instrs < instrs_to_run)
    {
		clock_wb( cycle);
		clock_decode( cycle);
        clock_fetch( cycle);
        clock_exec( cycle);
        clock_mem( cycle);
        if (!silent)
        {
            std::cout  << "fetch     " << "cycle " << cycle << ":  " <<  fetch_dump.str()
					   << "decode    " << "cycle " << cycle << ":  " <<  decode_dump.str()
					   << "execute   " << "cycle " << cycle << ":  " <<  exec_dump.str()
					   << "memory    " << "cycle " << cycle << ":  " <<  mem_dump.str()
					   << "writeback " << "cycle " << cycle << ":  " <<  wb_dump.str()
					   << std::endl << std::endl;

			fetch_dump.str("");
			decode_dump.str("");
			exec_dump.str("");
			mem_dump.str("");
			wb_dump.str("");
        } 
        else {
			std::cout << silent_dump.str();
			silent_dump.str("");
		}
        cycle++;
    }
    delete mem;
}