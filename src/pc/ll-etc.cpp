#include <ll-etc.hpp>
#include <ll-processing-unit.hpp>
#include <algorithm>

namespace etc {

using namespace pp;

#define MAX_FIELD 128
uint32_t keys[MAX_FIELD];
int nFields;
    
#define GEN_KEY(fileds, mask) {						\
	for (int i = 0; i < nFields; ++ i) keys[i] = fields[i] & mask[i]; \
    }
#define BEST_RULE(rule) {					\
	if (rule && rule->priority > bestRule->priority) {	\
	    bestRule = rule;					\
	}							\
    }
    
LLLeaf::LLLeaf(ChainedTupleSpaceSearch* tc)
    : tc_iml(tc)
    , m_searchChain(true)
{
    if (tc == NULL) return;
    
    auto chains = tc->getChains();
    m_nChains = 0;

    for (auto c : chains) {
	m_heads[m_nChains ++] = createTuple(c->getHead());
    }    
}

LLLeaf::LLLeaf(const std::vector<pc_rule_t>& rules)
    : tc_iml(NULL)
    , m_rules(rules)
    , m_searchChain(false)
{
    std::sort(m_rules.begin(), m_rules.end(), [] (const pc_rule_t& x, const pc_rule_t& y) {
	    return x->priority > y->priority;
	});
}

void
LLLeaf::search(uint32_t* const& fields, pc_rule_t& bestRule)
{
    if (!m_searchChain) {
	bool ok;
	for (auto& rule : m_rules) {
	    if (rule->priority <= bestRule->priority) break;
	    
	    ok = true;
	    for (int i = 0; i < nFields; ++ i) {
		if ((fields[i] & rule->masks[i]) != rule->fields[i]) {
		    ok =false;
		    break;
		}
	    }
	    if (ok) {
		bestRule = rule;
		break;
	    }
	}
    }
    else {
	tc::tuple_t* curr = NULL;
	MarkedTuple::entry_t et;
	pc_rule_t rule;
	for (int i = 0; i < m_nChains; ++ i) {
	    curr = m_heads[i];
	    rule = nullptr;
	    while (curr) {
		GEN_KEY(fields, curr->mask);
		et = static_cast<MarkedTuple::entry_t>(curr->ht->search(keys));
		if (et) {
		    rule = et->rule;
		    curr = (et->type & MarkedTuple::EntryType::MARKER) ? curr->succ : NULL; 
		}
		else {
		    curr = curr->fail;
		}
	    }
	    if (rule && rule->priority > bestRule->priority) {
		bestRule = rule;
	    }	    
	}
    }
}

void
LLLeaf::search5(uint32_t* const& fields, pc_rule_t& bestRule)
{
    if (!m_searchChain) {
	bool ok;
	for (auto& rule : m_rules) {
	    if (rule->priority <= bestRule->priority) break;

	    if ((fields[0] & rule->masks[0]) == rule->fields[0]
		&& (fields[1] & rule->masks[1]) == rule->fields[1]
		&& (fields[2] & rule->masks[2]) == rule->fields[2]
		&& (fields[3] & rule->masks[3]) == rule->fields[3]
		&& (fields[4] & rule->masks[4]) == rule->fields[4]) {
		bestRule = rule;
		break;
	    }
	}
    }
    else {
	tc::tuple_t* curr = NULL;
	MarkedTuple::entry_t et;
	pc_rule_t rule;

	for (int i = 0; i < m_nChains; ++ i) {
	    curr = m_heads[i];
	    rule = nullptr;
	    while (curr) {
		keys[0] = fields[0] & curr->mask[0];
		keys[1] = fields[1] & curr->mask[1];
		keys[2] = fields[2] & curr->mask[2];
		keys[3] = fields[3] & curr->mask[3];
		keys[4] = fields[4] & curr->mask[4];
		
		et = static_cast<MarkedTuple::entry_t>(curr->ht->search(keys));
		if (et) {
		    rule = et->rule;
		    curr = (et->type & MarkedTuple::EntryType::MARKER) ? curr->succ : NULL; 
		}
		else {
		    curr = curr->fail;
		}
	    }
	    if (rule && rule->priority > bestRule->priority) {
		bestRule = rule;
	    }	    
	}
    }
}

