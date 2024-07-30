#include <c-tss.hpp>
#include <common.hpp>
#include <graph-algorithm.hpp>
#include <algorithm>

namespace pp {

    //#define _DEBUG_TC

ChainedTupleSpaceSearch::ChainedTupleSpaceSearch(int nFields, int threshold,
						 const std::string& name)
  : TupleSpaceSearch(nFields, name)
  , m_threshold(threshold)
  , m_searchChain(false)
{
}

ChainedTupleSpaceSearch::~ChainedTupleSpaceSearch()
{
  for (int i = 0; i < m_chains.size(); ++ i) {
    if (m_chains[i]) {
      delete m_chains[i];
      m_chains[i] = nullptr;
    }
  }
}
  
uint64_t
ChainedTupleSpaceSearch::finalizeConstruction()
{
  m_searchChain = m_rules.size() > m_threshold;
  if (!m_searchChain) return 0;

  // construct graph
  std::vector<GraphEdge> edges;
  int nNodes = m_tuples.size();
  for (int i = 0; i < nNodes; ++ i) {
    for (int j = 0; j < nNodes; ++ j) {
      if (i != j && m_tuples[i]->cover(m_tuples[j])) {
	edges.push_back(GraphEdge(i, j));
      }
    }
  }

  //LOG_INFO("%3lu tuples have %3lu edges: --> ", m_tuples.size(), edges.size());
  auto paths = minimumPathCovering(m_tuples.size(), edges);
  //LOG_INFO("%3lu chains\n", paths.size());

  // construct chains
  m_chains = std::vector<TupleChain*>(paths.size(), nullptr);
  for (int i = paths.size() - 1; i >= 0; -- i) {
    m_chains[i] = createNewChain();
    for (int j = paths[i].size() - 1; j >= 0; -- j) {
      m_chains[i]->insertTuple(m_tuples[paths[i][j]]);
    }
  }

#ifdef _DEBUG_TC
  for (auto& chain : m_chains) {
    auto head = chain->getFirst();
    LOG_DEBUG("%d: ", head->size()); LOG_MASK(head->getMask());
  }
#endif
  return totalMemory();
}

uint64_t
ChainedTupleSpaceSearch::totalMemory() const
{
  INIT_PROFILER(profiler);
  USE_PROFILER_C(profiler, {
      profiler->nChains += m_chains.size();
      for (auto& chain : m_chains) {
	profiler->tuplesInChain.push_back(chain->size());
      }
      profiler->nTuples += m_tuples.size();
      for (auto& tuple : m_tuples) {
	profiler->tupleSize.push_back(tuple->size());
      }
    });
  
  uint64_t res = 5; // m_nFields + m_searchChain
  if (m_searchChain) {
    for (auto& tuple : m_tuples) {
      res += tuple->totalMemory(4); // marked tuples
    }
    res += m_chains.size() * 4; // head
  }
  else {
    res += m_rules.size() * 4; // rules
  }

  return res; 
}
    
Tuple*
ChainedTupleSpaceSearch::createNewTuple(uint32_t* const &mask)
{
  Tuple* tuple = new MarkedTuple(mask, m_nFields, m_threshold, m_profiler, m_tuples.size());
  m_tuples.push_back(tuple);
  return tuple;
}

void
ChainedTupleSpaceSearch::search(const key_t& matchFields, pc_rule_t& bestRule)
{
  INIT_PROFILER(profiler);
  if (!m_searchChain) {
      //LOG_INFO("++++ %d %d\n", m_threshold, m_rules.size());
    MATCH_RULES(m_nFields, matchFields, m_rules, bestRule);
    USE_PROFILER(profiler, { profiler->comparedRules += m_rules.size(); })
    return;
  }
  
  for (auto& chain : m_chains) {
    auto hitRule = chain->search(matchFields);
    UPDATE_BEST_RULE(bestRule, hitRule);
    USE_PROFILER(profiler, {
	profiler->accessedChains ++;
	if (hitRule) profiler->hitChains ++;
      });
  }
}

void
ChainedTupleSpaceSearch::insertRule(const pc_rule_t& rule)
{
  auto tuple = static_cast<Tuple*>(m_tupleMap.search(rule->masks));
  if (tuple == HASH_MISS) {
    tuple = createNewTuple(rule->masks);
    m_tupleMap.insert(rule->masks, static_cast<value_t>(tuple));
    insertTuple(tuple);
  }

  if (tuple->insert(rule)) {
    m_rules.push_back(rule);
  }
}
  
void
ChainedTupleSpaceSearch::insertTuple(Tuple* const& tuple)
{
  for (auto& chain : m_chains) {
    if (chain->insertTuple(tuple)) {
      return;
    }
  }

  m_chains.push_back(createNewChain());
  m_chains.back()->insertTuple(tuple);
}

TupleChain*
ChainedTupleSpaceSearch::createNewChain()
{
  return new TupleChain(m_profiler);
}

void
ChainedTupleSpaceSearch::display()
{
  LOG_DEBUG("%p %lu chains\n", this, m_chains.size());
  for (auto& chain : m_chains) chain->display();
}

void
ChainedTupleSpaceSearch::Profiler::dump(FILE* file)
{
  TupleSpaceSearch::Profiler::dump(file);

  if (tuplesInChain.empty()) return;
  nChains = tuplesInChain.size();
  LOG_FILE(file, "| tc = %4.1f / %6d | ", accessedChains * 1.0f / nPackets, nChains);

  sort(tuplesInChain.begin(), tuplesInChain.end());
  sort(tupleSize.begin(), tupleSize.end());
  if (file) {
    for (int i = 0; i < nChains; ++ i) {
      LOG_FILE(file, "%d%s", tuplesInChain[i], i == nChains - 1 ? " | " : ",");
    }
    for (int i = 0; i < nTuples; ++ i) {
      LOG_FILE(file, "%d%s", tupleSize[i], i == nTuples - 1 ? " " : ",");
    }
  }
  else {
    LOG_INFO("tic [%d:%d:%d] | ts [%d:%d:%d] | tb = %.2f + %.2f ",
	     tuplesInChain.front(), tuplesInChain[nChains /2], tuplesInChain.back(),
	     tupleSize.front(), tupleSize[nTuples / 2], tupleSize.back(),
	     nHashes * 1.0f / nPackets, nChecks * 1.0f / nPackets);
  }
}

} // namespace pp

