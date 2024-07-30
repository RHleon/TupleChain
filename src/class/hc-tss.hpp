#ifndef HC_TSS_HPP
#define HC_TSS_HPP

#include <c-tss.hpp>
#include <head-tuple.hpp>

namespace pp {

class HeadChainedTupleSpaceSearch : public ChainedTupleSpaceSearch
{
public:
  static void TEST();
  HeadChainedTupleSpaceSearch(int nFields, int threshold = 0);
  ~HeadChainedTupleSpaceSearch();

public:
  virtual uint64_t
  finalizeConstruction() override;

  virtual void
  search(const key_t& matchFields, pc_rule_t& bestRule) override;

  virtual void
  insertRule(const pc_rule_t& rule) override;

public:// profiler
  struct Profiler : public ChainedTupleSpaceSearch::Profiler
  {
    int hitTrees, accessedTrees, nTrees;
    std::vector<int> tuplesInTrees;
    Profiler(int n)
      : ChainedTupleSpaceSearch::Profiler(n)
      , hitTrees(0)
      , accessedTrees(0)
      , nTrees(0)
    {
    };
    virtual void dump(FILE* file = NULL) override;
  };
  DEFINE_PROFILER

protected:
  HeadTuple* m_head;
};

} // namespace pp

#endif // HC_TSS_HPP
