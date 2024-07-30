#include <ll-pkt-io.hpp>
#include <rule-traffic.hpp>
#include <pp-thread.hpp>

namespace dummyIo {

using namespace pp;
io_params_t g_ioParams;
io_statistic_t g_ioStat;

packet_t* g_pktBuffer;
int       g_nPackets;
int       g_nQueueMask;

pp_thread_t g_tMainLoop;
int g_rxHead, g_rxTail;
int g_txHead, g_txTail;

void mainLoop(pp_thread_t thisThread)
{
    int nPackets, nMaxPackets = g_nPackets;
    int nSentPackets = 0, nReceivedPackets = 0;

    double txRatePerNs = g_ioParams.tx_rate / 1000.0f; // from MPPS to GPPS
    uint64_t stopCount = g_ioParams.test_seconds; stopCount *= 1000000000; // ns
    uint64_t tickCount = 0;

    TimeMeasurer measurer;
    while (thisThread->running && (!globalStopRequested)) {
	tickCount = measurer.tick();
	if (tickCount > stopCount) break;
    
	// emulate the behaviour of flushing packets
	nPackets = tickCount * txRatePerNs;
	if (nPackets > nSentPackets) {
	    nSentPackets = nPackets;
	    g_rxTail = nSentPackets & g_nQueueMask;
	}

	// emulate the behavior of receiving packets
	nPackets = g_txTail - g_txHead;
	if (nPackets < 0) nPackets += nMaxPackets;
	if (nPackets > 0) {
	    g_txHead = g_txTail;
	    nReceivedPackets += nPackets;
	}
    }
    auto tot = measurer.stop(TimeUnit::us);
    globalStopRequested = true;

    // LOG_DEBUG("\ntester sent out %4.1f K packets (tx = %5.2f)\n",
    // 	    "tester recevied %4.1f K packets (rx = %5.2f)\n",
    // 	    nSentPackets / 1000.0f, nSentPackets * 1.0f / tot,
    // 	    nReceivedPackets / 1000.0f, nReceivedPackets * 1.0f / tot);
  
    g_ioStat.nSentPackets = nSentPackets;
    g_ioStat.nReceivedPackets = nReceivedPackets;
    g_ioStat.txRate = nSentPackets * 1.0f / tot;
    g_ioStat.rxRate = nReceivedPackets * 1.0f / tot;
    g_ioStat.testTime = tot / 1000000;
}

io_params_t parse(LLConfigFile config)
{
    io_params_t params;
    return params;
}

void start(const io_params_t& params)
{
    g_ioParams = params;
    
    g_nPackets = loadTrafficFromFile(params.pkt_file.c_str(), g_pktBuffer, params.data_type);
    for (uint32_t m = 1; m <= g_nPackets; m <<= 1) g_nQueueMask = m - 1;
    g_nPackets = g_nQueueMask + 1;

    g_rxHead = g_rxTail = g_txHead = g_txTail = 0;
    g_tMainLoop = MultiThreading::launchThread(std::bind(mainLoop, _1));
}

io_statistic_t stop()
{
    g_tMainLoop = MultiThreading::terminateThread(g_tMainLoop);

    MEM_FREE(g_pktBuffer);
    return g_ioStat;
}

packet_t* pull(int& num, int max)
{
    int tail = g_rxTail;
    auto pkts = g_pktBuffer + g_rxHead;

    int n = tail - g_rxHead;
    if (n < 0) n += g_nPackets;

    if (n > max) n = max;
    g_rxHead = (g_rxHead + n) & g_nQueueMask;
    num = n;

    return pkts;
}

int push(packet_t* pkts, action_t* actions, int max)
{
    g_txTail = (g_txTail + max) & g_nQueueMask;
    return max;
}

}

namespace pp {

struct pkt_io_t ll_dummy_io = {
    .parseConfigFile = dummyIo::parse,
    .start           = dummyIo::start,
    .stop            = dummyIo::stop,
    .pullPackets     = dummyIo::pull,
    .pushPackets     = dummyIo::push
};

}
