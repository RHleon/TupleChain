#include <rule-traffic.hpp>
#include <memory-manager.hpp>
#include <logger.hpp>
#include <stdio.h>
#include <assert.h>
#include <random>
#include <algorithm>//std::find
#include <pkt-cla-type.hpp>
#include <string.h>
#include <unordered_map>
#include <vip-lookup-interface.hpp>

namespace pp{

std::string
generateTrafficFromVec(int tr_nb, 
						int field_nb, 
						std::string data_base_dir)
{
  char filePath[256] = ""; 
  sprintf(filePath, "%s/traffic/%s_match.traffic", data_base_dir.c_str(), data_prefix.c_str());
  FILE* file = fopen(filePath, "w");
  if (!file) {
    LOG_ERROR("can not open file %s", filePath);
  }

  //write nfields and nPackets
  fprintf(file, "%d %d\n", field_nb, tr_nb);

  for(int i=0; i<tr_nb; i++){
    for(int j=0; j<field_nb; j++){
      if(j != 0)
        fprintf(file, " ");
      fprintf(file, "%.8x", rule_vec[i][j].data);
    }
    fprintf(file, "\n");
  }

  fclose(file);
  return std::string(filePath);
}

std::string
generateTrafficByRandom(int tr_nb, int field_nb, std::string data_base_dir,
			int type, const std::vector<rule_item>& rrc_vec)
{
  char filePath[256] = ""; 
  sprintf(filePath, "%s/traffic/%s_random.traffic", data_base_dir.c_str(), data_prefix.c_str()); 
  FILE* file = fopen(filePath, "w");
  if (!file) {
    LOG_ERROR("can not open file %s", filePath);
  }

  //write nFields and nPackets
  fprintf(file, "%d %d\n", field_nb, tr_nb);

  //write random traffic data
  std::default_random_engine e(time(nullptr));
  std::uniform_int_distribution<uint32_t> randomAddr(0);
  std::uniform_int_distribution<> randomIdx(0, rrc_vec.size() - 1);

  for(int i=0; i<tr_nb; i++){
    for(int j=0; j<field_nb; j++){
      if(j != 0)
        fprintf(file, " ");
      //fprintf(file, "%.8x", type ? randomAddr(e) : rrc_vec[randomIdx(e)].data);
      fprintf(file, "%.8x", randomAddr(e));
    }
    fprintf(file, "\n");
  }

  fclose(file);
  return std::string(filePath);
}

std::string
generateTrafficOfUpadate(int tr_nb, 
						int field_nb,
						std::string data_base_dir)
{
  char filePath[256] = "";
  sprintf(filePath, "%s/update/%s.update.traffic", data_base_dir.c_str(), data_prefix.c_str());
  FILE *file = fopen(filePath, "w");
  if(!file){
    LOG_ERROR("can not open file %s", filePath);
  }

  //write nFields and nPackets
  fprintf(file, "%d %d\n", field_nb, tr_nb);

  //write undeleted ruless
  for(int i = 0 ; i < rule_vec.size(); i++){
    //check if it is a deleted rule
    if( del_set.find(rule_vec[i]) == del_set.end()){

      //write if it is not a deleted rule
      for(int j = 0; j < rule_vec[i].size(); j++){
        if(j != 0)
          fprintf(file, " ");
        fprintf(file, "%.8x", rule_vec[i][j].data);
      }

      fprintf(file, "\n");

    }
  }

  //write inserted rules
  for(int i = 0 ; i < ins_vec.size(); i++){

    for(int j = 0; j < ins_vec[i].size(); j++){
      if(j != 0)
        fprintf(file, " ");
      fprintf(file, "%.8x", ins_vec[i][j].data);
    }

    fprintf(file, "\n");
  }

  fclose(file);
  return std::string(filePath);
}

std::vector< pc_pkt > pc_pkt_v;

std::string
generateTrafficOfPcFromFile(const char * filePath,
					std::string data_base_dir)
{
  FILE * file = fopen(filePath, "r");
  if(!file){
    LOG_ERROR("Can not open file: %s", filePath);
  }

  char buf[1024]="";
  uint32_t s_addr_tmp;
  uint32_t d_addr_tmp;
  uint16_t s_port_tmp;
  uint16_t d_port_tmp;
  uint8_t proto_tmp;

  while( (fgets(buf, 1024, file)) != NULL){
    pc_pkt pc_pkt_tmp;
    
    sscanf(buf, "%u %u %hu %hu %hhu", &pc_pkt_tmp.s_addr, 
                                      &pc_pkt_tmp.d_addr, 
                                      &pc_pkt_tmp.s_port, 
                                      &pc_pkt_tmp.d_port, 
                                      &pc_pkt_tmp.proto);
    
    pc_pkt_v.push_back(pc_pkt_tmp);
  }

  std::string traffic_file_path = data_base_dir + std::string("/traffic/") + 
                                      data_prefix + 
                                      std::string("_random.traffic");
  FILE * traffic_file = fopen(traffic_file_path.c_str(), "w");
  if(traffic_file == NULL){
    fclose(file);
    LOG_ERROR("Can not open %s", traffic_file_path.c_str());
  }

  //write # of traffic, # of field
  fprintf(traffic_file, "%d %lu\n", 5, pc_pkt_v.size());

  for(auto & iter : pc_pkt_v){
    fprintf(traffic_file, "%.8x %.8x %x %x %x\n", iter.s_addr, iter.d_addr, iter.s_port, iter.d_port, iter.proto);
  }

  fclose(file);
  fclose(traffic_file);

  return traffic_file_path;
}

std::string
generateRealTraffic(const char * real_traffic_file, 
					int number,
					std::string data_base_dir)
{
  return std::string(" ");
}

static uint32_t MASK_32[33] = {
  0x0,
  0x80000000, 0xc0000000, 0xe0000000, 0xf0000000,
  0xf8000000, 0xfc000000, 0xfe000000, 0xff000000,
  0xff800000, 0xffc00000, 0xffe00000, 0xfff00000,
  0xfff80000, 0xfffc0000, 0xfffe0000, 0xffff0000,
  0xffff8000, 0xffffc000, 0xffffe000, 0xfffff000,
  0xfffff800, 0xfffffc00, 0xfffffe00, 0xffffff00,
  0xffffff80, 0xffffffc0, 0xffffffe0, 0xfffffff0,
  0xfffffff8, 0xfffffffc, 0xfffffffe, 0xffffffff
};

struct rule_field{
  uint32_t data;
  uint8_t prefix_len;
};
typedef struct rule_field rule_field;

struct rule_line{
  int priority;
  int action;
  std::vector<rule_field> rule_fields;
};
typedef struct rule_line rule_line;

struct rule_set{
  int field_nb;
  int rule_nb;
  std::vector<rule_line> rule_list;
};
typedef struct rule_set rule_set;

int
generateResult(const char * rule_Path, const char * traffic_Path)
{
  FILE* rule_file = fopen(rule_Path, "r");
  if(!rule_file){
    LOG_ERROR("can not open file %s", rule_Path);
  }

  FILE* traffic_file = fopen(traffic_Path, "r");
  if(!traffic_file){
    LOG_ERROR("can not open file %s", traffic_Path);
  }

  char result_Path[256] = "";
  sprintf(result_Path, "%s.result", traffic_Path);
  FILE* result_file = fopen(result_Path, "w");
  if(!result_file){
    LOG_ERROR("can not open file %s", result_Path);
  }

  //load rule set
  //read # of field and rules
  int nFields = 0, nRules = 0;
  rule_set rule_s;
  fscanf(rule_file, "%d %d\n", &nFields, &nRules);
  printf("read %d fields data, %d lines\n", nFields, nRules);
  rule_s.field_nb = nFields;
  rule_s.rule_nb = nRules;
  for(int i = 0; i < rule_s.rule_nb; i++){
    rule_line rule_line_temp;

    //read priority and action
    fscanf(rule_file, "%d %d", &rule_line_temp.priority, &rule_line_temp.action);

    //read fields
    for(int j = 0; j < rule_s.field_nb; j++){
      rule_field rule_field_temp;
      fscanf(rule_file, "%x/%hhu", &rule_field_temp.data, &rule_field_temp.prefix_len);
      rule_line_temp.rule_fields.push_back(rule_field_temp);
    }

    rule_s.rule_list.push_back(rule_line_temp);
  }

  //try match traffic line by line
  int nTrafficFields = 0, nTraffic = 0;
  fscanf(traffic_file, "%d %d\n",  &nTrafficFields, &nTraffic);

  //write result nb
  fprintf(result_file, "%d\n", nTraffic);

  for(int i = 0; i < nTraffic; i++){
    //read one line of traffic
    uint32_t field_temp;
    std::vector<uint32_t> traffic_item;
    for(int j = 0; j<nTrafficFields; j++){
      fscanf(traffic_file, "%x", &field_temp);
      traffic_item.push_back(field_temp);
    }

    //try match in rule_set
    //try each line of rule_list
    int max_priority = 0;
    int action = 0;

    for(int k = 0; k < rule_s.rule_nb; k++){
      //try each field of rule line
      rule_line & rl_temp = rule_s.rule_list[k];
      bool all_match = true;
      for(int l = 0; l < rule_s.field_nb; l++){
        if( (traffic_item[l] & MASK_32[rl_temp.rule_fields[l].prefix_len]) != rl_temp.rule_fields[l].data  ){
          all_match = false;
          break;
        }
      }

      //if match succeed in all fields
      if( all_match ){
        //update action when current rule line has higher priority
        if( rl_temp.priority >= max_priority){
          action = rl_temp.action;
        }
      }
    }

    //write result of current traffic line
    fprintf(result_file, "%d\n", action);
  }

  fclose(rule_file);
  fclose(traffic_file);
  fclose(result_file);
  return 0;
}

int
convertTrafficM2P(const char * filePath, const char * dst_dir)
{
  //src rule file
  FILE* file = fopen(filePath, "r");
  int field_nb = 0, rule_nb = 0;

  if(!file){
    LOG_ERROR("open %s error", filePath);
  }
  else{
    printf("Read traffic from %s \n", filePath);
  }

  fscanf(file, "%d %d\n", &field_nb, &rule_nb);
  assert(field_nb != 0 && rule_nb != 0);

  //dst rule file
  FILE* dst_file;
  char dst_path[256] = "";

  sprintf(dst_path, "%s\%d_%d_pc.rules.trace", dst_dir, field_nb, rule_nb);
  dst_file = fopen(dst_path, "w");
  if(!dst_file){
    fclose(file);
    LOG_ERROR("open %s error", filePath);
  }

  char buf[1024] = "";

  uint32_t src, dst;
  while (fscanf(file, "%x %x", &src, &dst) != EOF) {
    fprintf (dst_file, "%u\t%u\t0\t0\t6\n", src, dst);
  }
  // while(fgets(buf, 1024, file) != NULL){
  //   buf[strlen(buf) - 1] = '\0';
  //   fprintf(dst_file, "%s", buf);
  //   for(int i = field_nb; i<5; i++){
  //     fprintf(dst_file, " 0");
  //   }
  //   fprintf(dst_file, "\n");
  //   memset(buf, 0 ,1024);
  // }
  
  fclose(file);
  fclose(dst_file);
  return 0;
}

}//end namespace pp

