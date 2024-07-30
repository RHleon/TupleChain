#ifndef LSP_RULE_LIST_HPP
#define LSP_RULE_LIST_HPP

#include <packet-classification.hpp>
#include <vector>

namespace pp {

  //#define ORDER_LIST

#ifdef ORDER_LIST
struct rule_list_entry {
  pc_rule_t rule;
  rule_list_entry* next;
  rule_list_entry() : rule(nullptr), next(nullptr) {};
  rule_list_entry(const pc_rule_t& r) : rule(r), next(nullptr) {};
};
typedef rule_list_entry* rule_list_entry_t;
#endif

#ifdef ORDER_LIST
#define INHERIT
#else
#define INHERIT : public std::vector<pc_rule_t>
#endif

#ifdef ORDER_LIST
struct rule_list_t {
  rule_list_entry_t m_head;
  rule_list_t() : m_head(nullptr) {};
  ~rule_list_t();
  void deleteEntry(const rule_list_entry_t& head);
  rule_list_entry_t insert(const rule_list_entry_t& head, const pc_rule_t& rule);
  rule_list_entry_t remove(const rule_list_entry_t& head, const pc_rule_t& rule);
#else
struct rule_list_t : public std::vector<pc_rule_t> {
#endif
  void insert(const pc_rule_t& rule);
  void remove(const pc_rule_t& rule);
  void search(uint32_t* const& matchFields, pc_rule_t& bestRule);
};

class LspRuleList
{
public:
  LspRuleList(const int& nFields, const uint32_t& mask);
  ~LspRuleList();
  void insertRule(const pc_rule_t& rule);
  void deleteRule(const pc_rule_t& rule);
  void search(uint32_t* const& matchFields, pc_rule_t& bestRule);
  uint64_t finalizeConstruction() { return 0; }

public:
  void getIdxRange(const uint32_t& field, const uint32_t& mask,
		   uint32_t& start, uint32_t& end);
  
  const int& m_nFields;
  int m_maskLen;
  rule_list_t* m_entries;
};
  
} // namespace pp

#endif // LSP_RULE_LIST_HPP
