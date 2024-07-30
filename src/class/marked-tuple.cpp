#include <marked-tuple.hpp>
#include <c-tss.hpp>
#include <common.hpp>

namespace pp {

#define HAS_MARKER(entry) (entry->type & EntryType::MARKER)
#define HAS_RULE(entry) (entry && (entry->type & EntryType::RULE))
#define SET_RULE(entry) (entry->type |= EntryType::RULE)

#define LOG_MARKED_ENTRY(value) {					\
    auto entry = static_cast<entry_t>(value);				\
    LOG_KEY(entry->key);						\
    LOG_DEBUG("type-%d, %lu markers", entry->type, entry->owners.size()); \
    if (entry->rule) { LOG_PC_RULE(entry->rule); } else {LOG_DEBUG("\n");} \
  }

MarkedTuple::MarkedTuple(uint32_t* mask, const int& nFields, const int& threshold,
			 ProcessingUnit::Profiler* &profiler, int id)
  : Tuple(mask, nFields, id)
  , m_threshold(threshold)
  , m_searchTable(false)
  , m_profiler(profiler)
  , prev(nullptr)
  , next(nullptr)
  , succ(nullptr)
  , fail(nullptr)
  , m_host(nullptr)
{
}

MarkedTuple::~MarkedTuple()
{
  for (int i = 0; i < m_entries.size(); ++ i) {
    if (m_entries[i]) {
      delete m_entries[i];
      m_entries[i] = nullptr;
    }
  }
}

MarkedTuple::entry_t
MarkedTuple::searchEntry(const key_t& matchFields)
{
  INIT_PROFILER_C(profiler, ChainedTupleSpaceSearch);
  if (m_searchTable) {
    USE_PROFILER(profiler, { profiler->nHashes ++; });
    return static_cast<entry_t>(m_table.search(matchFields, m_mask));
  }
  else {
    int fid;
    for (auto& entry : m_entries) {
      for (fid = 0; fid < m_nFields; ++ fid) {
	if (entry->key[fid] != (matchFields[fid] & m_mask[fid])) break;
      }
      USE_PROFILER(profiler, { profiler->nChecks ++; });
      if (fid == m_nFields) return entry;
    }
    return nullptr;
  }
}
  
bool
MarkedTuple::insert(const pc_rule_t& rule)
{
  auto entry = static_cast<entry_t>(m_table.search(rule->fields));
  if (!entry) {
    entry = new Entry(rule, m_nFields);
    m_entries.push_back(entry);
    if (!m_searchTable && m_entries.size() > m_threshold) m_searchTable = true;
    m_table.insert(rule->fields, static_cast<value_t>(entry));
    if (prev) prev->insertMarker(entry);
  }
  else {
    entry->setRule(rule);
  }

  return true;
}

bool
MarkedTuple::erase(const pc_rule_t& rule)
{
  auto entry = static_cast<entry_t>(m_table.search(rule->fields));
  if (HAS_RULE(entry) && (*(entry->rule)) == *rule) {
    entry->rule = nullptr;
    entry->type -= EntryType::RULE;

    entry_t marker = prev ? static_cast<entry_t>(prev->search(entry->key)) : entry;
    entry->refresh(marker->rule);

    if (entry->owners.empty()) {
      m_table.erase(rule->fields);
    }
    return true;
  }
  
  return false;
}

uint64_t
MarkedTuple::totalMemory(int bytesPerPointer) const
{
  auto res = Tuple::totalMemory(bytesPerPointer); // basic tuple
  res += m_entries.size() * (1 + bytesPerPointer); // entry: type + rule
  res += 2 * bytesPerPointer; // succ + fail
  return res;
}

void
MarkedTuple::leaveMarkers()
{
  if (prev == nullptr) return;
 
  for (auto& entry : m_entries) {
    entry->refresh(prev->insertMarker(entry));
  }
}

const pc_rule_t&
MarkedTuple::insertMarker(const entry_t& owner)
{
  auto entry = static_cast<entry_t>(m_table.search(owner->key, m_mask));
  if (entry) {
    entry->addMarker(owner);
    return entry->rule;
  }

  entry = new Entry(owner, m_nFields, m_mask);
  m_table.insert(entry->key, static_cast<value_t>(entry));
  m_entries.push_back(entry);
  if (!m_searchTable && m_entries.size() > m_threshold) m_searchTable = true;
  if (prev) entry->rule = prev->insertMarker(entry);

  return entry->rule;
}

void
MarkedTuple::display()
{
  LOG_DEBUG("%d <- [%d] | (%d, %d): ",
	    prev ? prev->getId() : -1, m_id,
	    succ ? succ->getId() : -1, fail ? fail->getId() : -1);
  LOG_MASK(m_mask); LOG_DEBUG("\n");
  m_table.display([] (const value_t& value) {
      LOG_MARKED_ENTRY(static_cast<entry_t>(value));
    });
}

MarkedTuple::Entry::Entry(const pc_rule_t& r, int nFields)
  : type(EntryType::RULE)
  , rule(r)
  , key(MEM_ALLOC(nFields, uint32_t))
{
  for (int i = 0; i < nFields; ++ i) key[i] = r->fields[i];
}
  
MarkedTuple::Entry::Entry(Entry* const &entry, int nFields, const field_t& mask)
  : type(EntryType::MARKER)
  , rule(nullptr)
  , key(MEM_ALLOC(nFields, uint32_t))
  , owners(1, entry)
{
  for (int i = 0; i < nFields; ++ i) key[i] = entry->key[i] & mask[i];
}

MarkedTuple::Entry::~Entry()
{
  MEM_FREE(key);
}

void
MarkedTuple::Entry::refresh(const pc_rule_t& r)
{
  if (type != EntryType::MARKER) return;
  
  rule = r;
  for (auto& entry : owners) {
    entry->refresh(rule);
  }
}

void
MarkedTuple::Entry::addMarker(Entry* const &entry)
{
  type |= EntryType::MARKER;
  owners.push_back(entry);
}

bool
MarkedTuple::Entry::setRule(const pc_rule_t& r)
{
  bool res = r && (type & EntryType::RULE) == 0;
  type |= EntryType::RULE;
  rule = r;
  refresh(rule);
  return res;
}

} // namespace pp
