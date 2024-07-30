#ifndef PRUNED_FIELD_HPP
#define PRUNED_FIELD_HPP

#include <ll-trie.hpp>
#include <ll-bitmap.hpp>
#include <processing-unit.hpp>

namespace pp {

class PrunedField : public LLTrie::TrieInterface<PrunedField, uint32_t>
{
public:
  static void TEST();
  
public:
  PrunedField(const int& maxTuples, int maxLength = 32,
	      ProcessingUnit::Profiler* const& profiler = nullptr);
  ~PrunedField();

  void
  insertConstruction(const uint32_t& field, const uint32_t& mask, int tupleId);

  void
  insertConstruction(const uint32_t& field, const uint8_t& len, int tupleId);

  void
  insert(const uint32_t& field, const uint32_t& mask, int tupleId);

  uint64_t
  finalizeConstruction(int height);

  bool
  matchField(const uint32_t& field, LLBitMap* const &res);

  void
  display();

public: // implementation of CRPT interfaces
    LLTrie::trie_value_t
    pushValue(const LLTrie::trie_value_t& from, const LLTrie::trie_value_t& to);

    LLTrie::trie_value_t
    eraseValue(const LLTrie::trie_value_t& from, const uint32_t& len) { return 0; };   
  
private:
  const int& m_maxTuples;
  ProcessingUnit::Profiler* const &m_profiler;
  std::vector<LLBitMap*> m_maps;
};

} // namespace pp

#endif // PRUNED_FIELD_HPP