int
loadMultiFieldTraffic(FILE* file, pp::packet_t*& packets)
{
  // line-1: nFileds nPackets
  // following nPackets lines, each of which includes nFileds 32-bit fileds
  int nFileds, nPackets;
  assert(fscanf(file, "%d %d", &nFileds, &nPackets) == 2);

  packets = MEM_ALLOC(nPackets, pp::packet_t);
  assert(packets);
  
  for(int i = 0; i < nPackets; ++ i) {
    pp::packet_t pkt = MEM_ALLOC(nFileds * 4, uint8_t);
    assert(pkt);
    
    for (int j = 0; j < nFileds; ++ j) {
      assert(fscanf(file, "%x", (uint32_t*)(pkt + j * 4)));
    }
    
    packets[i] = pkt;
  }
  
  return nPackets;
}

int
loadClassBenchTraffic(FILE* file, pp::packet_t*& packets)
{
  // TODO.
  return loadMultiFieldTraffic(file, packets);
}

template<typename ip_addr>
int
loadIpTraffic(FILE* file, pp::packet_t*& packets)
{
  // line-1: nFileds nPackets
  // following nPackets lines, each of which includes nFileds 32-bit fileds
  int nPackets;
  assert(fscanf(file, "%d", &nPackets) == 1);

  packets = MEM_ALLOC(nPackets, pp::packet_t);
  assert(packets);

  for(int i = 0; i < nPackets; ++ i) {
    pp::packet_t pkt = MEM_ALLOC(sizeof(ip_addr), uint8_t);
    assert(pkt);
    
    fscanf(file, ISV4(ip_addr) ? "%x" : "%llx", (ip_addr*)(pkt));
    packets[i] = pkt;
  }
  
  return nPackets;
}

