#ifndef PACKET_CLASSIFICATION_HPP
#define PACKET_CLASSIFICATION_HPP

#include <processing-unit.hpp>
#include <cinttypes>
#include <vector>
#include <iostream>
#include <iomanip>

namespace pp {

static uint32_t MASK_LEN_32[33] = {
  0x0,
  0x80000000, 0xc0000000, 0xe0000000, 0xf0000000,
  0xf8000000, 0xfc000000, 0xfe000000, 0xff000000,
  0xff800000, 0xffc00000, 0xffe00000, 0xfff00000,
  0xfff80000, 0xfffc0000, 0xfffe0000, 0xffff0000,
  0xffff8000, 0xffffc000, 0xffffe000, 0xfffff000,
  0xfffff800, 0xfffffc00, 0xfffffe00, 0xffffff00,
  0xffffff80, 0xffffffc0, 0xffffffe0, 0xfffffff0,
  0xfffffff8, 0xfffffffc, 0xfffffffe, 0xffffffff
};
  
struct pc_rule {
  uint32_t* fields;
  uint32_t* masks;
  uint8_t nFields;
  int priority;
  action_t action;
};
typedef struct pc_rule* pc_rule_t;

#define LOG_PC_RULE(rule) {						\
    pc_rule_t pRule = rule;						\
    LOG_INFO("[%3d %3u ", pRule->priority, pRule->action);		\
    for (int ii = 0; ii < pRule->nFields; ++ ii) {			\
      LOG_INFO("%08x/%08x ", pRule->fields[ii], pRule->masks[ii]);	\
    }									\
    LOG_INFO("]\n");							\
  }

#define UPDATE_BEST_RULE(x, y) { if (y && y->priority > x->priority) x = y; }
  
inline void
MATCH_RULES(const int& nFields, uint32_t* const &fields,
	    const std::vector<pc_rule_t>& rules, pc_rule_t& bestRule)
{
  int i;
  for (auto& rule : rules) {
    for (i = 0; i < nFields; ++ i) {
      if ((fields[i] & rule->masks[i]) != rule->fields[i]) break;
    }
    if (i == nFields) UPDATE_BEST_RULE(bestRule, rule);
  }
}
  
class PacketClassification : public ProcessingUnit
{ 
public:
  PacketClassification(const std::string& name);
  ~PacketClassification();

public:
  virtual uint64_t
  constructWithRules(int nRules, rule_t* rules) final;

  uint64_t
  constructWithRuleVec(const std::vector<pc_rule_t>& rules);

  virtual action_t
  matchPacket(const packet_t& packet) override;

  virtual void
  processUpdate(const update_t& update) override
  {
    auto rule = static_cast<pc_rule_t>(update.rule);
    rule->action ? insertRule(rule) : deleteRule(rule);
  }

public:
  virtual void
  initializeConstruction() {};

  virtual uint64_t
  finalizeConstruction() { return 0; };
  
  virtual int
  insertRuleConstruction(const pc_rule_t& rule)
  {
    m_rules.push_back(rule);
    return RuleInsertionResult::OK;
  }

  virtual void
  search(uint32_t* const &matchFields, pc_rule_t& bestRule);

  virtual void
  insertRule(const pc_rule_t& rule);

  virtual void
  deleteRule(const pc_rule_t& rule);

public:// profiler
  struct Profiler : public ProcessingUnit::Profiler
  {
    Profiler(int n) : ProcessingUnit::Profiler(n) {};
    virtual void dump(FILE* file = NULL) override {};
  };
  DEFINE_PROFILER
  
public:
  std::vector<pc_rule_t> m_rules;
  pc_rule_t m_defaultRule;
};

inline bool
operator == (const pc_rule& x, const pc_rule& y)
{
  if (x.priority != y.priority || x.nFields != y.nFields) {
    return false;
  }

  int fid = x.nFields;
  while (fid -- > 0 && x.fields[fid] == y.fields[fid] && x.masks[fid] == y.masks[fid]);

  return fid == -1;
}

} // namespace pp

#endif // PACKET_CLASSIFICATION_HPP
