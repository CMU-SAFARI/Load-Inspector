/**********************************************************
 * Load Inspector
 * A SDE tool that instruments x86-64 binaries
 * and dumps loads that repeatedly fetches same data
 * from the same effective load address.
 *
 * Author: Rahul Bera (write2bera@gmail.com)
 **********************************************************/

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <unordered_map>
#include <unordered_set>

#include "pin.H"
extern "C"
{
#include "xed-interface.h"
#include "sde-agen.h"
}
#include "sde-init.H"
#include "ialarm.H"
#include "atomic.hpp"

// sde-control.H will allow to use the SDE's API
// for requesting a pointer to the SDE's controller
#include "sde-control.H"
#include "pcregions_control.H"

#if defined(PINPLAY)
#include "sde-pinplay-supp.H"
using namespace INSTLIB;
#endif

using namespace CONTROLLER;

#define PREFIX std::left << setw(40)
// #define LOAD_DEBUG 1
#define DUMP_STABLE_LOADS 1

//-------------------------------
// Tool arguments
//-------------------------------
static KNOB<std::string> KnobStatsFilename(KNOB_MODE_WRITEONCE, "pintool", "statf", "stable-load.stats.txt",
                                            "specify output file name");

static KNOB<bool> KnobDumpStableLoads(KNOB_MODE_WRITEONCE, "pintool", "dsl", "0",
                                "Dump stable load IPs");

static KNOB<std::string> KnobStableLoadsFilename(KNOB_MODE_WRITEONCE, "pintool", "slf", "stable-load.ips.txt",
                                      "specify stable load output filename");

static CACHELINE_COUNTER global_ins_counter = {0, 0};
static CACHELINE_COUNTER global_ins_counter_inside_roi = {0, 0};

static PIN_LOCK output_lock;

static BOOL inside_roi = FALSE;

// Contains knobs and instrumentation to recognize start/stop points
static CONTROLLER::CONTROL_MANAGER* sde_control = SDE_CONTROLLER::sde_controller_get();

// Create pc regions object
KNOB_COMMENT pcregion_knob_family("pintool:pcregions_control", "PC regions control knobs");
CONTROL_ARGS args("", "pintool:pcregions_control");
CONTROL_PCREGIONS pcregions(args, sde_control);

typedef enum
{
    RIP_LOAD = 0,
    STACK_LOAD,
    REG_LOAD,
    ALL_LOAD,
    NUM_LOAD_TYPES = ALL_LOAD
} load_type_t;

std::string load_type_t2str[] = { "RIP", "STACK", "REG", "NO-CAT" };

uint64_t agen_icount = 0;
uint64_t load_count [NUM_LOAD_TYPES+1] = {}, non_vector_load_count[NUM_LOAD_TYPES+1] = {};

typedef struct
{
    load_type_t load_type;
    uint64_t addr = 0xdeadbeef;
    uint64_t val = 0x0;
    uint64_t occur = 0;
} load_attr_t;

std::unordered_map<uint64_t, load_attr_t> load_addr_val_map;
std::unordered_set<uint64_t> load_ip_known_unstable;

VOID Handler(EVENT_TYPE ev, VOID* v, CONTEXT* ctxt, VOID* ip, THREADID tid, BOOL bcast)
{
    PIN_GetLock(&output_lock, tid + 1);

    string eventstr;

    switch (ev)
    {
        case EVENT_START:
            eventstr = "Sim-Start";
            inside_roi = TRUE;
            break;

        case EVENT_WARMUP_START:
            eventstr = "Warmup-Start";
            break;

        case EVENT_STOP:
            eventstr = "Sim-End";
            inside_roi = FALSE;
            break;

        case EVENT_WARMUP_STOP:
            eventstr = "Warmup-Stop";
            break;

        case EVENT_THREADID:
            std::cerr << "ThreadID" << endl;
            eventstr = "ThreadID";
            break;

        default:
            ASSERTX(false);
            break;
    }

    std::cerr << eventstr;
    std::cerr << " tid " << dec << tid << " pc " << hex << ip;
    std::cerr << " global_ins_count " << dec << global_ins_counter._count << endl;

    PIN_ReleaseLock(&output_lock);
}

static inline BOOL mem_op_is_rip(xed_decoded_inst_t *xedd, unsigned int mem_idx)
{
    return (xed_decoded_inst_get_base_reg(xedd, mem_idx) == XED_REG_RIP);
}

static inline BOOL mem_op_is_stack(xed_decoded_inst_t *xedd, unsigned int mem_idx)
{
    return (xed_decoded_inst_get_base_reg(xedd, mem_idx) == XED_REG_RSP 
            || xed_decoded_inst_get_base_reg(xedd, mem_idx) == XED_REG_RBP);
}