template<typename T> int
loadVipTraffic(const char* path, pp::packet_t*& packets)
{
    SAFE_OPEN_FILE_OPT(file, path, "rb");
    GET_FILE_SIZE_RB(file, size);

    auto elemSize = sizeof(pp::vip_packet_t<T>);
    auto nPackets = size / elemSize;
    assert(elemSize * nPackets == size);

    packets = MEM_ALLOC(nPackets, pp::packet_t);
    assert(packets);

    for(int i = 0; i < nPackets; ++ i) {
	packets[i] = MEM_ALLOC(elemSize, uint8_t);
	assert(packets[i] && elemSize == fread(packets[i], 1, elemSize, file));
    }
  
    return nPackets;
}

template<typename T, int L = sizeof(T) * 8>
void makeIPTrafficFromRule(std::vector<pp::ip_prefix<T>>& rules,
			   const std::string& trafficPath, int nRules = 0)
{
    std::string resultPath = trafficPath + ".result";
    FILE* tr  = fopen(trafficPath.c_str(), "w");
    FILE* res = fopen(resultPath.c_str(), "w");
    assert(tr && res);

    // prepare the match structure
    std::unordered_map<T, pp::ip_prefix_p<T>> maps[L + 1];
    for (auto&& prefix : rules)
	maps[prefix.len][MAP_KEY(prefix.addr, prefix.len, L)] = &prefix;

    bool isRandom = nRules = 0;
    if (!isRandom) nRules = rules.size();
    fprintf(tr, "%lu\n", nRules);
    fprintf(res, "%lu\n", nRules);

    auto searchPrint = [&] (T ip) {
	fprintf(tr, ISV4(T) ? "%x\n" : "%llx\n", ip);
	uint32_t route = 0;
	for (int i = L; i > 0; -- i) {
	    auto res = maps[i].find(MAP_KEY(ip, i, L));
	    if (res != maps[i].end()) {
		route = res->second->route;
		break;
	    }
	}
	fprintf(res, "%u\n", route);
    };
    
    if (isRandom) {
	std::default_random_engine e(time(nullptr));
	std::uniform_int_distribution<T> randomAddr(0);
	while (nRules -- ) searchPrint(randomAddr(e));
    }
    else {
	for (const auto& prefix : rules) searchPrint(prefix.addr);
    }
  
    fclose(tr);
    fclose(res);
}

