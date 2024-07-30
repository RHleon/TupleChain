#ifndef PROCESSING_UNIT_INTERFACE_H
#define PROCESSING_UNIT_INTERFACE_H

#include <processing-unit.hpp>
#include <tuple>

namespace pp {

template<typename I>
struct ProcessingUnitInterface
{
    inline action_t processPacket(const packet_t& packet) {
	return (static_cast<I*>(this))->processPacket(packet);
    }

    inline bool processUpdate(const update_t& update) {
	return (static_cast<I*>(this))->processUpdate(update);
    }
	
    virtual uint64_t constructWithRules(int nRules, rule_t* rules) {
	return 0;
    }

    virtual std::tuple<uint64_t, std::string>
    constructWithRulesV(int nRules, rule_t* rules, int vid) {
	return std::make_tuple(0, "");
    }

    virtual std::tuple<uint64_t, std::string> constructWithRulesNew(int nRules, rule_t* rules) {
	return std::make_tuple(0, "");
    }
    
    virtual double
    matchTest(int nPackets, packet_t* packets, action_t* actions) { return 0; }

    virtual double
    matchTestV(int nPackets, packet_t* packets, action_t* actions, int num) { return 0; }

    virtual std::tuple<double, std::string>
    matchTest(int nPackets, packet_t* packets, action_t* actions,
	      int batch, int locality)
    {
	return std::make_tuple(0, "");
    }
    
    virtual int adjustResults(int n, packet_t* packets, action_t* actions) { return n; };

    virtual int generateResult(int n_Packets, packet_t* packet, rule_t * rules, int n_Rules, action_t* actions)	{ return 0;	}; 

    std::string getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

private:
    ProcessingUnitInterface(const std::string& name) : m_name(name) {};
    std::string m_name;

    friend I;
};
  
} // namespace pp

#endif
