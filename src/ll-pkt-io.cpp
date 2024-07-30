#include <ll-pkt-io.hpp>

namespace pp { 

extern struct pkt_io_t ll_dummy_io;
    
struct pkt_io_t* selectIo(const std::string& name)
{
    if (name == "dummy") {
	return &ll_dummy_io;
    }

    return NULL;
}
    
} // namespace pp