template<typename T, int L = sizeof(T) * 8>
void
mergeUpdate(std::vector<pp::ip_prefix<T>>& rules, const std::vector<pp::ip_update<T>>& updates)
{
  std::vector<pp::ip_prefix<T>> ins;
  std::unordered_map<T, bool> maps[L + 1];
  for (const auto& update : updates) {
    if (update.prefix.route == 0) {
      maps[update.prefix.len][MAP_KEY(update.prefix.addr, update.prefix.len, L)] = true;
    }
    else {
      ins.push_back(update.prefix);
    }
  }

  std::copy(ins.begin(), ins.end(), std::back_inserter(rules));
  rules.erase(std::remove_if(rules.begin(), rules.end(), [maps] (const pp::ip_prefix<T>& prefix) {
	return maps[prefix.len].find(MAP_KEY(prefix.addr, prefix.len, L)) != maps[prefix.len].end();
  }), rules.end());
}

template<typename T>
void load_ip_prefix_from(std::string path, std::vector<pp::ip_prefix<T>>& container)
{
    SAFE_OPEN_FILE(file, path.c_str());
    uint64_t addr; uint8_t len; uint32_t route = 0;
    while (fscanf(file, "%llX/%hhu", &addr, &len) != EOF) {
	
	container.push_back({.addr = (T)addr, .len = (uint8_t)ADJUST_LEN(len, T),
		    .route = ++ route});
    }
    fclose(file);
}

