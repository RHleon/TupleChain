#ifndef LL_PROCESSING_UNIT_H
#define LL_PROCESSING_UNIT_H

#include <packet-classification.hpp>
#include <ll-config-file.hpp>

namespace pp { 
    
struct memory_metric_t
{
    uint64_t nBytes;
};

struct lookup_metric_t
{
    double latency;
    double throughput;
    lookup_metric_t() : latency(0), throughput(0) {};
};

struct algorithm_args_t
{
    int num_fields;
    int threshold;
    int height;
    uint32_t etc_mask;
    algorithm_args_t()
	: num_fields(2)
	, threshold(0)
	, height(1)
	, etc_mask(0xff000000)
    {
    }
};
    
struct pc_unit_t
{
    std::string name;
    algorithm_args_t (*parseConfigFile)(LLConfigFile config);
    memory_metric_t  (*construct)(algorithm_args_t args, int nRules, pc_rule_t* rules);
    void             (*release)();
    uint32_t         (*matchPacket)(uint32_t* const& fields);
    void             (*insertRule)(const pc_rule_t& rule);
    void             (*deleteRule)(const pc_rule_t& rule);
};

struct pc_unit_t* selectClassifier(const std::string& name);



} // namespace pp

#endif
