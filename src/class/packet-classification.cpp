#include <packet-classification.hpp>
#include <common.hpp>

namespace pp {

PacketClassification::PacketClassification(const std::string& name)
  : ProcessingUnit(name)
  , m_defaultRule(MEM_ALLOC(1, pc_rule))
{
  m_defaultRule->priority = -1;
  m_defaultRule->action = DEFAULT_ACTION;
}

PacketClassification::~PacketClassification()
{
  if (m_profiler) {
    MEM_FREE(m_profiler);
    m_profiler = nullptr;
  }
    
  if (m_defaultRule) {
    MEM_FREE(m_defaultRule);
    m_defaultRule = nullptr;
  }
}
  
uint64_t
PacketClassification::constructWithRules(int nRules, rule_t* rules)
{
  initializeConstruction();

  for (int i = 0; i < nRules; ++i) {
    auto rule = static_cast<pc_rule_t>(rules[i]);
    if (insertRuleConstruction(rule) == RuleInsertionResult::FAIL) {
      LOG_PC_RULE(rule);
      LOG_ERROR("insert pc rule\n");
    }
  }
  
  return finalizeConstruction();
}

uint64_t
PacketClassification::constructWithRuleVec(const std::vector<pc_rule_t>& rules)
{
  initializeConstruction();

  for (auto rule : rules) {
    if (insertRuleConstruction(rule) == RuleInsertionResult::FAIL) {
      LOG_PC_RULE(rule);
      LOG_ERROR("insert pc rule\n");
    }
  }
  
  return finalizeConstruction();    
}

action_t
PacketClassification::matchPacket(const packet_t& packet)
{
  auto bestRule = m_defaultRule;
  uint32_t* matchFields = (uint32_t*)packet;
  search(matchFields, bestRule);
  return bestRule->action;
}

void
PacketClassification::search(const field_t& matchFields, pc_rule_t& bestRule)
{
  for (auto& rule : m_rules) {
    int fid = rule->nFields;
    while (fid-- > 0 && rule->fields[fid] == (matchFields[fid] & rule->masks[fid]));
    if (fid == -1) UPDATE_BEST_RULE(bestRule, rule);
  }  
}

void
PacketClassification::insertRule(const pc_rule_t& rule)
{
  m_rules.push_back(rule);
}

void
PacketClassification::deleteRule(const pc_rule_t& rule)
{
  m_rules.erase(std::remove_if(m_rules.begin(), m_rules.end(), [&rule] (const pc_rule_t& entry) {
	return (*entry) == (*rule);
      }), m_rules.end());
}

} // namespace pp
