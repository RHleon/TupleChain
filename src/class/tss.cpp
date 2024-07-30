#include <tss.hpp>
#include <common.hpp>
#include <map>

namespace pp {

  //#define _DEBUG_TSS

TupleSpaceSearch::TupleSpaceSearch(int nFields, const std::string& name)
  : PacketClassification(name)
  , m_nFields(nFields)
  , m_tupleMap(m_nFields)
{
}

TupleSpaceSearch::~TupleSpaceSearch()
{
  for (int i = m_tuples.size() - 1; i >= 0; -- i) {
    if (m_tuples[i]) {
      delete m_tuples[i];
      m_tuples[i] = nullptr;
    }
  }
}

int
TupleSpaceSearch::insertRuleConstruction(const pc_rule_t& rule)
{
  Tuple* hitTuple = HASH_MISS;
  value_t* vp = m_tupleMap.insert(rule->masks);
  if (*vp == HASH_MISS) { // need to create a new tuple
    hitTuple = createNewTuple(rule->masks);
    *vp = static_cast<value_t>(hitTuple);
  }
  else { // cast pointer of the existing tuple
    hitTuple = static_cast<Tuple*>(*vp);
  }

  LOG_ASSERT(hitTuple, "can not find/create a tuple");
  if (hitTuple->insert(rule)) {
    m_rules.push_back(rule);
    return hitTuple->getId();
  }
  else {
    return RuleInsertionResult::FAIL;
  }
}

uint64_t
TupleSpaceSearch::finalizeConstruction()
{
#ifdef _DEBUG_TSS
  std::map<int, int> tupleSize;
  for (auto& tuple : m_tuples) {
    tupleSize[tuple->size()] ++;
  }
  LOG_DEBUG("%lu tuples\n", m_tuples.size());
  for (auto& e : tupleSize) {
    LOG_DEBUG("%d tuples (%%%.2f) have %d entries\n",
	      e.second, e.second * 100.0f / m_tuples.size(), e.first);
  }
#endif
  
  INIT_PROFILER(profiler);
  USE_PROFILER_C(profiler, { profiler->nTuples += m_tuples.size(); });

  int bpp = m_profiler ? m_profiler->bytesPerPointer : 32;
  auto res = m_tupleMap.totalMemory(bpp); // tuple map
  for (auto& tuple : m_tuples) {
    res += tuple->totalMemory(bpp); // all tuples
  }
  res += 4; // m_nFields
  return res;
}
  
Tuple*
TupleSpaceSearch::createNewTuple(uint32_t* const &mask)
{  
  auto tuple = new OrderedTuple(mask, m_nFields, m_tuples.size());
  m_tuples.push_back(tuple);
  return tuple;
}
  
void
TupleSpaceSearch::search(uint32_t* const& matchFields, pc_rule_t& bestRule)
{
  INIT_PROFILER(profiler);
  for (const auto& tuple : m_tuples) {
    const auto& ot = static_cast<OrderedTuple*>(tuple);
    if (ot->maxPriority() <= bestRule->priority) continue;
    auto hitRule = static_cast<pc_rule_t>(ot->search(matchFields));
    USE_PROFILER(profiler, {profiler->accessedTuples ++; });
    UPDATE_BEST_RULE(bestRule, hitRule);
  }  
}

void
TupleSpaceSearch::deleteRule(const pc_rule_t& rule)
{
  auto hitTuple = static_cast<Tuple*>(m_tupleMap.search(rule->masks));
  if (hitTuple) {
    hitTuple->erase(rule);
  }
}

void
TupleSpaceSearch::Profiler::dump(FILE* file)
{
  LOG_FILE(file, "| tp = %5.1f / %-6d ", accessedTuples * 1.0f / nPackets, nTuples);
}
  
} // namespace pp


int
pp::TupleSpaceSearch::generateMatchResult(const char* rule_path, const char* traffic_path)
{
  auto readNumFields = [] (const char* path) -> int {
    assert(path);
    FILE* file = fopen(path, "r");				
    if (!file) LOG_ERROR("can not open file: %s\n", path);

    int nFields;
    fscanf(file, "%d", &nFields);
    fclose(file);
    
    return nFields;
  };
  
  std::string result_path = std::string(traffic_path) + ".result";
  FILE* file = fopen(result_path.c_str(), "w");
  if (!file) LOG_ERROR("can not open file to write result: %s\n", result_path.c_str());
  
  pp::rule_t* rules = nullptr;
  pp::packet_t* packets = nullptr;
  int nFields = readNumFields(rule_path);
  assert(nFields == readNumFields(traffic_path));

  int nRules = pp::loadRuleFromFile(rule_path, rules, pp::DataType::MULTI_FIELD);
  int nPackets = pp::loadTrafficFromFile(traffic_path, packets, pp::DataType::MULTI_FIELD);
  assert(rules != nullptr && packets != nullptr);

  TupleSpaceSearch tss(nFields);
  tss.constructWithRules(nRules, rules);

  fprintf(file, "%d\n", nPackets);
  for (int i = 0; i < nPackets; ++ i) {
    fprintf(file, "%u\n", tss.matchPacket(packets[i]));
  }
  fclose(file);

  return nPackets;
}
