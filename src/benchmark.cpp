#include <benchmark.hpp>
#include <common.hpp>
#include <dummy-io.hpp>
#include <packet-classification.hpp>
#include <tss.hpp>
#include <tss-pr.hpp>
#include <string.h>//strcmp
#include <tuple>

namespace pp {

Benchmark::Benchmark(BenchmarkParams params, PacketIO* io, UpdateManager* um)
  : m_io(io)
  , m_um(um)
  , m_rules(nullptr)
  , m_packets(nullptr)
  , m_actions(nullptr)
  , m_insertions(nullptr)
  , m_deletions(nullptr)
  , m_updatePackets(nullptr)
  , m_updateActions(nullptr)
  , m_nRules(0)
  , m_nPackets(0)
  , m_nInsertions(0)
  , m_nDeletions(0)
  , m_nUpdatePackets(0)
{
  loadConfigurations(params);
}

Benchmark::~Benchmark()
{
  freeRules(m_nRules, m_rules, m_configs.data_type);
  freePackets(m_nPackets, m_packets);
  freeActions(m_nPackets, m_actions);
  freeUpdates(m_nInsertions, m_insertions, m_configs.data_type);
  freeUpdates(m_nDeletions, m_deletions, m_configs.data_type);
#ifdef PROFILER_ENABLED
  freePackets(m_nUpdatePackets, m_updatePackets);
  freeActions(m_nUpdatePackets, m_updateActions);  
#endif
}
  
Benchmark&
Benchmark::reload(BenchmarkParams params)
{
  loadConfigurations(params);
  return *this;
}

Benchmark&
Benchmark::reset(PacketIO* io, UpdateManager* um)
{
  m_io->stop();
  m_io = io;
  m_um = um;

  return *this;
}

BenchmarkStatistic
Benchmark::run(ProcessingUnit* pu)
{
  LOG_ASSERT(pu != nullptr, "invalid processing unit");
  BenchmarkStatistic statistic(pu->getName(), m_nRules, m_nPackets);
  statistic.profiler = pu->initializeMatchProfiler(m_nPackets);
  statistic.profiler->bytesPerPointer = 4;

  m_measurer.start(); // start measurer
  statistic.profiler->constEnabled = true;
  LOG_INFO("construct with %d rules ...", m_nRules);
  statistic.totalBytes = pu->constructWithRules(m_nRules, m_rules);
  LOG_INFO("DONE\n");
  statistic.constructTime = m_measurer.stop(TimeUnit::ms); // ms
  statistic.profiler->constEnabled = false;

  if (m_configs.enable_match) {
    statistic.profiler->matchEnabled = true;
    LOG_INFO("match %d packets ...", m_nPackets);
    matchSimulation(pu, statistic);
    LOG_INFO("DONE\n");
    statistic.profiler->matchEnabled = false;
  }

  statistic.profiler->updateEnabled = true;
  updateSimulation(pu, statistic);
  statistic.profiler->updateEnabled = false;

  emulation(pu, statistic);
  
  return statistic;
}
  
void
Benchmark::matchSimulation(ProcessingUnit* pu, BenchmarkStatistic& statistic)
{
  assert(m_packets != NULL && m_actions != NULL);

  m_measurer.start();
  for(int i = 0; i < m_nPackets; ++ i) {
    auto result = pu->matchPacket(m_packets[i]);
#ifdef PROFILER_ENABLED
    if (result != m_actions[i]) {
      pu->matchPacket(m_packets[i]);
      LOG_ERROR("invalid result %d: %u %u", i, result, m_actions[i]);
    }
#endif
  }

  statistic.avgMatchSpeed = m_nPackets * 1.0f / m_measurer.stop(TimeUnit::us); // MPPS
}

void
Benchmark::updateSimulation(ProcessingUnit* pu, BenchmarkStatistic& statistic)
{
  if (m_configs.enable_insert) {
    LOG_INFO("insert %d rules ...", m_nInsertions);
    m_measurer.start();
    for (int i = 0; i < m_nInsertions; ++ i) {
      //LOG_INFO("insert %d: ", i); LOG_PC_RULE(static_cast<pc_rule_t>(m_insertions[i].rule));
      pu->processUpdate(m_insertions[i]);
    }
    statistic.avgInsertSpeed = m_nInsertions * 1000.0f / m_measurer.stop(TimeUnit::us); // KUPS
    LOG_INFO("DONE\n");
  }
  
  if (m_configs.enable_delete) {
    LOG_INFO("delete %d rules ...", m_nDeletions);
    m_measurer.start();
    for (int i = 0; i < m_nDeletions; ++ i) {
      //LOG_INFO("delete %d: ", i); LOG_PC_RULE(static_cast<pc_rule_t>(m_deletions[i].rule));
      pu->processUpdate(m_deletions[i]);
    }
    statistic.avgDeleteSpeed = m_nDeletions * 1000.0f / m_measurer.stop(TimeUnit::us); // KUPS
    LOG_INFO("DONE\n");
  }

  if (m_configs.enable_delete && m_configs.enable_insert) {
#ifdef PROFILER_ENABLED
    LOG_INFO("check update effect with %d packets ... ", m_nUpdatePackets);
    for (int i = 0; i < m_nUpdatePackets; ++ i) {
      auto result = pu->matchPacket(m_updatePackets[i]);
      if (result != m_updateActions[i]) {
	LOG_INFO("invalid update test result [%d] %llx: %u %u\n",
		 i, *((uint64_t*)(m_updatePackets[i])), result, m_updateActions[i]);
	pu->matchPacket(m_updatePackets[i]);
	exit(1);
      }
    }
    LOG_INFO("DONE\n");
#endif
  }
}

void
Benchmark::emulation(ProcessingUnit* pu, BenchmarkStatistic& statistic)
{
#ifdef PROFILER_DISABLED
  const int pktBatch = 32;
  const int updBatch = 20000;
  packet_t pkts[pktBatch];
  action_t acts[pktBatch];
  update_t* updates = nullptr;
  
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
								
      nPackets = m_io->pullPackets(pkts, pktBatch);		
      for(i = 0; i < nPackets; ++ i) {				
	  acts[i] = pu->matchPacket(pkts[i]);			
      }								
								
      rcvPackets += nPackets;					
      sntPackets += m_io->pushPackets(pkts, acts, nPackets);	
  }

