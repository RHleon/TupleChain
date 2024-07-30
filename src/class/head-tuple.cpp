#include <head-tuple.hpp>
#include <tuple-chain.hpp>
#include <common.hpp>

namespace pp {

static std::unordered_map<TupleChain*, LLRBTree<tree_data_t> > globalEmptyTrees;

HeadTuple::HeadTuple(const int& nFields, const std::vector<pc_rule_t>& rules,
		     const HashTable& tupleMap)
  : m_nFields(nFields)
  , m_mask(MEM_ALLOC(nFields, uint32_t))
  , m_table(m_nFields)
{
  LOG_ASSERT(!rules.empty(), "no rules");

  for (int i = 0; i < nFields; ++ i) {
    m_mask[i] = rules[0]->masks[i];
    for (int j = rules.size() - 1; j > 0; -- j) m_mask[i] &= rules[j]->masks[i];
  }

  for (const auto& rule : rules) {
    const auto& tuple = static_cast<Tuple*>(tupleMap.search(rule->masks));
    insert(rule, tuple);
  }
}

HeadTuple::~HeadTuple()
{
  MEM_FREE(m_mask);
  for (auto& entry : m_entries) {
    if (entry) {
      delete entry;
      entry = nullptr;
    }
  }
}

void
HeadTuple::insert(const pc_rule_t& rule, Tuple* const& tuple)
{
  auto entry = static_cast<entry_t>(m_table.search(rule->fields, m_mask));
  if (!entry) {
    entry = new Entry(rule->fields, m_mask, m_nFields);
    m_entries.push_back(entry);
    m_table.insert(entry->key, static_cast<value_t>(entry));
  }
  entry->insert(rule, tuple);
}

const std::unordered_map<TupleChain*, LLRBTree<tree_data_t> >&
HeadTuple::search(const field_t& fields)
{
  auto entry =  static_cast<entry_t>(m_table.search(fields, m_mask));
  return entry ? entry->trees : globalEmptyTrees;
}

uint64_t
HeadTuple::totalMemory(int bytesPerPointer) const
{
  uint64_t res = m_table.totalMemory(bytesPerPointer); // m_table
  res += m_nFields * 4; // m_mask

  for (auto& entry : m_entries) {
    for (auto& tree : entry->trees) {
      auto bytesPerTreeNode = 3 * bytesPerPointer;
      res += (2 * bytesPerPointer + tree.second.size() * bytesPerTreeNode); // trees
    }
  }
  
  return res;  
}

HeadTuple::Entry::Entry(const field_t& fields, const field_t& masks, const int& nFields)
  : key(MEM_ALLOC(nFields, uint32_t))
{
  for (int i = 0; i < nFields; ++ i) key[i] = fields[i] & masks[i];
}

HeadTuple::Entry::~Entry()
{
  MEM_FREE(key);
}

void
HeadTuple::Entry::insert(const pc_rule_t& rule, Tuple* const& tuple)
{
  const auto& mt = static_cast<MarkedTuple*>(tuple);
  auto chain = static_cast<TupleChain*>(mt->getHost());
  auto& tree = trees[chain];
  tree.insert(mt);
}

void
HeadTuple::display()
{
  for (const auto& entry : m_entries) {
    LOG_KEY(entry->key); LOG_DEBUG("%lu trees\n", entry->trees.size());
    for (const auto& tree : entry->trees) {
      LOG_DEBUG("%p ==>\n", tree.first);
      tree.second.display();
    }
  }
}

} // namespace pp
