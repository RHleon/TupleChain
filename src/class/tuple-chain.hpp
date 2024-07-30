#ifndef TUPLE_CHAIN_HPP
#define TUPLE_CHAIN_HPP

#include <marked-tuple.hpp>
#include <vector>

namespace pp {

#define MAX_TUPLES 128

class TupleChain
{
public:
  TupleChain(ProcessingUnit::Profiler* const& profiler = nullptr);
  
  bool
  insertTuple(Tuple* const &tuple);

  pc_rule_t
  search(uint32_t* const &fields);

  void
  display();

  MarkedTuple*
  getHead() const;

  MarkedTuple*
  getFirst() const;

  int
  size() const { return m_nTuples; }

protected:
  virtual MarkedTuple*
  getOptTuple(int start, int end);
  
  MarkedTuple* m_tuples[MAX_TUPLES];
  MarkedTuple* m_head;
  int m_nTuples;
  ProcessingUnit::Profiler* const& m_profiler;
};

} // namespace pp

#endif // TUPLE_CHAIN_HPP
