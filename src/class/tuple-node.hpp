#ifndef TUPLE_NODE_HPP
#define TUPLE_NODE_HPP

#include <tuple.hpp>
#include <multifield-hash.hpp>

namespace pp {

class TupleNode
{
public:
  TupleNode(const int& nFields, const int& threshold);
  TupleNode(const int& nFields, const int& threshold, const key_t& mask);
  ~TupleNode();

  enum EntryType {
    RULE = 0,
    LEAF,
    NODE
  };
  struct Entry {
    uint8_t type;
    key_t key;
    pc_rule_t rule;
    union {
      TupleNode* node;
      value_t leaf;
    }u;
    Entry(const int& nFields, const key_t& fields, const key_t& mask);
    ~Entry();
    void setLeaf(const value_t& value);
    void setNode(TupleNode* const &node);
  };
  typedef Entry* entry_t;

public:
  virtual uint64_t
  totalMemory(int bytesPerPointer = 4) const;

  void
  tranversal(std::function<void(const entry_t& entry)> func);  
  
  bool
  reconstruction();

  void
  insertConstruction(const pc_rule_t& rule);

  entry_t
  search(const key_t& fields);

  entry_t
  search(const key_t& fields, pc_rule_t& bestRule);

  value_t
  insert(const pc_rule_t& rule);

  value_t
  erase(const pc_rule_t& rule);

  void
  display(std::function<void(const value_t& value)> func);
  
public:
  std::vector<pc_rule_t> m_rules;
  std::vector<entry_t> m_entries;
  
public:
  const int& m_nFields;
  const int& m_threshold;
  key_t m_mask;
  HashTable* m_table;
};
 
inline bool
RULE_MATCH_MASK(const pc_rule_t& rule, const key_t& mask, const int& nFields)
{
  for (int i = 0; i < nFields; ++ i) {
    if ((rule->masks[i] & mask[i]) != mask[i]) {
      return false;
    }
  }
  return true;
}

inline bool
RULE_WITH_MASK(const pc_rule_t& rule, const key_t& mask, const int& nFields)
{
  for (int i = 0; i < nFields; ++ i) {
    if (rule->masks[i] != mask[i]) {
      return false;
    }
  }
  return true;
}

} // namespace pp

#endif // TUPLE_NODE_HPP
