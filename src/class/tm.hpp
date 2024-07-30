#ifndef TM_HPP
#define TM_HPP

#include <tss.hpp>
#include <rtss.hpp>
#include <deque>
#include <list>


namespace pp {
 



class TupleMerge: public TupleSpaceSearch
{
public:
  static void TEST();
  
  TupleMerge(int nFields = 2, int maxLength = 32);
  ~TupleMerge();

public:
  virtual int
  insertRuleConstruction(const pc_rule_t& rule) override;

  virtual uint64_t
  finalizeConstruction() override;
  
  virtual action_t
  matchPacket(const packet_t& packet) override;

  virtual void
  insertRule(const pc_rule_t& rule) override;

  virtual void
  deleteRule(const pc_rule_t& rule) override;
  
public: // profiler
  struct Profiler : public TupleSpaceSearch::Profiler
  {
    int accessedListNodes;
    Profiler(int n) : TupleSpaceSearch::Profiler(n), accessedListNodes(0) {};
    virtual void dump(FILE* file = NULL) override;
  };
  DEFINE_PROFILER
  
public:

  void
  display();

private:
  std::vector<std::vector<uint32_t> > dim;
};
} // namespace pp

#endif // TM_HPP
