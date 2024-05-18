/**********************************************************
 * Defines all stats of Load Inspector
 * Author: Rahul Bera (write2bera@gmail.com)
 **********************************************************/

#ifndef STATS_H
#define STATS_H

#include <unordered_map>
#include <unordered_set>
#include <iomanip>
#include "ialarm.H"


typedef enum
{
    RIP_LOAD = 0,
    STACK_LOAD,
    REG_LOAD,
    NUM_LOAD_TYPES
} load_type_t;

std::string load_type_t2str[] = { "RIP", "STACK", "REG" };

const uint32_t NUM_LOAD_SIZES = 8;
std::string load_size2str[] = { "1B", "2B", "4B", "8B", "16B", "32B", "64B", "UNCATEGORIZED" };

#define FOREACH_LOAD_TYPE_SIZE(body)                                \
    do                                                              \
    {                                                               \
        for (uint32_t type = 0; type < NUM_LOAD_TYPES; ++type)      \
            for (uint32_t size = 0; size < NUM_LOAD_SIZES; ++size)  \
                body;                                               \
    } while (0)

typedef struct
{
    load_type_t load_type;
    uint64_t addr = 0xdeadbeef;
    uint64_t val = 0x0;
    uint32_t sizeb = 0;
    uint64_t occur = 0;
} load_attr_t;


//-------------------------------//
// Stats used in the tool
//-------------------------------//
static CACHELINE_COUNTER global_ins_counter = {0, 0};
static CACHELINE_COUNTER global_ins_counter_inside_roi = {0, 0};
static uint64_t agen_icount = 0;
static uint64_t load_count[NUM_LOAD_TYPES][NUM_LOAD_SIZES]= {};
static std::unordered_map<uint64_t, load_attr_t> load_addr_val_map;
static std::unordered_set<uint64_t> load_ip_known_unstable;


//-------------------------------//
// Dumps all the stats
//-------------------------------//
static void dump_stats(std::string stats_filename, bool dump_stable_loads, std::string stable_load_stats_filename)
{
    std::ofstream stats;
    stats.open(stats_filename.c_str());

    std::vector<std::pair<uint64_t, load_attr_t>> pairs;

    uint64_t total_loads = 0, total_loads_nv = 0, total_loads_v = 0;
    FOREACH_LOAD_TYPE_SIZE({
        total_loads += load_count[type][size];
        if(size >= 4)
            total_loads_v += load_count[type][size];
        else
            total_loads_nv += load_count[type][size];
    });

    stats << "icount.total " << global_ins_counter._count << std::endl;
    stats << "icount.inside_roi " << global_ins_counter_inside_roi._count << std::endl;
    stats << "icount.agen " << agen_icount << std::endl;
    stats << std::endl;

    stats << "load.total " << total_loads << std::endl;
    stats << "load.non_vector " << total_loads_nv << std::endl;
    stats << "load.vector " << total_loads_v << std::endl;
    FOREACH_LOAD_TYPE_SIZE({
        stats << "load." << load_type_t2str[type] << "." << load_size2str[size] << " " << load_count[type][size] << std::endl;
    });
    stats << std::endl;

    uint64_t num_load_ips = 0, 
             num_stable_load_ips[NUM_LOAD_TYPES][NUM_LOAD_SIZES] = {},
             num_stable_loads[NUM_LOAD_TYPES][NUM_LOAD_SIZES] = {},
             total_stable_load_ips = 0,
             total_stable_loads = 0;
    
    num_load_ips = load_ip_known_unstable.size() + load_addr_val_map.size();

    for (auto it = load_addr_val_map.begin(); it != load_addr_val_map.end(); ++it)
    {
        if (it->second.occur > 1)
        {
            num_stable_load_ips[it->second.load_type][it->second.sizeb]++;
            total_stable_load_ips++;

            num_stable_loads[it->second.load_type][it->second.sizeb] += it->second.occur;
            total_stable_loads += it->second.occur;
            
            if (dump_stable_loads)
                pairs.push_back(*it);
        }
    }

    stats << "load_ips.total " << num_load_ips << std::endl;
    stats << "stable_load_ips.total " << total_stable_load_ips << std::endl;
    FOREACH_LOAD_TYPE_SIZE({
        stats << "stable_load_ips." << load_type_t2str[type] << "." << load_size2str[size] << " " << num_stable_load_ips[type][size] << std::endl;
    });
    stats << std::endl;

    stats << "stable_loads.total " << total_stable_loads << std::endl;
    FOREACH_LOAD_TYPE_SIZE({
        stats << "stable_loads." << load_type_t2str[type] << "." << load_size2str[size] << " " << num_stable_loads[type][size] << std::endl;
    });

    stats.close();

    if(dump_stable_loads)
    {
        std::ofstream sl_stats;
        sl_stats.open(stable_load_stats_filename.c_str());

        sl_stats << "stable_load_ip,occurence,load_type" << std::endl;
        std::sort(pairs.begin(), pairs.end(),
                [](std::pair<uint64_t, load_attr_t> &a, std::pair<uint64_t, load_attr_t> &b)
                {
                    return a.second.occur > b.second.occur;
                });
        for (auto it = pairs.begin(); it != pairs.end(); ++it)
        {
            sl_stats << "0x" << std::hex << it->first
                << "," << std::dec << it->second.occur 
                << "," << load_type_t2str[it->second.load_type] << std::endl;
        }

        sl_stats.close();
    }
}


#endif

