#ifndef LL_ETC_H
#define LL_ETC_H

#include <ec-tss.hpp>
#include <ll-tc.hpp>
#include <vector>

namespace etc { 
    
class LLLeaf
{
public:
    static void visitNode(pp::TupleNode* root);
    LLLeaf(pp::ChainedTupleSpaceSearch* tc);
    LLLeaf(const std::vector<pp::pc_rule_t>& rules);

    void search(uint32_t* const& fields, pp::pc_rule_t& bestRule);
    void search5(uint32_t* const& fields, pp::pc_rule_t& bestRule);
    
    pp::ChainedTupleSpaceSearch* tc_iml;
    int m_nChains;
    tc::tuple_t* m_heads[128];
    std::vector<pp::pc_rule_t> m_rules;
    bool m_searchChain;

protected:
    tc::tuple_t* createTuple(pp::MarkedTuple* mt);
    void destructTuple(tc::tuple_t* tuple);
};
    
} // namespace pp

#endif
