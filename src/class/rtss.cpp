#include <rtss.hpp>
#include <common.hpp>

namespace pp {

RTuple::RTuple(uint32_t* mask, const int & nFileds, int id,
	       ProcessingUnit::Profiler* const& profiler)
  : OrderedTuple(mask, nFileds, id)
  , m_profiler(profiler)
{
}
  
RTuple::~RTuple()
{
  for (int i = 0; i < m_lists.size(); ++ i) {
    if (m_lists[i]) {
      delete m_lists[i];
      m_lists[i] = nullptr;
    }
  }
}
bool
RTuple::check(const pc_rule_t& rule)
{
  for (int i = 0; i < m_nFields; i++) {
	if (m_mask[i] > rule->masks[i]) return false;
  }
  return true;
}
bool
RTuple::insert(const pc_rule_t& rule)
{
  auto mask = MEM_ALLOC(m_nFields, uint32_t);
  for (int i = 0; i < m_nFields; i++) mask[i] = m_mask[i] & rule->fields[i];
  auto entry = static_cast<list_t>(m_table.search(mask));

  if (entry) {
    entry->addRule(rule);
  }
  else {
    entry = new XList(rule);
    m_lists.push_back(entry);
    m_table.insert(mask, static_cast<value_t>(entry));
  }

  m_orderedPriorities.insert(rule->priority);
  return true;
}

bool
RTuple::erase(const pc_rule_t& rule)
{
  auto mask = MEM_ALLOC(m_nFields, uint32_t);
  for (int i = 0; i < m_nFields; i++) mask[i] = m_mask[i] & rule->fields[i];
  auto entry = static_cast<list_t>(m_table.search(mask));
  MEM_FREE(mask);

  if (entry && entry->deleteRule(rule)) {
    m_orderedPriorities.erase(rule->priority);
    return true;
  }
  return false;
}


value_t
RTuple::search(const key_t& matchFields) const
{
  INIT_PROFILER_C(profiler, RangeTupleSpaceSearch);
  auto hitlist = static_cast<list_t>(m_table.search(matchFields, m_mask));
  if (!hitlist) return nullptr;
  
  for (XList *node = hitlist->next; node; node = node->next) {
  	pc_rule_t rule = node->p_rule;
  	int fid = rule->nFields;
	USE_PROFILER(profiler, {profiler->accessedListNodes ++;});

    while (fid-- > 0 && rule->fields[fid] == (matchFields[fid] & rule->masks[fid]));

    if (fid == -1) {
      return static_cast<value_t>(rule);
    }
  }
  return nullptr;
}

void
RTuple::display()
{
  for (int i = 0; i < m_nFields; ++ i) LOG_DEBUG("%08x ", m_mask[i]);
  LOG_DEBUG("\n");
  m_table.display([] (const value_t& value) {
      //LOG_MARKED_LIST(static_cast<list_t>(value));
      auto hitlist = static_cast<list_t>(value);
      for (XList *node = hitlist->next; node; node = node->next) LOG_PC_RULE(node->p_rule);
    });
}

RTuple::List::List(const pc_rule_t& r){
	list_rule.push_back(r);
}
  
RTuple::List::~List(){}


void
RTuple::List::addRule(const pc_rule_t& r)
{
  	if (list_rule.empty()) {list_rule.push_back(r); return;}
	if (r->priority > list_rule.front()->priority) {
		list_rule.push_front(r); return;
	}
	
	for (int i = 1; i < list_rule.size(); i++) {
		if (r->priority > list_rule[i]->priority) {
			list_rule.insert(list_rule.begin() + i, r);
			return;
		}
	}
	list_rule.push_back(r);
}

bool 
RTuple::List::deleteRule(const pc_rule_t& r) {
	for (int i = 0; i < list_rule.size(); i++) {
		if (r->priority == list_rule[i]->priority && r->action == list_rule[i]->action) {
			list_rule.erase(list_rule.begin() + i); return true; 
		}
		if (r->priority > list_rule[i]->priority) break;
	}
	return false;
}

RTuple::XList::XList(const pc_rule_t& r){
	XList *node = MEM_ALLOC(1, XList);
	this->next = node;
	this->p_rule = nullptr;
	node->p_rule = r;
	node->next = nullptr;
}
  
RTuple::XList::~XList(){}


void
RTuple::XList::addRule(const pc_rule_t& r)
{
  	XList *node = MEM_ALLOC(1, XList);
	node->p_rule = r;
	XList *tmp = this->next;
	XList *now = this;
	while (tmp) {
		if (node->p_rule->priority > tmp->p_rule->priority) break;
		now = tmp;
		tmp = tmp->next;
	}
	
	now->next = node; 
	node->next = tmp;
}

bool 
RTuple::XList::deleteRule(const pc_rule_t& r) {
	XList *now = this;
	XList *tmp = this->next;
	while(tmp) {
		if (tmp->p_rule->priority == r->priority && tmp->p_rule->action == r->action) {
			now->next = tmp->next;
			MEM_FREE(tmp);
			return true;
			
		}
		if (tmp->p_rule->priority < r->priority) {
			return false;
		}
		now = tmp;
		tmp = tmp->next;
	}

	return false;
}

uint64_t
RTuple::totalMemory(int bytesPerPointer) const
{
  auto res = Tuple::totalMemory(bytesPerPointer); // base tuple

  auto sumList = [bytesPerPointer] (const list_t& list) -> uint64_t {
    list_t head = list;
    uint64_t res = 0;
    while (head) {
      res += 2 * bytesPerPointer; // p_rule + next
      head = head->next;
    }
    return res;
  };
  
  for (const auto& list : m_lists) {
    res += 4; // the pointer of the head
    res += sumList(list); // the whole list
  }
  
  return res;
}
  

const int tot_num = 4;
uint32_t tmp[tot_num] = {0x0, 0xF8000000, 0xFFF80000, 0xFFFFFE00};

RangeTupleSpaceSearch::RangeTupleSpaceSearch(int nFields, int maxLength)
	: TupleSpaceSearch(nFields, "RangeTupleSearch")
{
  
  for (int i = 0 ; i < m_nFields; i++) {
	std::vector<uint32_t> dx;
	for (int j = 0; j < tot_num; j++) dx.push_back(tmp[j]);
	dim.push_back(dx);
  }
  uint32_t *mask = (uint32_t*)malloc(sizeof(uint32_t)*nFields);
  int tot = 1;
  for (int i = 0; i < m_nFields; i++) {
	tot *= tot_num;
  }
  for (int i = 0; i < tot; i++) {
  	int id = i;
	for (int j = 0; j < m_nFields; j++) {
		mask[j]= dim[j][id%tot_num];
		id/=tot_num;
	}
	m_tuples.push_back(new RTuple(mask, m_nFields, 0, m_profiler));
  }
  LOG_DEBUG("m_tuple.size = %lu\n", m_tuples.size());
}

RangeTupleSpaceSearch::~RangeTupleSpaceSearch()
{

}

int
RangeTupleSpaceSearch::insertRuleConstruction(const pc_rule_t& rule)
{
  int id = 0;
  for (int j = dim.size() - 1; j >= 0; j--) {
  		for (int i = dim[j].size() - 1; i >= 0; i--) {
			if (rule->masks[j] >= dim[j][i]) {
				id = id * dim[j].size() + i; 
				break;
			}
		}
  }
  bool flag = m_tuples[id]->insert(rule);
  if (flag) {
    return m_tuples[id]->getId();
  }
  else {
    return RuleInsertionResult::FAIL;
  }
}

uint64_t
RangeTupleSpaceSearch::finalizeConstruction()
{
  INIT_PROFILER(profiler);
  USE_PROFILER_C(profiler, { profiler->nTuples = m_tuples.size(); });
  
  uint64_t res = 4; // m_nFields
  for (auto& tuple : m_tuples) res += tuple->totalMemory(m_profiler->bytesPerPointer);
  for (auto vec : dim) for (auto& e : vec) res += 4; // dims
  return res;
}

action_t
RangeTupleSpaceSearch::matchPacket(const packet_t& packet)
{
  INIT_PROFILER(profiler);
  auto bestRule = m_defaultRule;
  uint32_t* matchFields = (uint32_t*)packet;
  
  for (auto& tuple : m_tuples) {
    const auto& rt = static_cast<RTuple*>(tuple);
    if (rt->maxPriority() <= bestRule->priority) continue;
    auto hitRule = static_cast<pc_rule_t>(rt->search(matchFields));
    USE_PROFILER(profiler, {profiler->accessedTuples ++; });
    UPDATE_BEST_RULE(bestRule, hitRule);
  }

  return bestRule->action;

}

void
RangeTupleSpaceSearch::insertRule(const pc_rule_t& rule)
{
  int id = 0;
  for (int j = dim.size() - 1; j >= 0; j--) {
  		for (int i = dim[j].size() - 1; i >= 0; i--) {
			if (rule->masks[j] >= dim[j][i]) {
				id = id * dim[j].size() + i; 
				break;
			}
		}
  }
  m_tuples[id]->insert(rule);
}
void
RangeTupleSpaceSearch::deleteRule(const pc_rule_t& rule)
{
  int id = 0;
  for (int j = dim.size() - 1; j >= 0; j--) {
  		for (int i = dim[j].size() - 1; i >= 0; i--) {
			if (rule->masks[j] >= dim[j][i]) {
				id = id * dim[j].size() + i; 
				break;
			}
		}
  }
  m_tuples[id]->erase(rule);
}

void
RangeTupleSpaceSearch::Profiler::dump(FILE* file)
{
  TupleSpaceSearch::Profiler::dump(file);
  LOG_FILE(file, "| tl = %.2f ", accessedListNodes * 1.0f / nPackets);
}

void
RangeTupleSpaceSearch::display()
{
  LOG_DEBUG("# of tuples: %lu\n", m_tuples.size());
  for (int i = 0; i < m_tuples.size(); ++i) {
    LOG_DEBUG("###### field_%d #######\n", i);
    m_tuples[i]->display();
  }
}

} // namespace pp


void
pp::RangeTupleSpaceSearch::TEST()
{
  rule_t* rules;
  packet_t* packets;
  action_t* results;
  int nRules   = loadRuleFromFile("test-data/2_8.rule", rules);
  int nPackets = loadTrafficFromFile("test-data/2_8_match.traffic", packets);
  int nResults = loadResultFromFile("test-data/2_8_match.traffic.result", results);
  LOG_ASSERT(nPackets == nResults, "invalid # of results");

  globalNumFields = 2;
  RangeTupleSpaceSearch tss(globalNumFields, 5);
  tss.constructWithRules(nRules, rules);
  //tss.display();

  LOG_DEBUG("match %d packets\n", nPackets);
  for (int i = 0; i < nPackets; ++i) {
    LOG_ASSERT(tss.matchPacket(packets[i]) == results[i], "ERROR at %d", i);
  }
}
