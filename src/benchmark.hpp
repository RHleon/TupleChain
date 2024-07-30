#ifndef BENCHMARK_HPP
#define BENCHMARK_HPP

#include <time-measurer.hpp>
#include <packet-io.hpp>
#include <update-manager.hpp>
#include <processing-unit.hpp>
#include <processing-unit-interface.hpp>
#include <string>
#include <cstdio>
#include <tuple>
#include <common.hpp>

namespace pp {

struct BenchmarkParams {
    int data_type;
    std::string rule_file;
    std::string traffic_file;
    std::string update_file;
    bool enable_match;
    bool enable_insert;
    bool enable_delete;
    bool random_traffic;
    bool is_virtual;
    int num_instances;
    int  nBatch;
    int locality;

    BenchmarkParams()
	: rule_file("")
	, traffic_file("")
	, update_file("")
	, random_traffic(false)
	, is_virtual(false)
    {
    }
};

struct BenchmarkStatistic {
    std::string algoName;
    uint64_t totalBytes;
    std::string algorithmMetrics;
    uint64_t totalRules;
    uint64_t constructTime; // ms

    double avgMatchSpeed;
    std::string matchMetrics;
    double avgInsertSpeed;
    double avgDeleteSpeed;
    ProcessingUnit::Profiler* profiler;

    IOStatistic ioStatistic;
    UpdateStatistic updStatistic;

    BenchmarkStatistic(const std::string& name, int nRules, int nPackets)
	: algoName(name)
	, totalBytes(0)
	, totalRules(nRules)
	, algorithmMetrics("")
	, constructTime(0)
	, avgMatchSpeed(0)
	, matchMetrics("")
	, avgInsertSpeed(0)
	, avgDeleteSpeed(0)
	, profiler(nullptr) {};
    
    BenchmarkStatistic() {};

    void
    dump(FILE* statisticFile = NULL);
};
  
class Benchmark
{
public:
  static void TEST(BenchmarkParams params);
  Benchmark(BenchmarkParams params, PacketIO* io = NULL, UpdateManager* um = NULL);
  ~Benchmark();

  Benchmark&
  reload(BenchmarkParams params);

  Benchmark&
  reset(PacketIO* io, UpdateManager* um = NULL);

  BenchmarkStatistic
  run(ProcessingUnit* pu);

public:
  Benchmark&
  displayConfigurations();
  
  void
  matchSimulation(ProcessingUnit* pu, BenchmarkStatistic& statistic);

  void
  updateSimulation(ProcessingUnit* pu, BenchmarkStatistic& statistic);
    
  void
  emulation(ProcessingUnit* pu, BenchmarkStatistic& statistic);

public: // benchmark with CRTP
    template<typename I> BenchmarkStatistic
    run(ProcessingUnitInterface<I>* pu);

    template<typename I> void
    matchSimulation(ProcessingUnitInterface<I>* pu, BenchmarkStatistic& statistic);

    template<typename I> void
    updateSimulation(ProcessingUnitInterface<I>* pu, BenchmarkStatistic& statistic);

    template<typename I> void
    emulation(ProcessingUnitInterface<I>* pu, BenchmarkStatistic& statistic);
    
protected:
  void
  loadConfigurations(BenchmarkParams params);

private:
  BenchmarkParams m_configs;
  TimeMeasurer m_measurer;
  
  PacketIO* m_io;
  UpdateManager* m_um;
  
  rule_t* m_rules; // to construction pu
  packet_t* m_packets; // for simulation
  action_t* m_actions;
  update_t* m_insertions;
  update_t* m_deletions;
  packet_t* m_updatePackets;
  action_t* m_updateActions;

