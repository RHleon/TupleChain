#include <ll-processing-unit.hpp>

namespace pp { 

extern struct pc_unit_t ll_tss;
extern struct pc_unit_t ll_tc;
extern struct pc_unit_t ll_etc;
extern struct pc_unit_t ll_xtc;
extern struct pc_unit_t ll_bfi;
extern struct pc_unit_t ll_tme;

struct pc_unit_t* selectClassifier(const std::string& name)
{
    if (name == "tss") {
	return &ll_tss;
    }
    else if (name == "tc") {
	return &ll_tc;
    }
    else if (name == "etc") {
	return &ll_etc;
    }
    else if (name == "xtc") {
	return &ll_xtc;
    }
    else if (name == "bfi") {
	return &ll_bfi;
    }
    else if (name == "tme") {
	return &ll_tme;
    }
    
    return NULL;
}
    
} // namespace pp
