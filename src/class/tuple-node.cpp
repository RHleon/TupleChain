#include <tuple-node.hpp>
#include <ec-tss.hpp>
#include <common.hpp>

namespace pp {

TupleNode::TupleNode(const int& nFields, const int& threshold)
  : m_nFields(nFields)
  , m_threshold(threshold)
  , m_mask(MEM_ALLOC(nFields, uint32_t))
  , m_table(nullptr)
{
  for (int i = 0; i < m_nFields; ++ i) m_mask[i] = 0xffffffff;
}

TupleNode::TupleNode(const int& nFields, const int& threshold, const key_t& mask)
  : m_nFields(nFields)
  , m_threshold(threshold)
  , m_mask(mask)
  , m_table(nullptr)
{
}

TupleNode::~TupleNode()
{
  MEM_FREE(m_mask);
  for (auto& entry : m_entries) {
    if (entry) {
      delete entry;
      entry = nullptr;
    }
  }
  if (m_table) {
    delete m_table;
    m_table = nullptr;
  }
}

uint64_t
TupleNode::totalMemory(int bytesPerPointer) const
{
  uint64_t res = 0;
  if (m_table) res += m_table->totalMemory(bytesPerPointer); // m_table
  res += m_nFields * 4; // m_mask
  res += m_rules.size() * bytesPerPointer; // rest rules;
  res += m_entries.size() * (1 + bytesPerPointer + bytesPerPointer); // entry: t + r + p

  for (auto& entry : m_entries) {
    if (entry->type == EntryType::NODE && entry->u.node) {
      res += entry->u.node->totalMemory(bytesPerPointer);
    }
  }

  return res;
}

void
TupleNode::tranversal(std::function<void(const entry_t& entry)> func)
{
  for (auto& entry : m_entries) {
    func(entry);
    if (entry->type == EntryType::NODE && entry->u.node && entry->u.node) {
      entry->u.node->tranversal(func);
    }    
  }
}
  
bool
TupleNode::reconstruction()
{
  // make sure that empty rules will not trigger reconstruction
  if (IS_ZERO_MASK(m_mask, m_nFields)) return false;
  if (m_rules.size() < m_threshold) return true;
  
  m_table = new HashTable(m_nFields);
  LOG_ASSERT(m_table, "can not allocate table");

  for (const auto& rule : m_rules) insertConstruction(rule);
  m_rules.clear();

  return true;
}

void
TupleNode::insertConstruction(const pc_rule_t& rule)
{
  if (m_table == nullptr) {
    for (int i = 0; i < m_nFields; ++ i) m_mask[i] &= rule->masks[i];
    m_rules.push_back(rule);
    return;
  }

  auto entry = search(rule->fields);
  if (!entry) {
    entry = new Entry(m_nFields, rule->fields, m_mask);
    m_entries.push_back(entry);
    m_table->insert(entry->key, static_cast<value_t>(entry));
  }
  
  if (RULE_WITH_MASK(rule, m_mask, m_nFields)) {
    entry->rule = rule;
  }
  else {
    if (entry->type == EntryType::RULE) entry->setNode(new TupleNode(m_nFields, m_threshold));
    entry->u.node->insertConstruction(rule);
  }
}

TupleNode::entry_t
TupleNode::search(const key_t& fields)
{
  return static_cast<entry_t>(m_table->search(fields, m_mask));
}

TupleNode::entry_t
TupleNode::search(const key_t& fields, pc_rule_t& bestRule)
{
  if (!m_rules.empty()) {
    MATCH_RULES(m_nFields, fields, m_rules, bestRule);
  }

  if (m_table != nullptr) {
    auto entry = static_cast<entry_t>(m_table->search(fields, m_mask));
    if(entry) UPDATE_BEST_RULE(bestRule, entry->rule);
    return entry;
  }

  return nullptr;
}

value_t
TupleNode::insert(const pc_rule_t& rule)
{
  if (!RULE_MATCH_MASK(rule, m_mask, m_nFields) || m_table == nullptr) {
    m_rules.push_back(rule);
    return nullptr;
  }

  auto entry = search(rule->fields);
  if (!entry) {
    entry = new Entry(m_nFields, rule->fields, m_mask);
    m_entries.push_back(entry);
    m_table->insert(entry->key, static_cast<value_t>(entry));
  }

  if (RULE_WITH_MASK(rule, m_mask, m_nFields)) {
    entry->rule = rule;
    return nullptr;
  }

  switch (entry->type) {
  case EntryType::RULE: entry->setNode(new TupleNode(m_nFields, m_threshold));
  case EntryType::NODE: return entry->u.node->insert(rule);
  case EntryType::LEAF: return entry->u.leaf;
  }

  return nullptr;
}

value_t
TupleNode::erase(const pc_rule_t& rule)
{
  if (!RULE_MATCH_MASK(rule, m_mask, m_nFields) || m_table == nullptr) {
    for (auto it = m_rules.begin(); it != m_rules.end(); it ++) {
      if ((**it) == (*rule)) {
	m_rules.erase(it);
	break;
      }
    }
    return nullptr;
  }

  auto entry = search(rule->fields);
  if (!entry) {
    return nullptr;
  }

  if (RULE_WITH_MASK(rule, m_mask, m_nFields)) {
    entry->rule = nullptr;
    return nullptr;
  }

  switch (entry->type) {
  case EntryType::NODE: return entry->u.node->erase(rule);
  case EntryType::LEAF: return entry->u.leaf;
  }

  return nullptr;
}

TupleNode::Entry::Entry(const int& nFields, const key_t& fields, const key_t& mask)
  : type(EntryType::RULE)
  , key(MEM_ALLOC(nFields, uint32_t))
  , rule(nullptr)
{
  for (int i = 0; i < nFields; ++ i) key[i] = fields[i] & mask[i];
}

TupleNode::Entry::~Entry()
{
  if (type == EntryType::NODE) {
    if (u.node) {
      delete u.node;
      u.node = nullptr;
    }
  }
  if (key) {
    delete key;
    key = nullptr;
  }
}

void
TupleNode::Entry::setLeaf(const value_t& value)
{
  if (type == EntryType::NODE) {
    if (u.node) {
      delete u.node;
      u.node = nullptr;
    }
  }
  
  type = EntryType::LEAF;
  u.leaf = value;
}

void
TupleNode::Entry::setNode(TupleNode* const &node)
{
  type = EntryType::NODE;
  u.node = node;
}

void
TupleNode::display(std::function<void(const value_t& value)> func)
{
  LOG_DEBUG("%p: %lu %lu ", this, m_rules.size(), m_entries.size());
  LOG_MASK(m_mask); LOG_DEBUG("\n");

  for (auto& entry : m_entries) {
    LOG_DEBUG("\n###########################\n");
    LOG_MASK(entry->key); LOG_DEBUG(" ==> ");
    if (entry->type == EntryType::LEAF) { LOG_DEBUG("leaf: "); func(entry->u.leaf); }
    if (entry->type == EntryType::NODE) { LOG_DEBUG("node: %p\n", entry->u.node); }
    if (entry->rule) LOG_PC_RULE(entry->rule);
  }
}

} // namespace pp