static inline load_type_t get_load_type(BOOL is_rip, BOOL is_stack)
{
    return is_rip ? RIP_LOAD : (is_stack ? STACK_LOAD : REG_LOAD);
}

static VOID capture_load(ADDRINT ip, ADDRINT ea, UINT32 size, BOOL is_rip, BOOL is_stack)
{
    if (!inside_roi)
        return;

    load_type_t ltype = get_load_type(is_rip, is_stack);

    load_count[ltype]++;
    load_count[ALL_LOAD]++;

    if (size > 8)
        return;

    uint64_t load_value = 0;
    PIN_SafeCopy((void *)&load_value, (void *)(ea), size);

    non_vector_load_count[ltype]++;
    non_vector_load_count[ALL_LOAD]++;

    // if already known to be unstable, nothing to do
    if (load_ip_known_unstable.find(ip) != load_ip_known_unstable.end())
        return;

    // otherwise, check the load addr/value
    auto it = load_addr_val_map.find(ip);
    if (it != load_addr_val_map.end())
    {
        // if either the addr or value mismatches,
        // blacklist the load IP to be unstable
        if (it->second.addr != ea || it->second.val != load_value)
        {
            load_ip_known_unstable.insert(it->first);
            load_addr_val_map.erase(it);
        }
        else
        {
            it->second.occur++;
        }
    }
    else
    {
        load_attr_t la;
        la.load_type = ltype;
        la.addr = ea;
        la.val = load_value;
        la.occur = 1;
        load_addr_val_map.insert(std::pair<uint64_t, load_attr_t>(ip, la));
    }

#ifdef LOAD_DEBUG
    std::cout << std::hex << "0x" << ip
              << ": S=" << std::dec << size << "B"
              << ", Addr=0x" << std::hex << ea
              << ", Val=0x" << std::hex << load_value
              << std::endl;
#endif
}

static VOID mem_agen(THREADID tid, ADDRINT ip, xed_decoded_inst_t *xedd)
{
    if (!inside_roi)
        return;

    unsigned int i, nrefs = 0;
    if (!sde_agen_init(tid, &nrefs))
        return;

    agen_icount++;
    for (i = 0; i < nrefs; i++)
    {
        sde_memop_info_t meminfo;
        sde_agen_address(tid, i, &meminfo);

        BOOL is_rip   = mem_op_is_rip(xedd, i);
        BOOL is_stack = mem_op_is_stack(xedd, i);

        if (meminfo.memop_type == SDE_MEMOP_LOAD)
            capture_load(ip, meminfo.memea, meminfo.bytes_per_ref, is_rip, is_stack);
    }
}

// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID* v)
{
    xed_decoded_inst_t *xedd = INS_XedDec(ins);
    sde_bool_t agen_attr = sde_agen_is_agen_required(xedd);

    if (agen_attr)
    {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)mem_agen, IARG_THREAD_ID, IARG_INST_PTR, IARG_PTR, xedd, IARG_END);
        return;
    }

    if (INS_IsMemoryRead(ins) && INS_IsStandardMemop(ins))
    {
        BOOL is_rip = mem_op_is_rip(xedd, 0);
        BOOL is_stack = mem_op_is_stack(xedd, 0);

        INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)capture_load, IARG_INST_PTR, IARG_MEMORYREAD_EA,
                                 IARG_MEMORYREAD_SIZE, IARG_BOOL, is_rip, IARG_BOOL, is_stack, IARG_END);
    }

    if (INS_HasMemoryRead2(ins) && INS_IsStandardMemop(ins))
    {
        BOOL is_rip = mem_op_is_rip(xedd, 1);
        BOOL is_stack = mem_op_is_stack(xedd, 1);

        INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)capture_load, IARG_INST_PTR, IARG_MEMORYREAD2_EA,
                                 IARG_MEMORYREAD_SIZE, IARG_BOOL, is_rip, IARG_BOOL, is_stack, IARG_END);
    }
}

// This function is called before every block
// Use the fast linkage for calls
VOID PIN_FAST_ANALYSIS_CALL docount(ADDRINT c)
{
    ATOMIC::OPS::Increment<UINT64>(&global_ins_counter._count, c);
    if(inside_roi) ATOMIC::OPS::Increment<UINT64>(&global_ins_counter_inside_roi._count, c);
}

// Pin calls this function every time a new basic block is encountered
// It inserts a call to docount
VOID Trace(TRACE trace, VOID* v)
{
    // Visit every basic block  in the trace
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        BBL_InsertCall(bbl, IPOINT_ANYWHERE, AFUNPTR(docount), IARG_FAST_ANALYSIS_CALL,
                       IARG_UINT32, BBL_NumIns(bbl), IARG_END);
    }
}

