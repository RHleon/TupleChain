#ifndef C_TSS_HPP
#define C_TSS_HPP

#include <tss.hpp>
#include <tuple-chain.hpp>
#include <vector>

namespace pp {

class ChainedTupleSpaceSearch : public TupleSpaceSearch
{
public:
  static void TEST();
  ChainedTupleSpaceSearch(int nFields = 2, int threshold = 0,
			  const std::string& name = "TupleChains");
  ~ChainedTupleSpaceSearch();
  void display();
  
  void setProfiler(ProcessingUnit::Profiler* const &profiler)
  {
    m_profiler = profiler;
  }

public: // construction
  virtual uint64_t
  finalizeConstruction() override; // from pc

  virtual Tuple*
  createNewTuple(uint32_t* const &mask) override; // from tss

public: // runtime
  virtual void
  insertRule(const pc_rule_t& rule) override;

  virtual void
  search(const key_t& matchFields, pc_rule_t& bestRule) override;

  inline action_t
  matchPacketFast(const packet_t& pkt)
  {
      auto bestRule = m_defaultRule;
      uint32_t* matchFields = (uint32_t*)pkt;

      for (auto& chain : m_chains) {
	auto hitRule = chain->search(matchFields);
	UPDATE_BEST_RULE(bestRule, hitRule);
      }
  
      return bestRule->action;
  }

public: // profiler
  struct Profiler : public TupleSpaceSearch::Profiler
  {
    int comparedRules;
    int hitChains, accessedChains, nChains;
    std::vector<int> tuplesInChain;
    std::vector<int> tupleSize;
    int nHashes, nChecks;
    Profiler(int n)
      : TupleSpaceSearch::Profiler(n)
      , comparedRules(0)
      , hitChains(0)
      , accessedChains(0)
      , nChains(0)
      , nHashes(0)
      , nChecks(0)
    {
    };
    virtual void dump(FILE* file = NULL) override;
  };
  DEFINE_PROFILER
  
public:
  const std::vector<TupleChain*>&
  getChains() const { return m_chains; }

  uint64_t
  totalMemory() const;

public:
  TupleChain*
  createNewChain();
  
  void
  insertTuple(Tuple* const& tuple);
  
  std::vector<TupleChain*> m_chains;
  int m_threshold;
  bool m_searchChain;
};

} // namespace pp

#endif // C_TSS
