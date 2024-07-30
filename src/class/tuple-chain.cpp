#include <tuple-chain.hpp>
#include <tss.hpp>
#include <common.hpp>
#include <map>

namespace pp {

#define CHAINING_TUPLES(x, y) { x->next = y; y->prev = x; y->leaveMarkers(); }

TupleChain::TupleChain(ProcessingUnit::Profiler* const& profiler)
  : m_nTuples(0)
  , m_head(nullptr)
  , m_profiler(profiler)
{
}

bool
TupleChain::insertTuple(Tuple* const &tuple)
{
  LOG_ASSERT(m_nTuples < MAX_TUPLES, "too many tuples!");
  int i = 0; while (i < m_nTuples && m_tuples[i]->cover(tuple)) i++;
  
  // now, every tuple before i coveres [tuple]
  if (i < m_nTuples && !tuple->cover(m_tuples[i])) {
    // can not insert to this chain
    return false;
  }

  for (int j = m_nTuples ++; j > i; j --) {
    m_tuples[j] = m_tuples[j - 1];
    m_tuples[j]->setOrder(j);
  }
  m_tuples[i] = static_cast<MarkedTuple*>(tuple);
  m_tuples[i]->setOrder(i);

  m_tuples[i]->prev = m_tuples[i]->next = nullptr;
  if (i > 0) CHAINING_TUPLES(m_tuples[i - 1], m_tuples[i]);
  if (i < m_nTuples - 1) CHAINING_TUPLES(m_tuples[i], m_tuples[i + 1]);

#if 1
  m_head = m_tuples[0];
  m_head->fail = nullptr;
  m_head->succ = getOptTuple(1, m_nTuples);
#else
  m_head = getOptTuple(0, m_nTuples);
#endif

  auto mt = static_cast<MarkedTuple*>(tuple);
  mt->setHost(this);
  return true;
}

MarkedTuple*
TupleChain::getOptTuple(int start, int end)
{
  if (start >= end) return nullptr;

  int mid = (start + end) / 2;
  auto opt = m_tuples[mid];

  opt->succ = getOptTuple(mid + 1, end);
  opt->fail = getOptTuple(start, mid);
  return opt;
}

pc_rule_t
TupleChain::search(uint32_t* const &fields)
{
  INIT_PROFILER_C(profiler, TupleSpaceSearch);
  pc_rule_t bestRule = nullptr;

  //LOG_DEBUG("TO MATCH: "); LOG_KEY(fields); LOG_DEBUG("\n"); display();
  
  auto tuple = m_head;
  while (tuple) {
    auto hitEntry = tuple->searchEntry(fields);
    USE_PROFILER(profiler, {profiler->accessedTuples ++;});
    //LOG_DEBUG("test %p\n", tuple);
    if (hitEntry) {
      bestRule = hitEntry->rule;
      //if (hitEntry->rule) LOG_PC_RULE(hitEntry->rule);
      tuple = (hitEntry->type & MarkedTuple::EntryType::MARKER) ? tuple->succ : nullptr;
    }
    else {
      tuple = tuple->fail;
    }
  }
  
  return bestRule;
}
  
void
TupleChain::display()
{
  LOG_DEBUG("@@@@ DISPLAY CHAIN @@@@@@\n");
  LOG_DEBUG("%2d tuples: ", m_nTuples);
  LOG_ASSERT(m_tuples[0]->prev == nullptr, "invalid head!");

  LOG_DEBUG("%p(%d)", m_tuples[0], m_tuples[0]->getId());
  for (int i = 1; i < m_nTuples; ++ i) {
    LOG_ASSERT(m_tuples[i - 1]->next == m_tuples[i] && m_tuples[i]->prev == m_tuples[i - 1], "broken");
    LOG_DEBUG(" -- %p(%d)", m_tuples[i], m_tuples[i]->getId());
  }
  LOG_DEBUG("\n");
  LOG_ASSERT(m_tuples[m_nTuples - 1]->next == nullptr, "invalid tail!");

  for (int i = 0; i < m_nTuples; ++ i) {
    m_tuples[i]->display();
  }
}

MarkedTuple*
TupleChain::getHead() const
{
  return m_head;
}

MarkedTuple*
TupleChain::getFirst() const
{
  return m_tuples[0];
}

} // namespace pp
