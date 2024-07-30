#ifndef TSS_PR_HPP
#define TSS_PR_HPP

#include <tss.hpp>
#include <pruned-field.hpp>
#include <vector>

namespace pp {
  
class PrunedTupleSpaceSearch : public TupleSpaceSearch
{
public:
  static void TEST();
  
  PrunedTupleSpaceSearch(int nFields = 2, int height = 4, int maxLength = 32);
  ~PrunedTupleSpaceSearch();

public:
  virtual uint64_t
  finalizeConstruction() override;

  virtual void
  insertRule(const pc_rule_t& rule) override;

  virtual void
  search(uint32_t* const& matchFields, pc_rule_t& bestRule) override;

  inline action_t
  matchPacketFast(const packet_t& pkt)
  {
      auto bestRule = m_defaultRule;
      uint32_t* matchFields = (uint32_t*)pkt;
      MAP_FOR_MATCH->setAllBits();
      for (int i = 0; i < m_actualNumFields; ++i) {
	if (!m_prunedFields[i]->matchField(matchFields[i], MAP_FOR_MATCH)) {
	  return bestRule->action;
	}
      }

      int nSetBits = MAP_FOR_MATCH->getAllSetBits(MAP_SET_BITS);
      for (int i = 0; i < nSetBits; ++i) {
	auto hitRule = static_cast<pc_rule_t>(m_tuples[MAP_SET_BITS[i]]->search(matchFields));
	UPDATE_BEST_RULE(bestRule, hitRule);
      }
  
      return bestRule->action;
  }

public: // profiler
  struct Profiler : public TupleSpaceSearch::Profiler
  {
    int accessedTreeNodes;
    int nTreeNodes;
    Profiler(int n)
      : TupleSpaceSearch::Profiler(n)
      , accessedTreeNodes(0)
      , nTreeNodes(0)
    {
    }
    virtual void dump(FILE* file = NULL) override;
  };
  DEFINE_PROFILER

public:
  void
  insertPrunedField(const pc_rule_t& rule, const int& id);

  void
  display();

private:
  int m_maxTuples;
  int m_treeHeight;
  int m_maxLength;
  int m_actualNumFields;
  std::vector<PrunedField*> m_prunedFields;
  
  LLBitMap* MAP_FOR_MATCH;
  std::vector<int> MAP_SET_BITS;
};

} // namespace pp

#endif // TSS_PR_HPP
