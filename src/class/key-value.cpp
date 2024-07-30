#include <common.hpp>
#include <spp.h>

namespace pp {

#ifdef KEY_HAS_MASK
static field_t globalDefaultMasks = nullptr;
const field_t&
getDefaultMasks()
{
  if (globalDefaultMasks == nullptr) {
    globalDefaultMasks = MEM_ALLOC(globalNumFields, uint32_t);
    for (int i = 0; i < globalNumFields; ++ i) globalDefaultMasks[i] = 0xffffffff;
  }
  return globalDefaultMasks;
}
#endif
  
mf_key_t::mf_key_t(const field_t& fields)
  : m_fields(fields)
#ifdef KEY_HAS_MASK
  , m_masks(getDefaultMasks())
#endif
{
}

#ifdef KEY_HAS_MASK
mf_key_t::mf_key_t(const field_t& fields, const field_t& masks)
  : m_fields(fields)
  , m_masks(masks)
{
}
#endif

mf_key_t::mf_key_t(const mf_key_t& other)
  : m_fields(other.m_fields)
#ifdef KEY_HAS_MASK
  , m_masks(other.m_masks)
#endif
{
}

mf_key_t::mf_key_t(mf_key_t&& other)
  : m_fields(other.m_fields)
#ifdef KEY_HAS_MASK
  , m_masks(other.m_masks)
#endif
{
  MEM_FREE(other.m_fields);
#ifdef KEY_HAS_MASK
  MEM_FREE(other.m_masks);
#endif
}

mf_key_t&
mf_key_t::operator = (const mf_key_t& other)
{
  m_fields = other.m_fields;
#ifdef KEY_HAS_MASK
  m_masks = other.m_masks;
#endif
  return *this;
}

mf_key_t&
mf_key_t::operator = (mf_key_t&& other)
{
  m_fields = other.m_fields;
  MEM_FREE(other.m_fields);
#ifdef KEY_HAS_MASK  
  m_masks = other.m_masks;
  MEM_FREE(other.m_masks);
#endif
  return *this;
}

bool
mf_key_t::operator == (const mf_key_t& other) const
{
  for (int i = 0; i < globalNumFields; ++i) {
#ifdef KEY_HAS_MASK
    if ((m_fields[i] & m_masks[i]) != (other.m_fields[i] & other.m_masks[i])) {
#else
    if (m_fields[i] != other.m_fields[i]) {
#endif
      return false;
    }
  }
  return true;  
}

inline uint32_t  
hash_add(uint32_t hash, uint32_t data) {
    data *= 0xcc9e2d51;
    data = (data << 15) | (data >> (32 - 15));
    data *= 0x1b873593;
    hash = hash ^ data;
    hash = (hash << 13) | (hash >> (32 - 13));
    return hash * 5 + 0xe6546b64;
}

inline uint32_t 
hash_finish(uint32_t hash, uint32_t final) {
    hash ^= final;
    hash ^= hash >> 16;
    hash *= 0x85ebca6b;
    hash ^= hash >> 13;
    hash *= 0xc2b2ae35;
    hash ^= hash >> 16;
    return hash;
}
 
uint32_t
mf_key_hasher::operator()(const mf_key_t& key) const
{
  int nFields = globalNumFields;
#ifdef KEY_HAS_MASK
  size_t seed = 0;
  while (nFields --) spp::hash_combine(seed, key.m_fields[nFields] & key.m_masks[nFields]);
#else
  uint32_t seed = 0;
  //while (nFields --) spp::hash_combine(seed, key.m_fields[nFields]);
  while (nFields --) seed = hash_add(seed, key.m_fields[nFields]); hash_finish(seed, 16);
#endif
  return seed;
}
  
} // namespace pp
