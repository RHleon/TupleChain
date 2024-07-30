#ifndef LL_PKT_IO_H
#define LL_PKT_IO_H

#include <packet-io.hpp>
#include <string>
#include <ll-config-file.hpp>

namespace pp { 

struct io_params_t {
    std::string pkt_file;
    int data_type;

    double tx_rate;
    int test_seconds;
};

struct io_statistic_t {
    double txRate, rxRate;
    uint64_t nSentPackets, nReceivedPackets;
    int testTime;
};
 
struct pkt_io_t {
    io_params_t    (*parseConfigFile)(LLConfigFile config);
    void           (*start)(const io_params_t& params);
    io_statistic_t (*stop)();
    packet_t*      (*pullPackets)(int& num, int max);
    int            (*pushPackets)(packet_t* pkts, action_t* actions, int max);
};

struct pkt_io_t* selectIo(const std::string& name);
    
} // namespace pp

#endif
