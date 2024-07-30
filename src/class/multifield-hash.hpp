#ifndef MULTIFIELD_HASH_HPP
#define MULTIFIELD_HASH_HPP

#include <key-value.hpp>
#include <spp.h>
#include <functional>
#include <unordered_map>

namespace pp {

#define HASH_MISS nullptr

typedef std::unordered_map<mf_key_t, value_t, mf_key_hasher> HashBase;
//typedef spp::sparse_hash_map<mf_key_t, value_t, mf_key_hasher> HashBase;

class HashTable : public HashBase
{
public:
  /** 
   * construct a hash table
   * 
   * @param nFields the number of fields, passed from Tuple class
   * @param maxEntries the maximal number of hash entries
   * @param maxLoadFactor rebuild once the load factor exceeds this number
   * 
   * @return 
   */
  HashTable(const int& nFields,
	    int maxEntries = 1024,
	    double maxLoadFactor = 0.8);
  ~HashTable();

public:
  /** 
   * try to insert a key @key
   * if an entry with this key exists, then return the pointer points to the value;
   * otherwise, insert an entry with this key, and return the pointer of the value
   * for example, if the inserting or existing entry is (k, v), return &v 
   * 
   * @param key the key to try
   * 
   * @return the pointer points to the corresponding the value
   */
  value_t*
  insert(const key_t& key);

  /** 
   * insert an hash entry (@key, @value) 
   * if an entry with the same key exists, do nothing
   * 
   * @param key the key to insert
   * @param value the value associated to the key
   * 
   * @return whether insertion is finished (exists or inserted)
   */
  bool
  insert(const key_t& key,
	 const value_t& value);

  void
  erase(const key_t& key);

  /** 
   * search a key @key in the hash table
   * 
   * @param key the key to search
   * 
   * @return the associated value; return HASH_MISS if no hitting entry
   */
  value_t
  search(const key_t& key) const;

  value_t
  search(const key_t& fields, const key_t& masks) const;

  /** 
   * count the number of existing entries
   * 
   * @return the number of existing entries
   */
  int
  count() const;

  uint64_t
  totalMemory(int bytesPerPointer = 4) const;

  typedef std::function<void(const value_t&)> value_traversal_func_t;
  /** 
   * execute @func on every value 
   * 
   * @param func 
   */
  void
  enumerate(const value_traversal_func_t& func);

public:
  /** 
   * display the detail of the hash table; for debug purpose
   * 
   */
  void
  display(std::function<void(const value_t& value)> printValue) const;
  
private:
  const int& m_nFields;
  int  m_nEntries;
  int  m_maxEntries;
  double m_maxLoadFactor;
#ifndef KEY_HAS_MASK
  field_t TEMP_MATCH_KEY;
#endif
};

} // namespace pp

#endif // MULTIFIELD_HASH_HPP