template<typename T>
void load_ip_update_from(std::string path, std::vector<pp::ip_update<T>>& container,
			 uint32_t startRoute)
{
    SAFE_OPEN_FILE(file, path.c_str());
    uint64_t time; int count;						
    uint64_t addr; uint8_t len;	int type;				
    pp::ip_prefix<T> prefix;				
    while (fscanf(file, "%llu %d", &time, &count) != EOF) {		
      for (int i = 0; i < count; ++ i) {				
	assert(fscanf(file, "%d %llX/%hhu", &type, &addr, &len) != EOF);	
	prefix = {.addr = (T)addr, .len = (uint8_t)ADJUST_LEN(len, T),
		  .route = type ? ++ startRoute : 0}; 
	container.push_back({.time = time, .prefix = prefix});		
      }									
    }
    fclose(file);
}

template<typename T, int L = sizeof(T) * 8>
int
makeIPTraffic(const std::string& rulePath, const std::string& updatePath,
	      const std::string& trafficPath, int nRules)
{
    std::vector<pp::ip_prefix<T>> rules;
    load_ip_prefix_from<T>(rulePath, rules);
    makeIPTrafficFromRule<T>(rules, trafficPath, nRules);
    if (nRules) return nRules; // random

    std::vector<pp::ip_update<T>> updates;
    load_ip_update_from<T>(updatePath, updates, rules.size());
    mergeUpdate<T>(rules, updates);
    makeIPTrafficFromRule<T>(rules, updatePath + ".traffic");
    return rules.size();
}

int
loadResultFromBinaryFile(const char* path, pp::action_t*& actions)
{
    SAFE_OPEN_FILE_OPT(file, path, "rb");
    GET_FILE_SIZE_RB(file, size);

    auto elemSize = sizeof(pp::action_t);
    auto nActions = size / elemSize;
    assert(elemSize * nActions == size);

    actions = MEM_ALLOC(nActions, pp::action_t);
    assert(actions && nActions == fread(actions, elemSize, nActions, file));

    return nActions;    
}
 
namespace pp {

int
generateLPMTrafficRandom(const std::string& trafficPath, int nPackets, int type,
			 std::function<pp::ProcessingUnit*(void)> newPU)
{
  // TODO.
  return 0;
}
  
int
generateLPMTraffic(int type, const std::string& rulePath, const std::string& updatePath,
		   const std::string& trafficPath, int nRules)
{
  switch (type) {
  case DataType::IPV4:
      return makeIPTraffic<pp::ipv4_addr>(rulePath, updatePath, trafficPath, nRules);
  case DataType::IPV6:
      return makeIPTraffic<pp::ipv6_addr>(rulePath, updatePath, trafficPath, nRules);
  default : LOG_ERROR("invalid IP version\n");
  }

  return 0;
}

int
loadTrafficFromFile(const char* path,
		    packet_t*& packets,
		    int type)
{
  SAFE_OPEN_FILE(file, path);
  
  int res = 0;
  switch(type) {
  case DataType::MULTI_FIELD: res = loadMultiFieldTraffic(file, packets); break;
  case DataType::FIVE_TUPLE:  res = loadClassBenchTraffic(file, packets); break;
  case DataType::IPV4:        res = loadIpTraffic<ipv4_addr>(file, packets); break;
  case DataType::IPV6:        res = loadIpTraffic<ipv6_addr>(file, packets); break;
  case DataType::VIP4: fclose(file); res = loadVipTraffic<ipv4_addr>(path, packets); break;
  default : fclose(file); LOG_ERROR("INVALID DataType in loading traffic\n");
  }

  fclose(file);
  return res;
}

int
loadResultFromFile(const char* path, action_t*& actions, int type)
{
    if (type == DataType::VIP4) {
	return loadResultFromBinaryFile(path, actions);
    }
    
  SAFE_OPEN_FILE(file, path);

  int nResults;
  assert(fscanf(file, "%d", &nResults) == 1);

  actions = MEM_ALLOC(nResults, action_t);
  assert(actions);

  for(int i = 0; i < nResults; ++ i) {
    assert(fscanf(file, "%u", &actions[i]) == 1);
  }
  
  fclose(file);
  return nResults;  
}

void
freePackets(int nPackets, packet_t*& packets)
{
  if (!packets) return;
  for (int i = 0; i < nPackets; ++i) MEM_FREE(packets[i]);
  MEM_FREE(packets);
}

void
freeActions(int nPackets, action_t*& actions)
{
  MEM_FREE(actions);
}
  
} // namespace pp
