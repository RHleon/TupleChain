#ifndef DUMMY_IO_HPP
#define DUMMY_IO_HPP

#include <packet-io.hpp>
#include <pp-thread.hpp>
#include <string>
#include <vector>
#include <stdio.h>
#include <inttypes.h>

namespace pp {

struct DummyIOParameters
{
  std::string traffic_file;
  int data_type;
  double tx_rate; // mpps
  int max_io_events;
  int test_seconds;
};
  
class DummyIO : public PacketIO
{
public:
  static void TEST(DummyIOParameters params);

public:
  DummyIO(DummyIOParameters params);
  ~DummyIO();
  
public:
  virtual void
  start() override;

  virtual void
  stop() override;
  
  /** 
   * pull an array of packets from the underlying IO
   * 
   * @param pkts the array to store pulled packets
   * @param max_num pull at most @max_num packets
   * 
   * @return the actual number of pulled packets
   */
  virtual int
  pullPackets(packet_t* pkts, int max_num) override;

  virtual packet_t*
  pullPackets(int& num, int max_num) final;
    
  /** 
   * push an array of @num packets to next step
   * 
   * @param pkts the packets to process
   * @param actions the determined actions
   * @param num the number of packets
   * 
   * @return the actual processed packets
   */
  virtual int
  pushPackets(packet_t* pkts, action_t* actions, int num) override;

public:
  struct Event {
    uint64_t time; // ns, from start
    int nPackets; // >0: tx; <0: rx; =0
  };
  
  void
  emulatorLoop(pp_thread_t thisThread);

  void
  analyseEvents(const std::vector<Event>& events, int nEvents);

  void
  show_queue_info(){
        printf("m_rxHead=%d, m_rxTail=%d\n", m_rxHead, m_rxTail);
        printf("m_txHead=%d, m_txTail=%d\n", m_txHead, m_txTail);
  }

private:
  DummyIOParameters m_configs;
  packet_t* m_packetBuffer;
  int m_nTotalPackets;
  
  pp_thread_t m_emulator;
  double m_testTxRate;
  int m_rxHead, m_rxTail;
  int m_txHead, m_txTail;
  uint32_t m_packs_nb_mask; 
};

} // namespace pp

#endif //DUMMY_IO_HPP
