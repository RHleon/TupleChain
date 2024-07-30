#ifndef PACKET_IO_HPP
#define PACKET_IO_HPP

#include <stdint.h>

namespace pp {

typedef uint8_t* packet_t;
typedef uint32_t action_t;
#define DEFAULT_ACTION 0

struct IOStatistic
{
  uint64_t nSentPackets;
  uint64_t nReceivedPackets;
  double txRate;
  double rxRate;
  int testTime; // seconds

  IOStatistic()
    : nSentPackets(0)
    , nReceivedPackets(0)
    , txRate(0)
    , rxRate(0)
    , testTime(0)
  {}
};

  
class PacketIO
{
public:
    virtual ~PacketIO() {};
    
    virtual void
    start() = 0;

    virtual void
    stop() = 0;
  
    /** 
     * pull an array of packets from the underlying IO
     * 
     * @param pkts the array to store pulled packets
     * @param max_num pull at most @max_num packets
     * 
     * @return the actual number of pulled packets
     */
    virtual int
    pullPackets(packet_t* pkts, int max_num) = 0;

    virtual packet_t*
    pullPackets(int& num, int max_num) { return nullptr; }
    
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
    pushPackets(packet_t* pkts, action_t* actions, int num) = 0;

public:
    virtual PacketIO* getPacketIO() { return this; }

public:
    IOStatistic
    getStatistic() {return m_statistic; }
  
protected:
    IOStatistic m_statistic;
};

} // namespace pp

#endif // PACKET_IO_HPP