void
pp::ChainedTupleSpaceSearch::TEST()
{
  rule_t* rules;
  packet_t* packets;
  action_t* results;
#if 0
  int nRules   = loadRuleFromFile("test-data/2_8.rule", rules);
  int nPackets = loadTrafficFromFile("test-data/2_8_match.traffic", packets);
  int nResults = loadResultFromFile("test-data/2_8_match.traffic.result", results);
#else
  int nRules   = loadRuleFromFile("data/rule/2_100000.rule", rules);
  int nPackets = loadTrafficFromFile("data/traffic/2_100000_match.traffic", packets);
  int nResults = loadResultFromFile("data/traffic/2_100000_match.traffic.result", results);
#endif
  LOG_ASSERT(nPackets == nResults, "invalid # of results");

  globalNumFields = 2;
  ChainedTupleSpaceSearch tss(globalNumFields);
  tss.initializeMatchProfiler(nPackets);
  tss.constructWithRules(nRules, rules);

  LOG_DEBUG("match %d packets\n", nPackets);
  auto res = MEM_ALLOC(nPackets, action_t);

  TimeMeasurer measurer;
  measurer.start();
  for (int i = 0; i < nPackets; ++i) {
    res[i] = tss.matchPacketFast(packets[i]);
  }
  auto tot = measurer.stop(TimeUnit::us);
  LOG_INFO("%llu us, %.2f us, %.2f MPPS\n", tot, tot * 1.0f / nPackets, nPackets * 1.0f / tot);

  for (int i = 0; i < nPackets; ++ i) {
    LOG_ASSERT(res[i] == results[i], "ERROR at %d, %d != %d", i, res[i], results[i]);
  }
}
