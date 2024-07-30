#include <tuple.hpp>
#include <common.hpp>

namespace pp {

Tuple::Tuple(uint32_t* mask, const int& nFileds, int id)
  : m_nFields(nFileds)
  , m_id(id)
  , m_mask(MEM_ALLOC(m_nFields, uint32_t))
  , m_table(m_nFields)
{
  LOG_ASSERT(m_mask, "can not allocate mask of tuple\n");
  for (int i = 0; i < m_nFields; ++i) {
    m_mask[i] = mask[i];
  }
}

Tuple::~Tuple()
{
  MEM_FREE(m_mask);
}

bool
Tuple::insert(const pc_rule_t& rule)
{
  return m_table.insert(rule->fields, static_cast<const value_t>(rule));
}

bool
Tuple::erase(const pc_rule_t& rule)
{
  auto hitRule = static_cast<pc_rule_t>(m_table.search(rule->fields));
  if (hitRule && (*hitRule) == (*rule)) {
    m_table.erase(rule->fields);
    return true;
  }
  return false;
}

value_t
Tuple::search(const key_t& matchFields) const
{
  return m_table.search(matchFields, m_mask);
}

void
Tuple::enumerate(const HashTable::value_traversal_func_t& func)
{
  m_table.enumerate(func);
}

uint64_t
Tuple::totalMemory(int bytesPerPointer) const
{
  uint64_t res = m_table.totalMemory(bytesPerPointer); // m_table
  res += m_nFields * 4; // m_mask
  return res;
}

void
Tuple::display() const
{
  LOG_DEBUG("[%d] %d entries with %d fields, mask = ", m_id, m_table.count(), m_nFields);
  LOG_MASK(m_mask); LOG_DEBUG("\n");
  m_table.display([] (const value_t& value) {
      LOG_PC_RULE(static_cast<pc_rule_t>(value));
    });
}

bool
Tuple::cover(Tuple* other)
{
  LOG_ASSERT(m_nFields == other->m_nFields,
	     "invalid # of fields: %d VS %d", m_nFields, other->m_nFields);

  for (int i = 0; i < m_nFields; ++ i) {
    if ((m_mask[i] & other->m_mask[i]) != m_mask[i]) {
      return false;
    }
  }

  return true;
}

int
Tuple::size() const
{
  return m_table.count();
}

int
Tuple::getId() const
{
  return m_id;
}

uint32_t*
Tuple::getMask() const
{
  return m_mask;
}
  
} // namespace pp
