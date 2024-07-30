#include <pruned-field.hpp>
#include <common.hpp>

namespace pp {

PrunedField::PrunedField(const int& maxTuples, int maxLength,
			 ProcessingUnit::Profiler* const& profiler)
    : m_profiler(profiler)
    , m_maxTuples(maxTuples)
    , m_maps(1, new LLBitMap(m_maxTuples))
{
}
  
PrunedField::~PrunedField()
{
  for (int i = 0; i < m_maps.size(); ++i) {
    if (m_maps[i]) delete m_maps[i];
    m_maps[i] = nullptr;
  }
}

void
PrunedField::insertConstruction(const uint32_t& field, const uint32_t& mask, int tupleId)
{
  this->insertPrefixConstruction(field, __builtin_popcount(mask),
	LLTrie::trie_value_t(tupleId, LLTrie::TRIE_CUSTOM_VALUE_TYPE));
}

void
PrunedField::insertConstruction(const uint32_t& field, const uint8_t& len, int tupleId)
{
  this->insertPrefixConstruction(field, len,
	LLTrie::trie_value_t(tupleId, LLTrie::TRIE_CUSTOM_VALUE_TYPE));
}

uint64_t
PrunedField::finalizeConstruction(int height)
{
  auto res = this->reconstruction(height); // field trie
  res += m_maps.size() * ((m_maxTuples + 7) >> 3); // bitmaps 
  return res;
}

void
PrunedField::insert(const uint32_t& field, const uint32_t& mask, int tupleId)
{
  this->insertPrefix(field, __builtin_popcount(mask),
		     LLTrie::trie_value_t(tupleId, LLTrie::TRIE_CUSTOM_VALUE_TYPE));
}

bool
PrunedField::matchField(const uint32_t& field, LLBitMap* const &res)
{
  auto value = this->longestPrefixMatch(field);
  if (value.s.type == LLTrie::TRIE_CUSTOM_VALUE_TYPE) {
    return res->intersect(value.s.info);
  }
  return value.s.info != 0 && res->intersect(*m_maps[value.s.info]);
}

void
PrunedField::display()
{
  this->displayTree();
  LOG_DEBUG("-----------------------------------\n");
  for (int i = 0; i < m_maps.size(); ++ i) {
    LOG_DEBUG("[%3d] %p: ", i, m_maps[i]);
    m_maps[i]->print();
  }
  LOG_DEBUG("-----------------------------------\n");
}

LLTrie::trie_value_t
PrunedField::pushValue(const LLTrie::trie_value_t& from, const LLTrie::trie_value_t& to)
{
#define MERGE_TRIE_VALUE(m, x) \
  x.s.type == LLTrie::TRIE_CUSTOM_VALUE_TYPE ? m.setBit(x.s.info) : m |= *m_maps[x.s.info]

  m_maps.push_back(new LLBitMap(m_maxTuples));
  auto& map = *m_maps.back();
  
  MERGE_TRIE_VALUE(map, from);
  MERGE_TRIE_VALUE(map, to);

  return m_maps.size() - 1;
}
} // namespace pp

void
pp::PrunedField::TEST()
{
    struct trie_key_t {uint32_t addr; uint8_t len;};
    
  auto test = [] (const std::vector<trie_key_t>& key, int height) {
    int nTuples = 8;
    PrunedField pField(nTuples, 5);
    for (int i = 0; i < key.size(); ++ i) {
      pField.insertConstruction(key[i].addr, key[i].len, i);
    }
    pField.finalizeConstruction(height);

    LLBitMap res(nTuples);
    for (int i = 0; i < key.size(); ++ i) {
      res.setAllBits();

      LOG_DEBUG("%08x --> ", key[i].addr);
      if (pField.matchField(key[i].addr, &res)) {
	res.print();
      }
      else {
	LOG_DEBUG(" FAILE\n");
      }
    }
  };

  std::vector<trie_key_t> keys = {
    {0xacffffff, 1}, {0xacffffff, 2}, {0xacffffff, 5}, {0x88ffffff, 5},
    {0x00ffffff, 1}, {0xacffffff, 5}, {0xacffffff, 4}, {0xacffffff, 4}
  };
  test(keys, -4);
}
