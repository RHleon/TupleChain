#include <tss-pr.hpp>
#include <common.hpp>
#include <rule-traffic.hpp>

namespace pp {

PrunedTupleSpaceSearch::PrunedTupleSpaceSearch(int nFields, int height, int maxLength)
  : TupleSpaceSearch(globalNumFields, "PrunedTupleSearch")
  , m_actualNumFields(nFields)
  , m_maxTuples(0)
  , m_treeHeight(height)
  , m_maxLength(maxLength)
  , m_prunedFields(m_actualNumFields, nullptr)
  , MAP_FOR_MATCH(nullptr)
{
  LOG_DEBUG("construct PTS with h-%d and l-%d, %d fileds but %d trees\n",
	    height, maxLength, m_nFields, m_actualNumFields);
}

PrunedTupleSpaceSearch::~PrunedTupleSpaceSearch()
{
  if (MAP_FOR_MATCH) {
    delete MAP_FOR_MATCH;
    MAP_FOR_MATCH = nullptr;
  }
  
  for (int i = 0; i < m_actualNumFields; ++i) {
    if (m_prunedFields[i]) {
      delete m_prunedFields[i];
      m_prunedFields[i] = nullptr;
    }
  }
}

uint64_t
PrunedTupleSpaceSearch::finalizeConstruction()
{
  m_maxTuples = m_tuples.size() * 1.1;
  for (int i = 0; i < m_actualNumFields; ++i) {
    m_prunedFields[i] = new PrunedField(m_maxTuples, m_maxLength, m_profiler);
  }
  MAP_FOR_MATCH = new LLBitMap(m_maxTuples);
  MAP_SET_BITS = std::vector<int>(m_maxTuples, -1);
  
  int tuple_id = m_tuples.size() - 1;
  auto insert = [this, &tuple_id] (const value_t& value) {
    insertPrunedField(static_cast<const pc_rule_t>(value), tuple_id);
  };

  while (tuple_id >= 0) {
    m_tuples[tuple_id]->enumerate(insert);
    tuple_id --;
  }

  auto res = TupleSpaceSearch::finalizeConstruction(); // tuples and others
  for (int i = 0; i < m_actualNumFields; ++i) {
    res += m_prunedFields[i]->finalizeConstruction(-m_treeHeight); // pruned fileds
  }
  res += MAP_FOR_MATCH->size(); // MAP_FOR_MATCH
  res += m_maxTuples * 4; // MAP_SET_BITS
  
  return res;
}

void
PrunedTupleSpaceSearch::insertPrunedField(const pc_rule_t& rule, const int& id)
{
  for (int i = 0; i < m_actualNumFields; ++i) {
    m_prunedFields[i]->insertConstruction(rule->fields[i], rule->masks[i], id);
  }
}

void
PrunedTupleSpaceSearch::search(uint32_t* const& matchFields, pc_rule_t& bestRule)
{
  INIT_PROFILER(profiler);
  MAP_FOR_MATCH->setAllBits();
  for (int i = 0; i < m_actualNumFields; ++i) {
    if (!m_prunedFields[i]->matchField(matchFields[i], MAP_FOR_MATCH)) {
      return;
    }
  }
  
  int nSetBits = MAP_FOR_MATCH->getAllSetBits(MAP_SET_BITS);
  for (int i = 0; i < nSetBits; ++i) {
    auto hitRule = static_cast<pc_rule_t>(m_tuples[MAP_SET_BITS[i]]->search(matchFields));
    USE_PROFILER(profiler, { profiler->accessedTuples ++;});
    UPDATE_BEST_RULE(bestRule, hitRule);
  }
}

void
PrunedTupleSpaceSearch::insertRule(const pc_rule_t& rule)
{
  int tupleId = insertRuleConstruction(rule);
  for (int i = 0; i < m_actualNumFields; ++i) {
    m_prunedFields[i]->insert(rule->fields[i], rule->masks[i], tupleId);
  }
}

void
PrunedTupleSpaceSearch::Profiler::dump(FILE* file)
{
  TupleSpaceSearch::Profiler::dump(file);
  LOG_FILE(file, "| tn = %4.1f / %6d ", accessedTreeNodes * 1.0f / nPackets, nTreeNodes);
}
  
void
PrunedTupleSpaceSearch::display()
{
  LOG_DEBUG("# of tuples: %lu / %d, # of fields: %d\n",
	    m_tuples.size(), m_maxTuples, m_actualNumFields);
  for (int i = 0; i < m_actualNumFields; ++i) {
    LOG_DEBUG("###### field_%d #######\n", i);
    m_prunedFields[i]->display();
  }
}
  
} // namespace pp

void
pp::PrunedTupleSpaceSearch::TEST()
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
  PrunedTupleSpaceSearch tss(globalNumFields, 4, 5);
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
