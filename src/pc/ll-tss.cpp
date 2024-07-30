#include <ll-processing-unit.hpp>
#include <tss.hpp>

namespace tss {

using namespace pp;
TupleSpaceSearch* tss_iml = NULL;

#define MAX_TUPLE 1024
struct tuple_t
{
    HashTable* ht;
    uint32_t*  mask;
    int priority;
    tuple_t() : ht(NULL), mask(NULL), priority(0) {};
};
tuple_t tuples[MAX_TUPLE];
int nTuples;
int nFields;

#define MAX_FIELD 128
uint32_t keys[MAX_FIELD];
    
algorithm_args_t
parse(LLConfigFile config)
{
    algorithm_args_t args;

    // config.addHandler("num_fields", [&args] (std::string prarms) {
    // 	    args.num_fields = atoi(params.c_str());
    // 	});
    // config.parse("tss-arg");
    
    return args;
}

memory_metric_t
construct(algorithm_args_t args, int nRules, pc_rule_t* rules)
{
    memory_metric_t res;

    nFields = globalNumFields;
    tss_iml = new TupleSpaceSearch(nFields);
    res.nBytes = tss_iml->constructWithRules(nRules, (rule_t*)rules);

    nTuples = tss_iml->m_tuples.size();
    assert(nTuples < MAX_TUPLE);

    for (int i = 0; i < nTuples; ++ i) {
	tuples[i].ht = &tss_iml->m_tuples[i]->m_table;
	tuples[i].mask = tss_iml->m_tuples[i]->m_mask;
	tuples[i].priority = static_cast<OrderedTuple*>(tss_iml->m_tuples[i])->maxPriority();
    }
    
    return res;
}

void destruct()
{
    MEM_FREE(tss_iml);
}

uint32_t search(uint32_t* const& fields)
{
    int bestPriority = 0;
    action_t bestAction = 0;

    tuple_t* tuple = tuples;
    pc_rule_t rule;
    for (int i = nTuples; i -- ; ++ tuple) {
	if (tuple->priority <= bestPriority) continue;
	for (int j = 0; j < nFields; ++ j) keys[j] = fields[j] & tuple->mask[j];
	rule = static_cast<pc_rule_t>(tuple->ht->search(keys));
	if (rule && rule->priority > bestPriority) {
	    bestPriority = rule->priority;
	    bestAction = rule->action;
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

struct pc_unit_t ll_tss = {
    .name            = std::string("Tuple Space Search"),
    .parseConfigFile = tss::parse,
    .construct       = tss::construct,
    .release         = tss::destruct,
    .matchPacket     = tss::search,
    .insertRule      = tss::insert,
    .deleteRule      = tss::remove
};
    
}