  auto tot = m_measurer.stop(TimeUnit::us); // stop measurer
  if (m_um) m_um->stop();
  m_io->stop();

  statistic.ioStatistic = m_io->getStatistic();
  if (m_um) statistic.updStatistic = m_um->getStatistic();
#endif
}

void
Benchmark::loadConfigurations(BenchmarkParams params)
{
#define NEED_RELOAD(p, q, x) (!p.x.empty() && (q.x != p.x))

  LOG_DEBUG("load benchmark configurations ...");
  m_configs.data_type = params.data_type;
  m_configs.enable_match = params.enable_match;
  m_configs.enable_insert = params.enable_insert;
  m_configs.enable_delete = params.enable_delete;
  m_configs.nBatch = params.nBatch;
  m_configs.random_traffic = params.random_traffic;
  m_configs.locality = params.locality;
  m_configs.num_instances = params.num_instances;
  m_configs.is_virtual = params.is_virtual;

  if (NEED_RELOAD(params, m_configs, rule_file) && (!params.is_virtual)) {
    m_configs.rule_file = params.rule_file;

    freeRules(m_nRules, m_rules, params.data_type);
    m_nRules = loadRuleFromFile(params.rule_file.c_str(), m_rules, params.data_type);
  }
  else {
      m_configs.rule_file = params.rule_file;
  }

  if (NEED_RELOAD(params, m_configs, traffic_file) && (!params.is_virtual)) {
    m_configs.traffic_file = params.traffic_file;

    freePackets(m_nPackets, m_packets);
    m_nPackets = loadTrafficFromFile(params.traffic_file.c_str(), m_packets, params.data_type);

    freeActions(m_nPackets, m_actions);
	
	//random traffic does not have any result till so far
	if(!m_configs.random_traffic){
    	auto result_file = params.traffic_file + ".result";
    	assert(loadResultFromFile(result_file.c_str(), m_actions, params.data_type) == m_nPackets);
	}

  }
  else {
      m_configs.traffic_file = params.traffic_file;
  }

  bool needUpdate = m_configs.enable_insert || m_configs.enable_delete;
  if (needUpdate && NEED_RELOAD(params, m_configs, update_file)) {
    m_configs.update_file = params.update_file;
    
    freeUpdates(m_nInsertions, m_insertions, params.data_type);
    freeUpdates(m_nDeletions, m_deletions, params.data_type);

    std::tie(m_nInsertions, m_nDeletions)
      = loadUpdatesFromFile(params.update_file.c_str(), m_insertions, m_deletions,
			    params.data_type, m_nRules);

#ifdef PROFILER_ENABLED
    auto tr = params.update_file + ".traffic";
    auto res = tr + ".result";
    freePackets(m_nUpdatePackets, m_updatePackets);
    m_nUpdatePackets = loadTrafficFromFile(tr.c_str(), m_updatePackets, params.data_type);

    freeActions(m_nUpdatePackets, m_updateActions);
    assert(loadResultFromFile(res.c_str(), m_updateActions, params.data_type) == m_nUpdatePackets);    
#endif
  }
  
  LOG_DEBUG("END\n");
}

