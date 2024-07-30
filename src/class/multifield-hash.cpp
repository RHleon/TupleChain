#include <multifield-hash.hpp>
#include <common.hpp>


namespace pp {

HashTable::HashTable(const int& nFields, int maxEntries, double maxLoadFactor)
  : m_nFields(nFields)
  , m_nEntries(0)
  , m_maxEntries(maxEntries)
  , m_maxLoadFactor(maxLoadFactor)
#ifndef KEY_HASH_MASK
  , TEMP_MATCH_KEY(MEM_ALLOC(nFields, uint32_t))
#endif
{
  // alloc memory for hash table
}

HashTable::~HashTable()
{
  // dealloc memory for hash table
#ifndef KEY_HASH_MASK
  MEM_FREE(TEMP_MATCH_KEY);
#endif
}

value_t*
HashTable::insert(const key_t& key)
{
  return &emplace(key, (void*)HASH_MISS).first->second;
}

bool
HashTable::insert(const key_t& key, const value_t& value)
{
  return emplace(key, value).second || true;
}

void
HashTable::erase(const key_t& key)
{
  HashBase::erase(key);
}
  
value_t
HashTable::search(const key_t& key) const
{
  auto it = find(key);
  return it == end() ? HASH_MISS : it->second;
}

value_t
HashTable::search(const key_t& fields, const key_t& masks) const
{
  for (int i = 0; i < m_nFields; ++ i) TEMP_MATCH_KEY[i] = fields[i] & masks[i];
  auto it = find(TEMP_MATCH_KEY);
  return it == end() ? HASH_MISS : it->second;
}

int
HashTable::count() const
{
  return size();
}

uint64_t
HashTable::totalMemory(int bytesPerPointer) const
{
  uint64_t nEntries = size();
  uint64_t totEntries = (double)nEntries / m_maxLoadFactor;
  uint64_t res = totEntries * 2 * bytesPerPointer; // (key, value)
#ifndef KEY_HAS_MASK
  res += m_nFields * 4; // TEMP_MATCH_KEY
#endif
  return res;
}

void
HashTable::enumerate(const value_traversal_func_t& func)
{
  for (auto&& entry : *this) {
    func(entry.second);
  }
}
  
void
HashTable::display(std::function<void(const value_t& value)> printValue) const
{
  LOG_DEBUG("--------------------------------------\n");
  for (auto&& entry : *this) {
    //LOG_SPP_KEY(entry.first);
    printValue(entry.second);
  }
  LOG_DEBUG("--------------------------------------\n");
}
  
} // namespace pp
