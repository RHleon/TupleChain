#include <ll-processing-unit.hpp>
#include <ll-tc.hpp>

namespace tc {

using namespace pp;

LLChain::LLChain(int nFields, int threshold)
    : m_iml(new ChainedTupleSpaceSearch(nFields, threshold))
    , m_threshold(threshold)
{
}

LLChain::~LLChain()
{
    for (int i = 0; i < m_nChains; ++ i) {
	destructTuple(m_heads[i]);
	m_heads[i] = NULL;
    }
    MEM_FREE(m_iml);
}

memory_metric_t
LLChain::construct(int nRules, pc_rule_t* rules)
{
    memory_metric_t res;
    res.nBytes = m_iml->constructWithRules(nRules, (rule_t*)rules);

    auto chains = m_iml->getChains();
    m_nChains = 0;

    for (auto c : chains) {
	m_heads[m_nChains ++] = createTuple(c->getHead());
    }
    
    return res;
}

LLChain::LLChain(pp::ChainedTupleSpaceSearch* tc)
    : m_iml(tc)
{
    if (tc == NULL) return;
    
    auto chains = tc->getChains();
    m_nChains = 0;

    for (auto c : chains) {
	m_heads[m_nChains ++] = createTuple(c->getHead());
    }    
}

tuple_t*
LLChain::createTuple(pp::MarkedTuple* mt)
{
    if (mt == NULL) return NULL;

    tuple_t* tuple = MEM_ALLOC(1, struct tuple_t);
    tuple->ht   = &mt->m_table;
    tuple->mask = mt->m_mask;
    tuple->succ = createTuple(mt->succ);
    tuple->fail = createTuple(mt->fail);

    return tuple;
}

void
LLChain::destructTuple(tuple_t* tuple)
{
    if (tuple == NULL) return;
    destructTuple(tuple->succ);
    destructTuple(tuple->fail);
    tuple->ht = NULL;
    tuple->mask = NULL;
    MEM_FREE(tuple);
}

#define MAX_FIELD 128
uint32_t keys[MAX_FIELD];
int nFields;

LLChain* tc_iml = NULL;
tuple_t** head_arr = NULL;
int nChains;
    
algorithm_args_t
parse(LLConfigFile config)
{
    algorithm_args_t args;

    config.addHandler("threshold", [&args] (std::string params) {
    	    args.threshold = atoi(params.c_str());
    	});
    config.parse("tc-arg");
    
    return args;
}

memory_metric_t
construct(algorithm_args_t args, int nRules, pc_rule_t* rules)
{
    nFields = globalNumFields;
    tc_iml = new LLChain(nFields, args.threshold);
    auto res = tc_iml->construct(nRules, rules);

    head_arr = tc_iml->m_heads;
    nChains = tc_iml->m_nChains;

    return res;
}

void destruct()
{
    MEM_FREE(tc_iml);
}

uint32_t search(uint32_t* const& fields)
{
    int bestPriority = -1;
    action_t bestAction = 0;
    MarkedTuple::entry_t entry;
    pc_rule_t bestRule = nullptr;

    tuple_t* curr = NULL;
    for (int i = 0; i < nChains; ++ i) {
	curr = head_arr[i];
	bestRule = nullptr;
	while (curr) {
	    for (int j = 0; j < nFields; ++ j) keys[j] = fields[j] & curr->mask[j];
	    entry = static_cast<MarkedTuple::entry_t>(curr->ht->search(keys));
	    if (entry) {
		bestRule = entry->rule;
		curr = (entry->type & MarkedTuple::EntryType::MARKER) ? curr->succ : NULL; 
	    }
	    else {
		curr = curr->fail;
	    }
	}
	if (bestRule && bestRule->priority > bestPriority) {
	    bestPriority = bestRule->priority;
	    bestAction = bestRule->action;
	}
    }
    
    return bestAction;
}

void insert(const pc_rule_t& rule)
{
    
}

void remove(const pc_rule_t& rule)
{
    
}

}

namespace pp {

struct pc_unit_t ll_tc = {
    .name            = std::string("TupleChain"),
    .parseConfigFile = tc::parse,
    .construct       = tc::construct,
    .release         = tc::destruct,
    .matchPacket     = tc::search,
    .insertRule      = tc::insert,
    .deleteRule      = tc::remove
};
    
}