//#define _DEBUG_CONFIG_
Benchmark&
Benchmark::displayConfigurations()
{
  auto makeDataTypeStr = [] (int type, int n) -> std::string {
    switch(type) {
    case DataType::MULTI_FIELD: return std::to_string(globalNumFields) + "-fields";
    case DataType::FIVE_TUPLE:  return "5-tuple";
    case DataType::IPV4:        return "ipv4";
    case DataType::IPV6:        return "ipv6";
    default:                    return "UNSUPPORTED";
    }
  };
  LOG_INFO("Benchmark Configurations:\tDataType: %-8s, Options: %s%s%s\n",
	   makeDataTypeStr(m_configs.data_type, globalNumFields).c_str(),
	   m_configs.enable_match ? "[M]" : "M",
	   m_configs.enable_insert ? "[I]" : "I",
	   m_configs.enable_delete ? "[D]" : "D");

  if (m_nRules) {
    LOG_DEBUG("\t%d Rules loaded from: %s\n", m_nRules, m_configs.rule_file.c_str());
  }
  if (m_nPackets) {
    LOG_DEBUG("\t%d packets loaded from %s\n",
	     m_nPackets, m_configs.traffic_file.c_str());

    if(!m_configs.random_traffic){
      LOG_DEBUG("\t%d actions loaded from %s.result\n",
	      m_nPackets, m_configs.traffic_file.c_str());
    }
  }
  if (m_nInsertions) {
    LOG_DEBUG("\t%d insertions loaded from %s\n", m_nInsertions, m_configs.update_file.c_str());
  }
  if (m_nDeletions) {
    LOG_DEBUG("\t%d deletions loaded from %s\n", m_nDeletions, m_configs.update_file.c_str());
  }
  if (m_nUpdatePackets) {
    LOG_DEBUG("\t%d packets for update test loaded from %s.traffic\n"
	     "\t%d actions for update test loaded from %s.traffic.result\n",
	     m_nUpdatePackets, m_configs.update_file.c_str(),
	     m_nUpdatePackets, m_configs.update_file.c_str());
  }

#ifdef _DEBUG_CONFIG_
  if (m_configs.data_type == DataType::MULTI_FIELD) {
    LOG_DEBUG("--RULE----------------------------------------------------\n");
    pc_rule_t* pcRules = (pc_rule_t*)m_rules;
    for (int i = 0; i < m_nRules; ++i) LOG_PC_RULE(pcRules[i]);
    
    LOG_DEBUG("--PACKET & RESULT------------------------------------------\n");
    int nFields = pcRules[0]->nFields;
    for (int i = 0; i < m_nPackets; ++i) {
      LOG_DEBUG("%3u ", m_actions[i]);
      uint32_t* p = (uint32_t*)m_packets[i];
      for (int j = 0; j < nFields; ++j) {
	LOG_DEBUG("%08x ", *(p + j));
      }
      LOG_DEBUG("\n");
    }

    LOG_DEBUG("--INSERT------------------------------------------\n");
    for (int i = 0; i < m_nInsertions; ++ i) {
      LOG_DEBUG("INS "); LOG_PC_RULE(static_cast<pc_rule_t>(m_insertions[i].rule));
    }
    LOG_DEBUG("--DELETE------------------------------------------\n");
    for (int i = 0; i < m_nDeletions; ++ i) {
      LOG_DEBUG("DEL "); LOG_PC_RULE(static_cast<pc_rule_t>(m_deletions[i].rule));
    }

    LOG_DEBUG("--PACKET & RESULT FOR UPDATES----------------------\n");
    for (int i = 0; i < m_nUpdatePackets; ++i) {
      LOG_DEBUG("%3u ", m_updateActions[i]);
      uint32_t* p = (uint32_t*)m_updatePackets[i];
      for (int j = 0; j < nFields; ++j) {
	LOG_DEBUG("%08x ", *(p + j));
      }
      LOG_DEBUG("\n");
    }
  }
#endif

  return *this;
}

