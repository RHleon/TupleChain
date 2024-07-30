#include <hc-tss.hpp>
#include <common.hpp>
#include <algorithm>

namespace pp {

HeadChainedTupleSpaceSearch::HeadChainedTupleSpaceSearch(int nFields, int threshold)
  : ChainedTupleSpaceSearch(nFields, threshold, "HeadedTupleChains")
  , m_head(nullptr)
{
}

HeadChainedTupleSpaceSearch::~HeadChainedTupleSpaceSearch()
{
  if (m_head) {
    delete m_head;
    m_head = nullptr;
  }
}

uint64_t
HeadChainedTupleSpaceSearch::finalizeConstruction()
{
  auto res = ChainedTupleSpaceSearch::finalizeConstruction();

  if (m_searchChain) {
    m_head = new HeadTuple(m_nFields, m_rules, m_tupleMap);
    res += m_head->totalMemory(m_profiler->bytesPerPointer);
  }

  INIT_PROFILER(profiler);
  USE_PROFILER_C(profiler, {
      const auto& entries = m_head->getEntries();
      for (auto& entry : entries) {
	for (auto& tree : entry->trees) {
	  profiler->nTrees ++;
	  profiler->tuplesInTrees.push_back(tree.second.size());
	}
      }
    });

  return res;
}

void
HeadChainedTupleSpaceSearch::search(const key_t& matchFields, pc_rule_t& bestRule)
{
  INIT_PROFILER(profiler);    
  if (!m_searchChain) {
    MATCH_RULES(m_nFields, matchFields, m_rules, bestRule);
    return;
  }
  //LOG_DEBUG("TO MATCH: "); LOG_KEY(matchFields); LOG_DEBUG("\n");
  
  pc_rule_t hitRule = nullptr;
  const auto& trees = m_head->search(matchFields);
  for (const auto& tree : trees) {
#if 1
    auto root = tree.second.getRoot();
    hitRule = nullptr;
    while (root) {
      USE_PROFILER(profiler, {profiler->accessedTuples ++;});
      const auto& tuple = static_cast<MarkedTuple*>(root->data.tuple);
      auto hitEntry = tuple->searchEntry(matchFields);
      if (hitEntry) {
	hitRule = hitEntry->rule;
	root = (hitEntry->type & MarkedTuple::EntryType::MARKER) ? root->right : nullptr;
      }
      else {
	root = root->left;
      }
    }
#else
    hitRule = tree.first->search(matchFields);
    if (hitRule) {
      auto hitTuple = static_cast<MarkedTuple*>(m_tupleMap.search(hitRule->masks));
      if (hitTuple != tree.second.find(hitTuple->getOrder())) {
	LOG_DEBUG("get matched: %p[%d]\n", hitTuple, hitTuple->getOrder());
	tree.second.display();
	exit(1);
      }
    }
#endif
    UPDATE_BEST_RULE(bestRule, hitRule);
    USE_PROFILER(profiler, {
	profiler->accessedTrees ++;
	if (hitRule) profiler->hitTrees ++;
      });
  }
}

void
HeadChainedTupleSpaceSearch::insertRule(const pc_rule_t& rule)
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

  if (m_head) {
    m_head->insert(rule, tuple);
  }
}

void
HeadChainedTupleSpaceSearch::Profiler::dump(FILE* file)
{
  TupleSpaceSearch::Profiler::dump(file);

  if (tuplesInTrees.empty()) return;
  LOG_FILE(file, "| tc = %4.1f / %6d | ", accessedTrees * 1.0f / nPackets, nTrees);

  LOG_ASSERT(nTrees == tuplesInTrees.size(), "invalid trees");
  sort(tuplesInTrees.begin(), tuplesInTrees.end());
  sort(tupleSize.begin(), tupleSize.end());
  
  if (file) {
    for (int i = 0; i < nTrees; ++ i) {
      LOG_FILE(file, "%d%s", tuplesInTrees[i], i == nTrees - 1 ? " | " : ",");
    }
    for (int i = 0; i < nTuples; ++ i) {
      LOG_FILE(file, "%d%s", tupleSize[i], i == nTuples - 1 ? " " : ",");
    }
  }
  else {
    LOG_INFO("tit [%d:%d:%d] | ts [%d:%d:%d] | tb = %.2f + %.2f",
	     tuplesInTrees.front(), tuplesInTrees[nTrees / 2], tuplesInTrees.back(),
	     tupleSize.front(), tupleSize[nTuples / 2], tupleSize.back(),
	     nHashes * 1.0f / nPackets, nChecks * 1.0f / nPackets);
  }
}

} // namespace pp

void
pp::HeadChainedTupleSpaceSearch::TEST()
{
  rule_t* rules;
  packet_t* packets;
  action_t* results;
#if 0
  int nRules   = loadRuleFromFile("test-data/2_1000.rule", rules);
  int nPackets = loadTrafficFromFile("test-data/2_1000_match.traffic", packets);
  int nResults = loadResultFromFile("test-data/2_1000_match.traffic.result", results);
#else
  int nRules   = loadRuleFromFile("data/rule/2_100000.rule", rules);
  int nPackets = loadTrafficFromFile("data/traffic/2_100000_match.traffic", packets);
  int nResults = loadResultFromFile("data/traffic/2_100000_match.traffic.result", results);
#endif
  LOG_ASSERT(nPackets == nResults, "invalid # of results");

  globalNumFields = 2;
  HeadChainedTupleSpaceSearch tss(globalNumFields, 3);
  tss.constructWithRules(nRules, rules);
  tss.initializeMatchProfiler(nPackets);

  LOG_DEBUG("match %d packets\n", nPackets);
  auto res = MEM_ALLOC(nPackets, action_t);

  TimeMeasurer measurer;
  measurer.start();
  for (int i = 0; i < nPackets; ++i) {
    res[i] = tss.matchPacket(packets[i]);
  }
  auto tot = measurer.stop(TimeUnit::us);
  LOG_INFO("%llu us, %.2f us, %.2f MPPS\n", tot, tot * 1.0f / nPackets, nPackets * 1.0f / tot);

  for (int i = 0; i < nPackets; ++ i) {
    LOG_ASSERT(res[i] == results[i], "ERROR at %d, %d != %d", i, res[i], results[i]);
  }
}
