#ifndef KEY_VALUE_HPP
#define KEY_VALUE_HPP

#include <cinttypes>
#include <logger.hpp>

namespace pp {

typedef uint32_t* key_t;
typedef void*     value_t;
typedef uint32_t* field_t;

//#define KEY_HAS_MASK
struct mf_key_t
{
  field_t m_fields; // will never alloc memory for it
#ifdef KEY_HAS_MASK
  field_t m_masks; // will never alloc memory for it
#endif

  mf_key_t(const field_t& fields);
#ifdef KEY_HAS_MASK
  mf_key_t(const field_t& fields, const field_t& masks);
#endif
  mf_key_t(const mf_key_t& other);
  mf_key_t(mf_key_t&& other);

  mf_key_t& operator =  (const mf_key_t& other);
  mf_key_t& operator =  (mf_key_t&& other);
  bool      operator == (const mf_key_t &other) const;  
};
  
struct mf_key_hasher
{
  uint32_t operator()(const mf_key_t& key) const;
};


#define LOG_MASK(x) {\
    LOG_DEBUG("{");   \
    for (int i = 0; i < globalNumFields; ++ i) {		\
      LOG_DEBUG(" %08x(%u)", x[i], __builtin_popcount(x[i]));	\
    }								\
    LOG_DEBUG("}\n");						\
  }

#define LOG_KEY(x) {\
    LOG_DEBUG("{");   \
    for (int i = 0; i < globalNumFields; ++ i) {		\
      LOG_DEBUG(" %08x", x[i]);	\
    }								\
    LOG_DEBUG("} ==> ");						\
  }
} // namespace pp

#endif // KEY_VALUE_HPP
