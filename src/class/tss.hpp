#ifndef TSS_HPP
#define TSS_HPP

#include <packet-classification.hpp>
#include <tuple.hpp>
#include <vector>

namespace pp {

class TupleSpaceSearch : public PacketClassification
{
public:
  static int
  generateMatchResult(const char* rule_path, const char* traffic_path);
    
public:
  TupleSpaceSearch(int nFields = 2, const std::string& name = "TupleSpaceSearch");
  ~TupleSpaceSearch();

public:
  virtual int
  insertRuleConstruction(const pc_rule_t& rule) override;

  virtual uint64_t
  finalizeConstruction() override;

  virtual void
  search(uint32_t* const& matchFields, pc_rule_t& bestRule) override;

  virtual void
  insertRule(const pc_rule_t& rule) override
  {
    insertRuleConstruction(rule);
  }

  virtual void
  deleteRule(const pc_rule_t& rule) override;

public:
  struct Profiler : public ProcessingUnit::Profiler
  {
    int accessedTuples;
    int nTuples;
    Profiler(int n)
      : ProcessingUnit::Profiler(n)
      , accessedTuples(0)
      , nTuples(0)
    {
    };
    virtual void dump(FILE* file = NULL) override;
  };
  DEFINE_PROFILER;

public:
  virtual Tuple*
  createNewTuple(uint32_t* const &mask);

public:
  int m_nFields;
  HashTable m_tupleMap;
  std::vector<Tuple*> m_tuples;
};
} // namespace pp

#endif // TSS_HPP
