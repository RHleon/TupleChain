#ifndef EC_TSS_HPP
#define EC_TSS_HPP

#include <hc-tss.hpp>
#include <tss-pr.hpp>
#include <tuple-node.hpp>
#include <lsp-rule-list.hpp>

namespace pp {

class ExChainedTupleSpaceSearch : public PacketClassification
{
public:
  static void TEST();
  ExChainedTupleSpaceSearch(int nFields, int depth = 0, int threshold = 0);
  ~ExChainedTupleSpaceSearch();

public:
  virtual uint64_t
  finalizeConstruction() override;

  virtual void
  search(const key_t& matchFields, pc_rule_t& bestRule) override;

  virtual void
  insertRule(const pc_rule_t& rule) override;

  virtual void
  deleteRule(const pc_rule_t& rule) override;

  void
  searchTree(TupleNode* const& root, const key_t& matchFields, pc_rule_t& bestRule);

public:
  public: // profiler
  struct Profiler : public ChainedTupleSpaceSearch::Profiler
  {
    int nTreeNodes;
    int nCTSS;
    int nDirectRules;
    int accessedTreeNodes;
    int accessedCTSS;
    Profiler(int n)
      : ChainedTupleSpaceSearch::Profiler(n)
      , nTreeNodes(0)
      , nCTSS(0)
      , nDirectRules(0)
      , accessedTreeNodes(0)
      , accessedCTSS(0)
    {};
    virtual void dump(FILE* file = NULL) override;
  };
  DEFINE_PROFILER
  
protected:
  bool
  constructionTree(TupleNode* const &root, int depth);

  ChainedTupleSpaceSearch*
  constructionChains(TupleNode* const &node);
  
public:
  int m_nFields;
  int m_treeDepth;
  int m_threshold;
  TupleNode* m_treeRoot;
  std::vector<ChainedTupleSpaceSearch*> ctsses;
};

class ExTupleChains : public ExChainedTupleSpaceSearch
{
public:
  ExTupleChains(int nFields, int depth = 0, int threshold = 0,
		uint32_t maskThreshold = 0, int height = 4)
    : ExChainedTupleSpaceSearch(nFields, depth, threshold)
    , m_maskThreshold(MEM_ALLOC(2, uint32_t))
    , m_lessSpecificRules(m_nFields, maskThreshold)
    , m_srcRoot(nullptr)
    , m_dstRoot(nullptr)
  {
    LOG_ASSERT(maskThreshold != 0, "invalid mask threshold!");
    LOG_DEBUG("construct ExTC with PTS: h-%d m-%x\n", height, maskThreshold);
    this->setName("ExTC with PTS");
    m_maskThreshold[0] = m_maskThreshold[1] = maskThreshold;
  }
  
  ~ExTupleChains()
  {
    MEM_FREE(m_maskThreshold);
  }

public:
  virtual uint64_t
  finalizeConstruction() override;
  
  virtual void
  insertRule(const pc_rule_t& rule) override;

  virtual void
  deleteRule(const pc_rule_t& rule) override;

  virtual void
  search(uint32_t* const& matchFields, pc_rule_t& bestRule) override;
  
public:
  inline uint32_t
  checkSpecific(const pc_rule_t& rule);
  
  field_t m_maskThreshold;
  //PrunedTupleSpaceSearch m_lessSpecificRules;
  LspRuleList m_lessSpecificRules;
  TupleNode* m_srcRoot;
  TupleNode* m_dstRoot;
};

inline uint32_t
ExTupleChains::checkSpecific(const pc_rule_t& rule)
{
  uint32_t res = 0;
  if ((m_maskThreshold[0] & rule->masks[0]) == m_maskThreshold[0]) {
    res |= 0x2;
  }
  if ((m_maskThreshold[1] & rule->masks[1]) == m_maskThreshold[1]) {
    res |= 0x1;
  }
  return res;
}
    
} // namespace pp

#endif // EC_TSS_HPP