VOID fini(INT32 code, VOID *v)
{
    std::vector<std::pair<uint64_t, load_attr_t>> pairs;
    std::ofstream stats;
    stats.open(KnobStatsFilename.Value().c_str());

    stats << PREFIX << "icount.total " << global_ins_counter._count << std::endl;
    stats << PREFIX << "icount.inside_roi " << global_ins_counter_inside_roi._count << std::endl;
    stats << PREFIX << "icount.agen " << agen_icount << std::endl;
    stats << std::endl;

    stats << PREFIX << "load.total " << load_count[ALL_LOAD] << std::endl;
    stats << PREFIX << "load.RIP_LOAD " << load_count[RIP_LOAD] << std::endl;
    stats << PREFIX << "load.STACK_LOAD " << load_count[STACK_LOAD] << std::endl;
    stats << PREFIX << "load.REG_LOAD " << load_count[REG_LOAD] << std::endl;
    stats << std::endl;

    stats << PREFIX << "non_vector_load.total " << non_vector_load_count[ALL_LOAD] << std::endl;
    stats << PREFIX << "non_vector_load.RIP_LOAD " << non_vector_load_count[RIP_LOAD] << std::endl;
    stats << PREFIX << "non_vector_load.STACK_LOAD " << non_vector_load_count[STACK_LOAD] << std::endl;
    stats << PREFIX << "non_vector_load.REG_LOAD " << non_vector_load_count[REG_LOAD] << std::endl;
    stats << std::endl;

    uint64_t num_load_ips = 0, num_stable_load_ips[NUM_LOAD_TYPES+1] = {}, num_stable_loads[NUM_LOAD_TYPES+1] = {};
    
    num_load_ips = load_ip_known_unstable.size() + load_addr_val_map.size();

    for (auto it = load_addr_val_map.begin(); it != load_addr_val_map.end(); ++it)
    {
        if (it->second.occur > 1)
        {
            num_stable_load_ips[it->second.load_type]++;
            num_stable_load_ips[ALL_LOAD]++;

            num_stable_loads[it->second.load_type] += it->second.occur;
            num_stable_loads[ALL_LOAD] += it->second.occur;
            
            if (KnobDumpStableLoads)
                pairs.push_back(*it);
        }
    }

    stats << PREFIX << "load_ips.total " << num_load_ips << std::endl;
    stats << PREFIX << "stable_load_ips.total " << num_stable_load_ips[ALL_LOAD] << std::endl;
    stats << PREFIX << "stable_load_ips.RIP_LOAD " << num_stable_load_ips[RIP_LOAD] << std::endl;
    stats << PREFIX << "stable_load_ips.STACK_LOAD " << num_stable_load_ips[STACK_LOAD] << std::endl;
    stats << PREFIX << "stable_load_ips.REG_LOAD " << num_stable_load_ips[REG_LOAD] << std::endl;
    stats << std::endl;

    stats << PREFIX << "stable_loads.total " << num_stable_loads[ALL_LOAD] << std::endl;
    stats << PREFIX << "stable_loads.RIP_LOAD " << num_stable_loads[RIP_LOAD] << std::endl;
    stats << PREFIX << "stable_loads.STACK_LOAD " << num_stable_loads[STACK_LOAD] << std::endl;
    stats << PREFIX << "stable_loads.REG_LOAD " << num_stable_loads[REG_LOAD] << std::endl;
    
    stats.close();
    
    if (KnobDumpStableLoads)
    {
        std::ofstream slfile;
        slfile.open(KnobStableLoadsFilename.Value().c_str());

        slfile << "stable_load_ip,occurence,load_type" << std::endl;
        std::sort(pairs.begin(), pairs.end(),
                [](std::pair<uint64_t, load_attr_t> &a, std::pair<uint64_t, load_attr_t> &b)
                {
                    return a.second.occur > b.second.occur;
                });
        for (auto it = pairs.begin(); it != pairs.end(); ++it)
        {
            slfile << "0x" << std::hex << it->first
                << "," << std::dec << it->second.occur 
                << "," << load_type_t2str[it->second.load_type] << std::endl;
        }
        
        slfile.close();
    }
}

int main(int argc, char* argv[])
{
    sde_pin_init(argc, argv);
    PIN_InitLock(&output_lock);

    INS_AddInstrumentFunction(Instruction, 0);
    TRACE_AddInstrumentFunction(Trace, 0);

    //Register handler on SDE's controller, must be done before PIN_StartProgram
    sde_control->RegisterHandler(Handler, 0, 0);

    sde_init();

    pcregions.Activate();

    // Fini function
    PIN_AddFiniFunction(fini, 0);

    // Start the program, never returns
    PIN_StartProgram();

    return 0;
}
