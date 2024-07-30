#ifndef LL_BENCHMARK_H
#define LL_BENCHMARK_H

#include <ll-processing-unit.hpp>
#include <ll-pkt-io.hpp>

namespace pp { 

struct test_data_t
{
    std::string rule_file;
    std::string pkt_file;
    
    rule_t* rules; // to construction pu
    packet_t* packets; // for simulation
    action_t* actions;
    update_t* insertions;
    update_t* deletions;
    packet_t* updatePackets;
    action_t* updateActions;

    int nRules;
    int nPackets;
    int nInsertions;
    int nDeletions;
    int nUpdatePackets;
};
    
class LLBenchMark
{
public:
    static void run(const char* cfg_file);

public:
    LLBenchMark(const char* file_name,
		const std::string& algo = "tss",
		const std::string& io = "dummy");
    ~LLBenchMark();

    memory_metric_t runConstruction();
    lookup_metric_t runPCSimulation();
    void runClassBench(const std::string& output);
    void runPCEmulation(){};
    void runEvaluation(){};

private:
    void prepareTestData(LLConfigFile config);

private:
    struct pkt_io_t* m_io;
    struct pc_unit_t* m_classifier;
    struct io_params_t m_params;
    struct algorithm_args_t m_args;
    struct test_data_t m_data;
};
    
} // namespace pp

#endif
