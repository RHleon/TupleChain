#include <lsp-rule-list.hpp>
#include <common.hpp>

namespace pp {

#ifdef ORDER_LIST
rule_list_t::~rule_list_t()
{
  if (m_head) {
    deleteEntry(m_head);
    delete m_head;
    m_head = nullptr;
  }
}
  
void
rule_list_t::deleteEntry(const rule_list_entry_t& head)
{
  if (head->next) {
    deleteEntry(head->next);
    delete head->next;
    m_head = nullptr;
  }
}

rule_list_entry_t
rule_list_t::insert(const rule_list_entry_t& head, const pc_rule_t& rule)
{
  if (head == nullptr) {
    return new rule_list_entry(rule);
  }
  
  if (head->rule == nullptr) {
    head->rule = rule;
  }
  else if (head->rule->priority < rule->priority) {
    head->next = insert(head->next, head->rule);
    head->rule = rule;
  }
  else {
    head->next = insert(head->next, rule);
  }

  return head;
}

rule_list_entry_t
rule_list_t::remove(const rule_list_entry_t& head, const pc_rule_t& rule)
{
  if (head == nullptr || head->rule == nullptr || head->rule->priority < rule->priority) {
    return head;
  }
  
  if ((*(head->rule)) == (*rule)) {
    auto newHead = head->next;
    delete head;
    return newHead;
  }

  head->next = remove(head->next, rule);
  return head;
}
#endif
  
void
rule_list_t::insert(const pc_rule_t& rule)
{
#ifdef ORDER_LIST
  m_head = insert(m_head, rule);
#else
  push_back(rule);
#endif 
}

void
rule_list_t::remove(const pc_rule_t& rule)
{
#ifdef ORDER_LIST
  m_head = remove(m_head, rule);
#else
  for (auto it = begin(); it != end(); ++ it) {
    if ((**it) == (*rule)) {
      erase(it);
      break;
    } 
  }
#endif
}

void
rule_list_t::search(uint32_t* const& matchFields, pc_rule_t& bestRule)
{
#ifdef ORDER_LIST
  auto head = m_head;
  while (head) {
    const auto& rule = head->rule;
#else
  for (auto it = begin(); it != end(); ++ it) {
    const auto& rule = *it;
#endif
    int fid = rule->nFields;
    while (fid-- > 0 && rule->fields[fid] == (matchFields[fid] & rule->masks[fid]));
    if (fid == -1) {
      UPDATE_BEST_RULE(bestRule, rule);
#ifdef ORDER_LIST
      return;
#endif
    }
#ifdef ORDER_LIST
    head = head->next;
#endif
  }
}

LspRuleList::LspRuleList(const int& nFields, const uint32_t& mask)
  : m_nFields(nFields)
  , m_maskLen((__builtin_popcount(mask)) - 1)
  , m_entries(new rule_list_t[(1u << m_maskLen) << m_maskLen])
{
}

LspRuleList::~LspRuleList()
{
  delete[] m_entries;
}

#define DO_FOR_COVERED_ENTRY(rule, process) {				\
    uint32_t ss, se, ds, de, idx;					\
    getIdxRange(rule->fields[0], rule->masks[0], ss, se);		\
    getIdxRange(rule->fields[1], rule->masks[1], ds, de);		\
    for (uint32_t i = ss; i < se; ++ i) {				\
      for (uint32_t j = ds; j < de; ++ j) {				\
	idx = i << m_maskLen | j;					\
	process;							\
      }									\
    }									\
  }
  
void
LspRuleList::insertRule(const pc_rule_t& rule)
{
  DO_FOR_COVERED_ENTRY(rule, {
      //LOG_INFO("%u: %u\n", idx, (1u << m_maskLen) << m_maskLen);
      m_entries[idx].insert(rule);
    });
}

void
LspRuleList::deleteRule(const pc_rule_t& rule)
{
  DO_FOR_COVERED_ENTRY(rule, {
      m_entries[idx].remove(rule);
    });
}

void
LspRuleList::search(uint32_t* const& matchFields, pc_rule_t& bestRule)
{
  uint32_t src = matchFields[0] >> (32 - m_maskLen);
  uint32_t dst = matchFields[1] >> (32 - m_maskLen);
  uint32_t idx = src << m_maskLen | dst;
  m_entries[idx].search(matchFields, bestRule);
}

void
LspRuleList::getIdxRange(const uint32_t& field, const uint32_t& mask,
			 uint32_t& start, uint32_t& end)
{
  int maskLen = __builtin_popcount(mask);
  LOG_ASSERT(maskLen <= m_maskLen, "invalid mask len: %d VS %d\n", maskLen, m_maskLen);
  start = (field & mask) >> (32 - m_maskLen);
  end = start + (1u << (m_maskLen - maskLen));
}
  
} // namespace pp
