#include <ec-tss.hpp>
#include <map>
#include <common.hpp>

namespace pp {

  //#define _DEBUG_ExTC

ExChainedTupleSpaceSearch::ExChainedTupleSpaceSearch(int nFields, int depth, int threshold)
  : PacketClassification("ExtendedTupleChains")
  , m_nFields(nFields)
  , m_treeDepth(depth)
  , m_threshold(threshold)
  , m_treeRoot(nullptr)
{
}

ExChainedTupleSpaceSearch::~ExChainedTupleSpaceSearch()
{
  if (m_treeRoot) {
    delete m_treeRoot;
    m_treeRoot = nullptr;
  }
  for (auto& ctss : ctsses) {
    if (ctss) {
      ctss->setProfiler(nullptr);
      delete ctss;
      ctss = nullptr;
    }
  }
}

uint64_t
ExChainedTupleSpaceSearch::finalizeConstruction()
{
  m_treeRoot = new TupleNode(m_nFields, m_threshold);
  for (auto& rule : m_rules) m_treeRoot->insertConstruction(rule);
  LOG_ASSERT(constructionTree(m_treeRoot, m_treeDepth), "invalid construction");

  uint64_t res = m_treeRoot->totalMemory(4); // tree
  for (auto& ctss : ctsses) {
    res += ctss->totalMemory();
  }

  res += 12; // m_nFields + m_treeDepth + m_threshold
  res += 4; // m_treeRoot;

  INIT_PROFILER(profiler);
  USE_PROFILER_C(profiler, {profiler->nTreeNodes ++;}) // the root
  m_treeRoot->tranversal([this] (const TupleNode::entry_t& entry) {
      INIT_PROFILER_C(profiler, ExChainedTupleSpaceSearch);
      if (entry->type == TupleNode::EntryType::NODE) {
	USE_PROFILER_C(profiler, {profiler->nTreeNodes += (entry->u.node != nullptr);});
      }
      if (entry->type == TupleNode::EntryType::LEAF) {
	USE_PROFILER_C(profiler, {profiler->nCTSS += (entry->u.leaf != nullptr);});
      }
      USE_PROFILER_C(profiler, {profiler->nDirectRules += (entry->rule != nullptr);});
    });
  
  return res;
}

void
ExChainedTupleSpaceSearch::search(uint32_t* const& matchFields, pc_rule_t& bestRule)
{
  searchTree(m_treeRoot, matchFields, bestRule);
}

void
ExChainedTupleSpaceSearch::searchTree(TupleNode* const& root,
				      const key_t& matchFields, pc_rule_t& bestRule)
{
  INIT_PROFILER(profiler);
  USE_PROFILER(profiler, {profiler->accessedTreeNodes ++;});
  //LOG_KEY(matchFields);
  //LOG_INFO("## start %d %d\n", bestRule->priority, bestRule->action);
  auto entry = root->search(matchFields, bestRule);
  //LOG_INFO("root %d %d\n", bestRule->priority, bestRule->action);
  while (entry && entry->type == TupleNode::EntryType::NODE) {
    USE_PROFILER(profiler, {profiler->accessedTreeNodes ++;});
    entry = entry->u.node->search(matchFields, bestRule);
    //LOG_INFO("node %d %d\n", bestRule->priority, bestRule->action);
  }

  if (entry && entry->type == TupleNode::EntryType::LEAF) {
    const auto& ctss = static_cast<ChainedTupleSpaceSearch*>(entry->u.leaf);
    USE_PROFILER(profiler, {profiler->accessedCTSS ++;});
    ctss->search(matchFields, bestRule);
    //LOG_INFO("ctss %d %d\n", bestRule->priority, bestRule->action);
  }  
}
    
bool
ExChainedTupleSpaceSearch::constructionTree(TupleNode* const &root, int depth)
{
  if (depth == 0 || !root->reconstruction()) return false;

  auto& entries = root->m_entries;
  for (auto& entry : entries) {
    if (entry->type == TupleNode::EntryType::NODE) {
      if(!constructionTree(entry->u.node, depth - 1)) {
	entry->setLeaf(static_cast<value_t>(constructionChains(entry->u.node)));
      }
    }
  }

  return true;
}

ChainedTupleSpaceSearch*
ExChainedTupleSpaceSearch::constructionChains(TupleNode* const &node)
{
  auto ctss = new ChainedTupleSpaceSearch(m_nFields, m_threshold);
  ctsses.push_back(ctss);
  ctss->setProfiler(m_profiler);
  
  const auto& rules = node->m_rules;
  for (auto& rule : rules) ctss->insertRuleConstruction(rule);
  ctss->finalizeConstruction();

  return ctss;
}

void
ExChainedTupleSpaceSearch::insertRule(const pc_rule_t& rule)
{
  auto ctss = static_cast<ChainedTupleSpaceSearch*>(m_treeRoot->insert(rule));
  if (ctss) {
    ctss->insertRule(rule);
  }
}

void
ExChainedTupleSpaceSearch::deleteRule(const pc_rule_t& rule)
{
  auto ctss = static_cast<ChainedTupleSpaceSearch*>(m_treeRoot->erase(rule));
  if (ctss) {
    ctss->deleteRule(rule);
  }
}

void
ExChainedTupleSpaceSearch::Profiler::dump(FILE* file)
{
  ChainedTupleSpaceSearch::Profiler::dump(file);
  if (!file) {
    LOG_INFO("| tn = %.2f / %d | tc = %.2f / %d | r = %d ",
	     accessedTreeNodes * 1.0f / nPackets, nTreeNodes,
	     accessedCTSS * 1.0f / nPackets, nCTSS, nDirectRules);
  }
}

uint64_t
ExTupleChains::finalizeConstruction()
{
  auto rootMask = MEM_ALLOC(m_nFields, uint32_t);
  auto srcMask = MEM_ALLOC(m_nFields, uint32_t);
  auto dstMask = MEM_ALLOC(m_nFields, uint32_t);
  for (int i = 0; i < m_nFields; ++ i) rootMask[i] = srcMask[i] = dstMask[i] = 0;
  rootMask[0] = srcMask[0] = m_maskThreshold[0];
  rootMask[1] = dstMask[1] = m_maskThreshold[1];

  m_treeRoot = new TupleNode(m_nFields, m_threshold, rootMask);
  m_srcRoot = new TupleNode(m_nFields, m_threshold, srcMask);
  m_dstRoot = new TupleNode(m_nFields, m_threshold, dstMask);

  for (auto& rule : m_rules) {
    switch (checkSpecific(rule)) {
    case 0x3: m_treeRoot->insertConstruction(rule); break;
    case 0x2: m_srcRoot->insertConstruction(rule); break;
    case 0x1: m_dstRoot->insertConstruction(rule); break;
      //default: m_lessSpecificRules.insertRuleConstruction(rule); break;
    default: m_lessSpecificRules.insertRule(rule); break;
    }
  }

  constructionTree(m_treeRoot, m_treeDepth);
  constructionTree(m_srcRoot, m_treeDepth);
  constructionTree(m_dstRoot, m_treeDepth);

  uint64_t res = 0;
  res += m_treeRoot->totalMemory(4);
  res += m_srcRoot->totalMemory(4);
  res += m_dstRoot->totalMemory(4);
  for (auto& ctss : ctsses) {
    res += ctss->totalMemory();
  }

#ifdef _DEBUG_ExTC
  std::map<int, int> tupleSize;
  int totTuples = 0;
  for (auto& ctss : ctsses) {
    for (auto& chain : ctss->getChains()) {
      auto tuple = chain->getFirst();
      while (tuple) {
	tupleSize[tuple->size()] ++;
	totTuples ++;
	tuple = tuple->next;
      }
    }
  }
  for (auto& e : tupleSize) LOG_DEBUG("%d tuples (%.2f) have %d entries\n",
				      e.second, e.second * 100.0f / totTuples, e.first);
#endif
  return res + m_lessSpecificRules.finalizeConstruction();
}
 
void
ExTupleChains::search(uint32_t* const& matchFields, pc_rule_t& bestRule)
{
  searchTree(m_treeRoot, matchFields, bestRule);
  searchTree(m_srcRoot, matchFields, bestRule);
  searchTree(m_dstRoot, matchFields, bestRule);
  m_lessSpecificRules.search(matchFields, bestRule);
}


void
ExTupleChains::insertRule(const pc_rule_t& rule)
{
  const auto& pcRule = static_cast<pc_rule_t>(rule);
  value_t ctss = nullptr;
  
  switch (checkSpecific(pcRule)) {
  case 0x3: ctss = m_treeRoot->insert(pcRule); break;
  case 0x2: ctss = m_srcRoot->insert(pcRule); break;
  case 0x1: ctss = m_dstRoot->insert(pcRule); break;
  default: m_lessSpecificRules.insertRule(pcRule); break;
  }
  if (ctss) {
    static_cast<ChainedTupleSpaceSearch*>(ctss)->insertRule(rule);
  }
}

void
ExTupleChains::deleteRule(const pc_rule_t& rule)
{
  const auto& pcRule = static_cast<pc_rule_t>(rule);
  value_t ctss = nullptr;
  
  switch (checkSpecific(pcRule)) {
  case 0x3: ctss = m_treeRoot->erase(pcRule); break;
  case 0x2: ctss = m_srcRoot->erase(pcRule); break;
  case 0x1: ctss = m_dstRoot->erase(pcRule); break;
  default: m_lessSpecificRules.deleteRule(pcRule); break;
  }
  if (ctss) {
    static_cast<ChainedTupleSpaceSearch*>(ctss)->deleteRule(rule);
  }
}

} // namespace pp

void
pp::ExChainedTupleSpaceSearch::TEST()
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
  ExChainedTupleSpaceSearch tss(globalNumFields, 3, 8);
  tss.initializeMatchProfiler(nPackets);
  tss.constructWithRules(nRules, rules);

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
