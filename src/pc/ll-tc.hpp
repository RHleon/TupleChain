#ifndef LL_TC_H
#define LL_TC_H

#include <c-tss.hpp>
#include <ll-processing-unit.hpp>

namespace tc { 

#define MAX_CHAIN 1024
    
struct tuple_t
{
    pp::HashTable* ht;
    uint32_t*  mask;
    struct tuple_t *succ, *fail;
};

class LLChain
{
public:
    LLChain(int nFields, int threshold);
    LLChain(pp::ChainedTupleSpaceSearch* tc);
    ~LLChain();

    pp::memory_metric_t
    construct(int nRules, pp::pc_rule_t* rules);
    
    
public:
    tuple_t* m_heads[MAX_CHAIN];
    int m_nChains;
    int m_threshold;

public:
    tuple_t*
    createTuple(pp::MarkedTuple* mt);

    void
    destructTuple(tuple_t* tuple);
    
    pp::ChainedTupleSpaceSearch* m_iml;
};
    
} // namespace tc

#endif
