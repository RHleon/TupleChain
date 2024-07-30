#ifndef HEAD_TUPLE_HPP
#define HEAD_TUPLE_HPP

#include <marked-tuple.hpp>
#include <ll-rb-tree.hpp>
#include <unordered_map>

namespace pp {

class TupleChain;

struct tree_data_t {
  MarkedTuple* tuple;
  tree_data_t(MarkedTuple* const &t) : tuple(t) {};
  tree_data_t(Tuple* const &t) : tuple(static_cast<MarkedTuple*>(t)) {};
  tree_data_t(const tree_data_t& other) : tuple(other.tuple) {};
  tree_data_t(tree_data_t&& other) : tuple(other.tuple) { other.tuple = nullptr; }
  tree_data_t& operator = (const tree_data_t& other)
  {
    tuple = other.tuple; return *this;
  }
  tree_data_t& operator = (tree_data_t&& other)
  {
    tuple = other.tuple; other.tuple = nullptr; return *this;
  }
  bool operator < (const tree_data_t& other)
  {
    return tuple->getOrder() < other.tuple->getOrder();
  }
  bool operator > (const tree_data_t& other)
  {
    return tuple->getOrder() > other.tuple->getOrder();
  }
  bool operator == (const tree_data_t& other)
  {
    return tuple == other.tuple;
  }
};

class HeadTuple
{
public:
  HeadTuple(const int& nFields, const std::vector<pc_rule_t>& rules,
	    const HashTable& tupleMap);
  ~HeadTuple();
  void display();

  struct Entry {
    key_t key;
    std::unordered_map<TupleChain*, LLRBTree<tree_data_t> > trees;
    Entry(const field_t& fields, const field_t& masks, const int& nFields);
    ~Entry();
    void insert(const pc_rule_t& rule, Tuple* const& tuple);
  };
  typedef Entry* entry_t;

  void
  insert(const pc_rule_t& rule, Tuple* const& tuple);

  const std::unordered_map<TupleChain*, LLRBTree<tree_data_t> >&
  search(const field_t& fields);

  virtual uint64_t
  totalMemory(int bytesPerPointer = 4) const;

  const std::vector<entry_t>&
  getEntries() const { return m_entries; }

protected:
  const int& m_nFields;
  field_t m_mask;
  HashTable m_table;
  std::vector<entry_t> m_entries;
};

} // namespace pp

#endif // HEAD_TUPLE_HPP