tc::tuple_t*
LLLeaf::createTuple(pp::MarkedTuple* mt)
{
    if (mt == NULL) return NULL;

    tc::tuple_t* tuple = MEM_ALLOC(1, struct tc::tuple_t);
    tuple->ht   = &mt->m_table;
    tuple->mask = mt->m_mask;
    tuple->succ = createTuple(mt->succ);
    tuple->fail = createTuple(mt->fail);

    return tuple;
}

void
LLLeaf::destructTuple(tc::tuple_t* tuple)
{
    if (tuple == NULL) return;
    destructTuple(tuple->succ);
    destructTuple(tuple->fail);
    tuple->ht = NULL;
    tuple->mask = NULL;
    MEM_FREE(tuple);
}
    
ExChainedTupleSpaceSearch* etc_iml = NULL;
TupleNode* tree_root = NULL;
pc_rule_t defaultRule = NULL;

void
LLLeaf::visitNode(TupleNode* root)
{
    if (root == NULL) return;
    auto& entries = root->m_entries;
    for (auto& entry : entries) {
	if (entry->type == TupleNode::EntryType::LEAF) {
	    auto tc_iml = static_cast<ChainedTupleSpaceSearch*>(entry->u.leaf);
	    LLLeaf* leaf;
	    if (tc_iml->m_searchChain) {
		leaf = new LLLeaf(tc_iml);
	    }
	    else {
		leaf = new LLLeaf(tc_iml->m_rules);
	    }
	    entry->u.leaf = (value_t)leaf; 
	}
	else if (entry->type == TupleNode::EntryType::NODE) {
	    if (entry->u.node->m_table == nullptr) {
		LLLeaf* leaf = new LLLeaf(entry->u.node->m_rules);
		entry->u.leaf = (value_t)leaf;
		entry->type = TupleNode::EntryType::LEAF;
	    }
	    else {
		visitNode(entry->u.node);
	    }
	}
    }
}
    
algorithm_args_t
parse(LLConfigFile config)
{
    algorithm_args_t args;

    config.addHandler("height", [&args] (std::string params) {
    	    args.height = atoi(params.c_str());
    	});	
    config.addHandler("threshold", [&args] (std::string params) {
    	    args.threshold = atoi(params.c_str());
    	});
    config.parse("etc-arg");
    
    return args;
}

memory_metric_t
construct(algorithm_args_t args, int nRules, pc_rule_t* rules)
{
    memory_metric_t res;
    nFields = globalNumFields;
    etc_iml = new ExChainedTupleSpaceSearch(nFields, args.height, args.threshold);
    res.nBytes = etc_iml->constructWithRules(nRules, (rule_t*)rules);

    defaultRule = new pc_rule();
    defaultRule->priority = -1;
    defaultRule->action = 0;
    
    tree_root = etc_iml->m_treeRoot;
    LLLeaf::visitNode(tree_root);
    
    return res;
}

void destruct()
{
    MEM_FREE(etc_iml);
}
    
uint32_t search(uint32_t* const& fields)
{
    pc_rule_t bestRule = defaultRule;
    pc_rule_t rule = NULL;
    auto root = tree_root;

    GEN_KEY(fields, root->m_mask);
    auto entry = static_cast<TupleNode::entry_t>(root->m_table->search(keys));
    while (entry) {
	rule = entry->rule; BEST_RULE(rule);
	if (entry->type != TupleNode::EntryType::NODE) break;
	root = entry->u.node;
	GEN_KEY(fields, root->m_mask);
	entry = static_cast<TupleNode::entry_t>(root->m_table->search(keys));
    }

    if (entry && entry->type == TupleNode::EntryType::LEAF) {
	static_cast<LLLeaf*>(entry->u.leaf)->search(fields, bestRule);
    }
  
    return bestRule->action;
}

void insert(const pc_rule_t& rule)
{
    
}

void remove(const pc_rule_t& rule)
{
    
}

}

namespace pp {

struct pc_unit_t ll_etc = {
    .name            = std::string("Extended TupleChain"),
    .parseConfigFile = etc::parse,
    .construct       = etc::construct,
    .release         = etc::destruct,
    .matchPacket     = etc::search,
    .insertRule      = etc::insert,
    .deleteRule      = etc::remove
};
    
}
