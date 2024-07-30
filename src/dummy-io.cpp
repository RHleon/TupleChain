#include <dummy-io.hpp>
#include <rule-traffic.hpp>
#include <common.hpp>
#include <vector>

namespace pp {

DummyIO::DummyIO(DummyIOParameters params)
  : m_configs(params)
  , m_testTxRate(0)
  , m_rxHead(0)
  , m_rxTail(0)
  , m_txHead(0)
  , m_txTail(0)
{
  // load traffics from file
  m_nTotalPackets = loadTrafficFromFile(m_configs.traffic_file.c_str(),
					m_packetBuffer,
					m_configs.data_type);

  uint32_t temp = (uint32_t)1;
  
  while((temp-1) <= m_nTotalPackets){
        m_packs_nb_mask = temp-1;
        temp <<= 1;
  }

  LOG_DEBUG("DummyIO loaded %d packets, queue mask=0x%x\n", m_nTotalPackets, m_packs_nb_mask);
}
  
DummyIO::~DummyIO()
{
  freePackets(m_nTotalPackets, m_packetBuffer);
}

void
DummyIO::start()
{
  m_rxHead = m_rxTail = 0;
  m_txHead = m_txTail = 0;

  LOG_DEBUG("system concurrency: %d\n", std::thread::hardware_concurrency());
  m_emulator = MultiThreading::launchThread(std::bind(&DummyIO::emulatorLoop, this, _1));
}

void
DummyIO::stop()
{
  m_emulator = MultiThreading::terminateThread(m_emulator);
}
  
int
DummyIO::pullPackets(packet_t* pkts, int max_num)
{
  int idx = 0;
  int tail = m_rxTail;

  if (tail < m_rxHead) {
    while (idx < max_num && m_rxHead < m_nTotalPackets) {
      pkts[idx ++] = m_packetBuffer[m_rxHead ++];
    }
    if (idx < max_num) {
      m_rxHead = 0;
    }
  }

  while(idx < max_num && m_rxHead < tail) {
    pkts[idx ++] = m_packetBuffer[m_rxHead ++];
  }
  
  return idx;
}

packet_t*
DummyIO::pullPackets(int& num, int max_num)
{
    int tail = m_rxTail;
    auto res = m_packetBuffer + m_rxHead;
    if (tail < m_rxHead) {
	num = m_nTotalPackets - m_rxHead;
    }
    else {
	num = tail - m_rxHead;
    }
    if (num > max_num) num = max_num;
    //m_rxHead = (m_rxHead + num) % m_nTotalPackets;
    m_rxHead = (m_rxHead + num) & m_packs_nb_mask;
 
    return res;
}

int
DummyIO::pushPackets(packet_t* pkts, action_t* actions, int num)
{
    //m_txTail = (m_txTail + num) % m_nTotalPackets;
    m_txTail = (m_txTail + num) & m_packs_nb_mask;
    return num;
}

void
DummyIO::emulatorLoop(pp_thread_t thisThread)
{
  int nEvents = 0, nMaxEvents = m_configs.max_io_events;
  std::vector<Event> ioEvents(nMaxEvents);
#define ADD_IO_EVENT(t, n) {			\
    if (nEvents < nMaxEvents) {			\
      ioEvents[nEvents].time = t;		\
      ioEvents[nEvents ++].nPackets = n;	\
    }						\
}

  int nPackets, nMaxPackets = m_nTotalPackets;
  int nSentPackets = 0, nReceivedPackets = 0;

  double txRatePerNs = m_configs.tx_rate / 1000.0f; // from MPPS to GPPS
  uint64_t stopCount = m_configs.test_seconds; stopCount *= 1000000000; // ns
  uint64_t tickCount = 0;
  int tail;

  TimeMeasurer measurer;
  while (thisThread->running) {
    tickCount = measurer.tick();
    if (tickCount > stopCount) break;
    
    // emulate the behaviour of flushing packets
    nPackets = tickCount * txRatePerNs;
    if (nPackets > nSentPackets) {
      nSentPackets = nPackets;
      //tail = nSentPackets; if (tail >= nMaxPackets) tail -= nMaxPackets;
      m_rxTail = nSentPackets % nMaxPackets;
      //ADD_IO_EVENT(tickCount, nPackets); // add tx event
    }

    // emulate the behavior of receiving packets
    nPackets = m_txTail - m_txHead;
    if (nPackets < 0) nPackets += nMaxPackets;
    if (nPackets > 0) {
      m_txHead = m_txTail;
      nReceivedPackets += nPackets;
      //ADD_IO_EVENT(tickCount, -1 * nPackets); // add rx event
    }
  }
  auto tot = measurer.stop(TimeUnit::us);
  globalStopRequested = true;

  // LOG_DEBUG("\ntester sent out %4.1f K packets (tx = %5.2f)\n",
  // 	    "tester recevied %4.1f K packets (rx = %5.2f)\n",
  // 	    nSentPackets / 1000.0f, nSentPackets * 1.0f / tot,
  // 	    nReceivedPackets / 1000.0f, nReceivedPackets * 1.0f / tot);
  
  m_statistic.nSentPackets = nSentPackets;
  m_statistic.nReceivedPackets = nReceivedPackets;
  m_statistic.txRate = nSentPackets * 1.0f / tot;
  m_statistic.rxRate = nReceivedPackets * 1.0f / tot;
  m_statistic.testTime = tot / 1000000;

  //analyseEvents(ioEvents, nEvents);
}

void
DummyIO::analyseEvents(const std::vector<Event>& events, int nEvents)
{
  // TODO. Analyse packet latency distribution
}
 
void
DummyIO::TEST(DummyIOParameters params)
{
  LOG_INFO("hello DummyIO\n");
  DummyIO io(params);
  io.start();

  packet_t pkts[1024];
  int nPkts = 0;
  int rcvPkts = 0;
  int sntPkts = 0;
  
  TimeMeasurer measurer;
  while (!globalStopRequested) {
    rcvPkts += (nPkts = io.pullPackets(pkts, 256));
    sntPkts += io.pushPackets(pkts, NULL, nPkts);
  }
  auto tot = measurer.stop(TimeUnit::us);

  LOG_INFO("average rx_rate: %.2f MPPS\n"
	   "average tx_rate: %.2f MPPS\n",
	   rcvPkts * 1.0f / tot, sntPkts * 1.0f / tot);

  io.stop();
  
  auto statistic = io.getStatistic();
  LOG_INFO("emulator tx_rate: %.2f MPPS\n"
	   "emulator rx_rate: %.2f MPPS",
	   statistic.txRate, statistic.rxRate);
}

} // namespace pp
