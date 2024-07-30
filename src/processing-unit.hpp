#ifndef PROCESSING_UNIT_HPP
#define PROCESSING_UNIT_HPP

#include <string>
#include <vector>
#include <packet-io.hpp>
#include <profiler-flag.hpp>

namespace pp {

typedef void* rule_t;

struct update_time_t
{
  uint64_t s  : 32;
  uint64_t ms : 10;
  uint64_t us : 10;
  uint64_t od : 12;
};
  
struct update_t
{
  update_time_t time;
  rule_t rule;
};

enum RuleInsertionResult {
  OK = 0,
  FAIL = -1
};

class ProcessingUnit
{
public:
  static int
  generateMatchResult(const char* rule_path, const char* traffic_path, ProcessingUnit* pu);
  
public:
  ProcessingUnit(const std::string& name)
    : m_name(name)
    , m_profiler(nullptr)
  {}
  
  virtual ~ProcessingUnit() = default;
  
public:
  virtual uint64_t
  constructWithRules(int nRules, rule_t* rules) = 0;

  virtual action_t
  matchPacket(const packet_t& packet) = 0;

  virtual void
  processUpdate(const update_t& update) = 0;

public:
  struct Profiler
  {
    bool constEnabled;
    bool matchEnabled;
    bool updateEnabled;
    int nPackets;
    int bytesPerPointer;
    Profiler(int n)
      : nPackets(n)
      , constEnabled(false)
      , matchEnabled(false)
      , updateEnabled(false)
      , bytesPerPointer(4) {};
    virtual ~Profiler() = default;
    virtual void dump(FILE* file = NULL) = 0;
  };
  
  virtual Profiler*
  initializeMatchProfiler(int nPackets) { return m_profiler; }
  
public:
  std::string
  getName() const { return m_name; }

  void
  setName(const std::string& name) { m_name = name; }

protected:
  std::string m_name;
  Profiler* m_profiler;
};

#define DEFINE_PROFILER					\
  virtual ProcessingUnit::Profiler*			\
  initializeMatchProfiler(int nPackets) override	\
  {							\
    return (m_profiler = new Profiler(nPackets));	\
  }

#ifdef PROFILER_ENABLED
#define INIT_PROFILER(profiler)					\
  const auto& profiler = static_cast<Profiler*>(m_profiler);
#define INIT_PROFILER_C(profiler, class)					\
  const auto& profiler = static_cast<class::Profiler*>(m_profiler);
#else
#define INIT_PROFILER(profiler)
#define INIT_PROFILER_C(profiler, class)
#endif

#ifdef PROFILER_ENABLED
#define USE_PROFILER_C(profiler, process)	\
  if (profiler && profiler->constEnabled) {	\
    process					\
  }
#else 
#define USE_PROFILER_C(profiler, process)
#endif

#ifdef PROFILER_ENABLED
#define USE_PROFILER(profiler, process)	\
  if (profiler && profiler->matchEnabled) {	\
    process					\
  }
#else 
#define USE_PROFILER(profiler, process)
#endif

#ifdef PROFILER_ENABLED
#define USE_PROFILER_U(profiler, process)	\
  if (profiler && profiler->updateEnabled) {	\
    process					\
  }
#else 
#define USE_PROFILER_U(profiler, process)
#endif
} // namespace pp

#endif // PROCESSING_UNIT_HPP