void
BenchmarkStatistic::dump(FILE* statisticFile)
{
  // print important metrics in one line
  LOG_FILE(statisticFile, "| %20s | %6.2f MB | %5.1f B | %s ",
	   algoName.c_str(), GET_MB(totalBytes), totalBytes * 1.0f / totalRules,
	   algorithmMetrics.c_str());
  if (avgMatchSpeed != 0.0f) {
      LOG_FILE(statisticFile, "| %5.2f MPPS ", avgMatchSpeed);
      if (statisticFile) LOG_FILE(statisticFile, "| %s ", matchMetrics.c_str())
  }
  else {
    LOG_FILE(statisticFile, "| ----- MPPS ");
  }
  if (avgInsertSpeed != 0.0f) {
    LOG_FILE(statisticFile, "| %7.1f KUPS ", avgInsertSpeed);
  }
  else {
    LOG_FILE(statisticFile, "| ------- KUPS ");
  }
  if (avgDeleteSpeed != 0.0f) {
    LOG_FILE(statisticFile, "| %7.1f KUPS ", avgDeleteSpeed);
  }
  else {
    LOG_FILE(statisticFile, "| ------- KUPS ");
  }
#ifdef PROFILER_DISABLED
  LOG_FILE(statisticFile,
	   "| ins = %7.1f KUPS | del = %7.1f KUPS | tx = %6.2f MPPS | rx = %6.3f MPPS ",
	   updStatistic.insRate, updStatistic.delRate,
	   ioStatistic.txRate, ioStatistic.rxRate);
#endif
  
#ifdef PROFILER_ENABLED
  if(profiler && statisticFile) profiler->dump(statisticFile);
#endif

  LOG_FILE(statisticFile, "| \n");
}

} // namespace pp

void pp::Benchmark::TEST(pp::BenchmarkParams params)
{
  DummyIO io({
    .traffic_file = params.traffic_file,
    .data_type = params.data_type,
    .tx_rate = 1, // MPPS
    .max_io_events = 10000
  });

  pp::Benchmark benchmark(params, (PacketIO*)&io); 
  benchmark.displayConfigurations();

  pp::PacketClassification classifier("classifier");
  benchmark.run((pp::ProcessingUnit*)&classifier).dump();
}
