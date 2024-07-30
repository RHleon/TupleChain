#include <tm.hpp>
#include <common.hpp>

namespace pp {

  
const uint32_t bound[4] = {0xFFFFFFFF, 0xFFFFFF00, 0xFFFF0000, 0xFF000000};
const uint32_t tound[4] = {0xFFFFFFF0, 0xFFFFF800, 0xFFFC0000, 0xFE000000};

uint32_t cal(uint32_t d) {
  if (d >= bound[0])  return tound[0];
  else if (d >= bound[1]) return tound[1];
  else if (d >= bound[2]) return tound[2];
  else if (d >= bound[3]) return tound[3];
  else return d;
}


TupleMerge::TupleMerge(int nFields, int maxLength)
	: TupleSpaceSearch(nFields, "TupleMerge")
{

}

TupleMerge::~TupleMerge()
{

}

int
TupleMerge::insertRuleConstruction(const pc_rule_t& rule)
{
  for (auto tuple: m_tuples) {
	const auto& rt = static_cast<RTuple*>(tuple);
	if (rt->check(rule)) {
		rt->insert(rule);
		return rt->getId();
	}
  }
  uint32_t *mask = (uint32_t*)malloc(sizeof(uint32_t)*m_nFields);
  for (int i = 0; i < m_nFields; i++) {
	mask[i] = cal(rule->masks[i]);
  }
  m_tuples.push_back(new RTuple(mask, m_nFields, 0, m_profiler));
  int id = m_tuples.size() - 1;
  const auto& rt = static_cast<RTuple*>(m_tuples[id]);
  rt->insert(rule);
  return m_tuples[id]->getId();
  
}

uint64_t
TupleMerge::finalizeConstruction()
{
  INIT_PROFILER(profiler);
  USE_PROFILER_C(profiler, { profiler->nTuples = m_tuples.size(); });
  
  uint64_t res = 4; // m_nFields
  //for (auto& tuple : m_tuples) res += tuple->totalMemory(m_profiler->bytesPerPointer);
  for (auto vec : dim) for (auto& e : vec) res += 4; // dims
  return res;
}

action_t
TupleMerge::matchPacket(const packet_t& packet)
{
  INIT_PROFILER(profiler);
  auto bestRule = m_defaultRule;
  uint32_t* matchFields = (uint32_t*)packet;
  
  for (auto& tuple : m_tuples) {
    const auto& rt = static_cast<RTuple*>(tuple);
    if (rt->maxPriority() <= bestRule->priority) continue;
    auto hitRule = static_cast<pc_rule_t>(rt->search(matchFields));
    USE_PROFILER(profiler, {profiler->accessedTuples ++; });
    UPDATE_BEST_RULE(bestRule, hitRule);
  }

  return bestRule->action;

}

void
TupleMerge::insertRule(const pc_rule_t& rule)
{
  for (auto tuple: m_tuples) {
	const auto& rt = static_cast<RTuple*>(tuple);
	if (rt->check(rule)) {
		rt->insert(rule);

	}
  }
}
void
TupleMerge::deleteRule(const pc_rule_t& rule)
{
  return;
}

void
TupleMerge::Profiler::dump(FILE* file)
{
  TupleSpaceSearch::Profiler::dump(file);
  LOG_FILE(file, "| tl = %.2f ", accessedListNodes * 1.0f / nPackets);
}

void
TupleMerge::display()
{
  LOG_DEBUG("# of tuples: %lu\n", m_tuples.size());
  for (int i = 0; i < m_tuples.size(); ++i) {
    LOG_DEBUG("###### field_%d #######\n", i);
    m_tuples[i]->display();
  }
}

} // namespace pp


void
pp::TupleMerge::TEST()
{
  rule_t* rules;
  packet_t* packets;
  action_t* results;
  int nRules   = loadRuleFromFile("test-data/2_10000.rule", rules);
  int nPackets = loadTrafficFromFile("test-data/2_10000_match.traffic", packets);
  int nResults = loadResultFromFile("test-data/2_10000_match.traffic.result", results);
  LOG_ASSERT(nPackets == nResults, "invalid # of results");
  globalNumFields = 2;
  printf("now break\n");
  TupleMerge tss(globalNumFields, 5);
  tss.constructWithRules(nRules, rules);
  tss.display();

  LOG_DEBUG("match %d packets\n", nPackets);
  for (int i = 0; i < nPackets; ++i) {
    LOG_ASSERT(tss.matchPacket(packets[i]) == results[i], "ERROR at %d", i);
  }
}
