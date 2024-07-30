#ifndef RULE_TRAFFIC_HPP
#define RULE_TRAFFIC_HPP

#include <packet-io.hpp>
#include <packet-classification.hpp>
#include <ip-lookup-interface.hpp>
#include <update-manager.hpp>
#include <vector>
#include <set>
#include <cstdio>
#include <string>
#include <tuple>
#include <functional>
#include <algorithm>

struct rule_item{
  uint32_t data;
  uint8_t prefix_len;

  bool operator<(const rule_item& tmp) const
  {
	  return this->data < tmp.data;
  }
};
typedef struct rule_item rule_item;

extern std::vector< std::vector<rule_item> > rule_vec;
extern std::vector< std::vector<rule_item> > ins_vec;
extern std::set< std::vector<rule_item> > del_set;
extern std::string data_prefix;

namespace pp {

#define SAFE_OPEN_FILE(file, path)					\
    FILE* file = fopen(std::string(path).c_str(), "r");			\
    if (!file) LOG_ERROR("can not open file: %s\n", std::string(path).c_str());		

#define SAFE_OPEN_FILE_OPT(file, path, opt)				\
    FILE* file = fopen(std::string(path).c_str(), opt);			\
    if (!file) LOG_ERROR("can not open file: %s\n", std::string(path).c_str());

#define SAFE_CLOSE_FILE(file) if(file) fclose(file);

#define GET_FILE_SIZE_RB(file, len)		\
    fseek (file , 0 , SEEK_END);		\
    auto len = ftell (file);			\
    rewind (file);				\

#define CREATE_PREFIX_MAP(rules, map, prefix_t, key_t, maxLen)		\
  std::unordered_map<key_t, prefix_t*> map[maxLen + 1];			\
  for (auto&& prefix : rules) {						\
    map[prefix.len][MAP_KEY(prefix.addr, prefix.len, maxLen)] = &prefix; \
  }
#define SEARCH_PREFIX_MAP_ADDR(addr, route, map, maxLen)		\
  for (int i = maxLen; i > 0; -- i) {					\
    auto res = map[i].find(MAP_KEY(addr, i, maxLen));			\
    if (res != map[i].end()) { route = res->second->route; break; }	\
  }
#define MERGE_UPDATE(rules, updates, prefix_t, key_t, maxLen) {		\
    CREATE_PREFIX_MAP(rules, maps, prefix_t, key_t, maxLen);		\
    std::vector<prefix_t> ins;						\
    for (const auto& update : updates) {				\
      auto key = MAP_KEY(update.prefix.addr, update.prefix.len, maxLen); \
      auto hit = maps[update.prefix.len].find(key);			\
      if (hit == maps[update.prefix.len].end()) {			\
	if (update.prefix.route != 0) ins.push_back(update.prefix);	\
      } else {								\
	hit->second->route = update.prefix.route;			\
      }									\
    }									\
    std::copy(ins.begin(), ins.end(), std::back_inserter(rules));	\
    rules.erase(std::remove_if(rules.begin(), rules.end(),		\
			       [] (const prefix_t& prefix) { return prefix.route == 0; }), \
		rules.end());						\
  }
#define MAP_KEY(addr, len, maxLen) (((addr) >> (maxLen - (len))) << (maxLen - (len)))
#define ADJUST_LEN(len, addr_t) (len > sizeof(addr_t) * 8 ? sizeof(addr_t) * 8 : len)

enum DataType {
  MULTI_FIELD = 0,
  FIVE_TUPLE,
  MULTI_TO_PC, // for generation only
  IPV4,
  IPV6,
  VIP4,
  VIP6
};

/**
 * generate packet classification rule files
*/
std::string
generatePcRuleFile(const char* filePath,
				std::string data_base_dir);

/**
 * rule generator
*/
std::string
generateRuleFile(const char* filePath, 
					int rule_nb,
					int field_nb,
					std::string data_base_dir);

/**
 * all match traffic generator
*/
std::string
generateTrafficFromVec(int tr_nb, 
						int field_nb, 
						std::string data_base_dir);

/**
 * random traffic match generator
*/
std::string
generateTrafficByRandom(int tr_nb, int field_nb, std::string data_base_dir,
			int type, const std::vector<rule_item>& rrc_vec);

/**
 * real traffic match generator
*/
std::string
generateRealTraffic(const char * real_traffic_file, 
					int number,
					std::string data_base_dir);

/**
 * create update traffic by ins_vec and del_vec 
*/
std::string
generateTrafficOfUpadate(int tr_nb, 
						int field_nb,
						std::string data_base_dir);

/**
 * create packet classification traffic from file
*/
std::string
generateTrafficOfPcFromFile(const char * filePath,
					std::string data_base_dir);

/**
 * match result generator
*/
int
generateResult(const char * rule_Path, const char * traffic_Path);

void
generateDataFile(const char* filePath, int data_nb, int field_nb,
		 std::string data_base_dir,
		 int type,
		 std::function<pp::ProcessingUnit*(void)> newPU);

/**
 * convert rules from multi fields to pc
*/
int
convertRuleM2P(const char * filePath, const char * dst_dir);

/**
 * convert traffic from multi fields to pc
*/
int
convertTrafficM2P(const char * filePath, const char * dst_dir);

int
generateLPMTrafficRandom(const std::string& trafficPath, int nPackets, int type,
			 std::function<pp::ProcessingUnit*(void)> newPU);

int
generateLPMTraffic(int type, const std::string& rulePath, const std::string& updatePath,
		   const std::string& trafficPath, int nRules = 0);

int
generateVirtualRouterData(int type, int nInstances, int nRules, std::string fibPath);
						
int
loadRuleFromFile(const char* path,
		 rule_t*& rules,
		 int type = DataType::MULTI_FIELD);

int
loadTrafficFromFile(const char* path,
		    packet_t*& packets,
		    int type = DataType::MULTI_FIELD);

int
loadResultFromFile(const char* path, action_t*& actions, int type = DataType::MULTI_FIELD);

std::tuple<int, int>
loadUpdatesFromFile(const char* path, update_t*& inertions, update_t*& deletions,
		    int type = DataType::MULTI_FIELD, int nRules = 0);

void
freeRule(rule_t& rule, int type = DataType::MULTI_FIELD);

void
freeRules(int nRules, rule_t*& rules,
	  int type = DataType::MULTI_FIELD);

void
freePackets(int nPackets, packet_t*& packets);

void
freeActions(int nPackets, action_t*& actions);

void
freeUpdates(int nUpdates, update_t*& updates,
	    int type = DataType::MULTI_FIELD);

} // namespace pp

#endif // RULE_TRAFFIC_HPP