  int m_nRules;
  int m_nPackets;
  int m_nInsertions;
  int m_nDeletions;
  int m_nUpdatePackets;
};

static std::vector<std::string>
splitStrings1(std::string input, const std::string& sep = ",")
{
  //LOG_INFO("to sep %s by %s\n", input.c_str(), sep.c_str());
  std::vector<std::string> res;
  size_t pos = 0;
  while ((pos = input.find(sep, pos)) != std::string::npos) {
    res.push_back(input.substr(0, pos));
    input = input.substr(pos + 1);
  }
  res.push_back(input);
  //for (auto& str : res) LOG_INFO("%s\n", str.c_str());
  return res;
}

template<typename I> BenchmarkStatistic
Benchmark::run(ProcessingUnitInterface<I>* pu)
{
    LOG_ASSERT(pu != nullptr, "invalid processing unit");
    BenchmarkStatistic statistic(pu->getName(), m_nRules, m_nPackets);
    //statistic.profiler = pu->initializeMatchProfiler(m_nPackets);
    //statistic.profiler->bytesPerPointer = 4;

    // construction
    m_measurer.start(); // start measurer

      // construction
      if (m_configs.is_virtual) {
	  std::vector<std::string> ruleFiles = splitStrings1(m_configs.rule_file);
	  ruleFiles.pop_back();
	  int vid = 0;
	  statistic.totalRules = 0;
	  statistic.totalBytes = 0;
	  for (auto file : ruleFiles) {
	      freeRules(m_nRules, m_rules, m_configs.data_type);
	      m_nRules = loadRuleFromFile(file.c_str(), m_rules, m_configs.data_type);
	      statistic.totalRules += m_nRules;
	      std::tie(statistic.totalBytes, statistic.algorithmMetrics) =
		  pu->constructWithRulesV(m_nRules, m_rules, vid ++);
	  }

	  std::vector<std::string> trFiles = splitStrings1(m_configs.traffic_file);
	  trFiles.pop_back();
	  int nPackets = 0;
	  double totalNs = 0;
	  for (int i = 0; i < trFiles.size(); ++ i) {
	      freePackets(m_nPackets, m_packets);
	      m_nPackets = loadTrafficFromFile(trFiles[i].c_str(), m_packets,
					       m_configs.data_type);
	      totalNs += pu->matchTestV(m_nPackets, m_packets, m_actions, i);
	      nPackets += m_nPackets;
	  }

	  statistic.avgMatchSpeed = nPackets * 1000.0f / totalNs;
	  return statistic;
      }
      else {
	  //statistic.profiler->constEnabled = true;
	  LOG_INFO("construct with %d rules ...\n", m_nRules);
	  std::tie(statistic.totalBytes, statistic.algorithmMetrics) =
	      pu->constructWithRulesNew(m_nRules, m_rules);
	  LOG_INFO("DONE\n");
	  statistic.constructTime = m_measurer.stop(TimeUnit::ms); // ms
      }
  
    //statistic.profiler->constEnabled = false;

    if (m_configs.enable_match) {
	    //statistic.profiler->matchEnabled = true;
	    LOG_INFO("match %d packets ...\n", m_nPackets);
	    matchSimulation(pu, statistic);
	    LOG_INFO("DONE\n");
	    //statistic.profiler->matchEnabled = false;
    }

    //statistic.profiler->updateEnabled = true;
    updateSimulation(pu, statistic);
    //statistic.profiler->updateEnabled = false;

    emulation(pu, statistic);
    //if (pu) delete pu;
    return statistic;
}

template<typename I> void
Benchmark::matchSimulation(ProcessingUnitInterface<I>* pu, BenchmarkStatistic& statistic)
{
	// Null actions means random traffic
	if(m_actions == NULL){
    	m_actions = (action_t*)calloc(m_nPackets, sizeof(action_t));
		pu->generateResult(m_nPackets, m_packets, m_rules, m_nRules, m_actions);
	}
		
  assert(m_packets != NULL && m_actions != NULL);
  int nProcessedPackets = pu->adjustResults(m_nPackets, m_packets, m_actions);

  std::tie(statistic.avgMatchSpeed, statistic.matchMetrics) =
      pu->matchTest(m_nPackets, m_packets, m_actions, m_configs.nBatch, m_configs.locality);
  
  // struct test_case_t {
  //     packet_t pkt;
  //     action_t act;
  // };
  // std::vector<test_case_t> test(m_nPackets);
  // for (int i = 0; i < m_nPackets; ++ i) test[i].pkt = m_packets[i];
  
  // m_measurer.start();
  // for(int i = 0; i < m_nPackets; ++ i) test[i].act = pu->processPacket(test[i].pkt);
  // statistic.avgMatchSpeed = nProcessedPackets * 1.0f / m_measurer.stop(TimeUnit::us); // MPPS

  // for (int i = 0; i < m_nPackets; ++ i) {
  //     if (test[i].act != m_actions[i]) {
  // 	  pu->processPacket(test[i].pkt);
  // 	  LOG_ERROR("WRONG at %d: got %u but expect %u", i, test[i].act, m_actions[i]);	  
  //     }
  // }
  
}

template<typename I> void
Benchmark::updateSimulation(ProcessingUnitInterface<I>* pu, BenchmarkStatistic& statistic)
{
  if (m_configs.enable_insert) {
    LOG_INFO("insert %d rules ...", m_nInsertions);
    int nProcessed = 0;
    m_measurer.start();
    for (int i = 0; i < m_nInsertions; ++ i) {
      //LOG_INFO("insert %d: ", i); LOG_PC_RULE(static_cast<pc_rule_t>(m_insertions[i].rule));
      nProcessed += pu->processUpdate(m_insertions[i]);
    }
    auto totNs = m_measurer.stop(TimeUnit::ns);
    statistic.avgInsertSpeed = nProcessed * 1000000.0f / totNs; // KUPS
    LOG_INFO("DONE %5.2f ns per update\n", totNs * 1.0f / nProcessed);
  }
  
  if (m_configs.enable_delete) {
    LOG_INFO("delete %d rules ...", m_nDeletions);
    int nProcessed = 0;
    m_measurer.start();
    for (int i = 0; i < m_nDeletions; ++ i) {
      //LOG_INFO("delete %d: ", i); LOG_PC_RULE(static_cast<pc_rule_t>(m_deletions[i].rule));
      nProcessed += pu->processUpdate(m_deletions[i]);
    }
    auto totNs = m_measurer.stop(TimeUnit::ns);
    statistic.avgDeleteSpeed = nProcessed * 1000000.0f / totNs; // KUPS
    LOG_INFO("DONE %5.2f ns per update\n", totNs * 1.0f / nProcessed);
  }

  if (m_configs.enable_delete && m_configs.enable_insert) {
#ifdef PROFILER_ENABLED
    LOG_INFO("check update effect with %d packets ... \n", m_nUpdatePackets);
    pu->adjustResults(m_nUpdatePackets, m_updatePackets, m_updateActions);

    pu->matchTest(m_nUpdatePackets, m_updatePackets, m_updateActions, 0, 1);
    // for (int i = 0; i < m_nUpdatePackets; ++ i) {
    //   auto result = pu->processPacket(m_updatePackets[i]);
    //   if (result != pu->adjustRoute(m_updateActions[i])) {
    // 	LOG_INFO("invalid update test result [%d] %llx: get %u but expect %u\n",
    // 		 i, *((uint64_t*)(m_updatePackets[i])), result, m_updateActions[i]);
    // 	pu->processPacket(m_updatePackets[i]);
    // 	exit(1);
    //   }
    // }
    LOG_INFO("DONE\n");
#endif
  }
}

template<typename I> void
Benchmark::emulation(ProcessingUnitInterface<I>* pu, BenchmarkStatistic& statistic)
{
#ifdef PROFILER_DISABLED
  int pktBatch = m_configs.nBatch;
  if (pktBatch < 32) pktBatch = 32;
  const int updBatch = 32;
  //packet_t pkts[pktBatch];
  action_t acts[pktBatch];
  update_t* updates = nullptr;
  packet_t* pkts = nullptr;

  int emp_pkts=0;
  
  assert(m_io);

  int i, nPackets, nUpdates, sntPackets = 0, rcvPackets = 0;

  m_io->start();
  if (m_um) m_um->start();
  m_measurer.start();

  while(!globalStopRequested) {
      if (m_um) {
	  updates = m_um->pullUpdates(nUpdates, updBatch);
	  for (i = 0; i < nUpdates; ++ i) {
	      pu->processUpdate(updates[i]);				
	  }
      }									
								
      pkts = m_io->pullPackets(nPackets, pktBatch);	
      for(i = 0; i < nPackets; ++ i) {
            //if(pkts[i] == NULL || pkts[i] == nullptr){
            //    emp_pkts += 1;
            //    continue;
            //}
	        acts[i] = pu->processPacket(pkts[i]);			
      }								
								
      rcvPackets += nPackets;					
      sntPackets += m_io->pushPackets(pkts, acts, nPackets);	
  }

  LOG_INFO("================DGB===================\n");
  LOG_INFO("Empty pkts = %d\n", emp_pkts);

  auto tot = m_measurer.stop(TimeUnit::us); // stop measurer
  LOG_DEBUG("\nrecevied %4.1f K packets (rx = %5.2f)\n"
	    "sent out %4.1f K packets (tx = %5.2f)\n",
	    rcvPackets / 1000.0f, rcvPackets * 1.0f/ tot,
	    sntPackets / 1000.0f, sntPackets * 1.0f / tot);
  if (m_um) m_um->stop();
  m_io->stop();

  statistic.ioStatistic = m_io->getStatistic();
  if (m_um) statistic.updStatistic = m_um->getStatistic();
#endif
}

} // namespace pp

#endif // BENCHMARK_HPP
