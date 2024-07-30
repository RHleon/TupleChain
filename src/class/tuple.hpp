#ifndef TUPLE_HPP
#define TUPLE_HPP

#include <multifield-hash.hpp>
#include <packet-classification.hpp>
#include <priority-heap.hpp>
#include <vector>

namespace pp {
 
class Tuple
{ 
public:
  Tuple(uint32_t* mask, const int& nFileds, int id = 0);
  Tuple(const Tuple& other);
  ~Tuple();

public:
  virtual bool
  insert(const pc_rule_t& rule);

  virtual bool
  erase(const pc_rule_t& rule);

  virtual value_t
  search(const key_t& matchFields) const;

  virtual void
  enumerate(const HashTable::value_traversal_func_t& func);

  virtual uint64_t
  totalMemory(int bytesPerPointer = 4) const;

  int
  size() const;

public:
  void
  display() const;

  bool
  cover(Tuple* other);

  int
  getId() const;

  uint32_t*
  getMask() const;

public:
  const int& m_nFields;
  int m_id;
  uint32_t* m_mask;
  HashTable m_table;
};

class OrderedTuple : public Tuple
{
public:
  OrderedTuple(uint32_t* mask, const int& nFields, int id = 0)
    : Tuple(mask, nFields, id) {};
  
  virtual bool
  insert(const pc_rule_t& rule)
  {
    if (Tuple::insert(rule)) {
      m_orderedPriorities.insert(rule->priority);
      return true;
    }
    return false;
  }

  virtual bool
  erase(const pc_rule_t& rule)
  {
    if (Tuple::erase(rule)) {
      m_orderedPriorities.erase(rule->priority);
      return true;
    }
    return false;
  }
  
  const int&
  maxPriority() const
  {
    return m_orderedPriorities.max;
  }

protected:
 priority_heap_t m_orderedPriorities; 
};

inline bool
IS_ZERO_MASK(const key_t& mask, const int& nFields)
{
  for (int i = 0; i < nFields; ++ i) {
    if (mask[i] != 0) return false;
  }
  return true;
}

} // namespace pp

#endif // TUPLE_HPP
