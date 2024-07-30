#ifdef __linux__
#include <sys/stat.h>
#endif
#include <rule-traffic.hpp>
#include <memory-manager.hpp>
#include <logger.hpp>
#include <tss.hpp>//generateMatchResult()
#include <pkt-cla-type.hpp>
#include <pkt-cla-helper.hpp>
#include <string.h>
#include <vip-lookup-interface.hpp>

#include <iostream>
#include <cassert>
#include <random>
#include <functional>
#include <algorithm>    // std::shuffle
#include <chrono>       // std::chrono::system_clock
#include <unordered_map>

#include <sys/socket.h>//convert network address from uint32_t to char*
#include <netinet/in.h>
#include <arpa/inet.h>

/******************global data**********************/
//1.Multi field data
std::vector<rule_item> rrc_vec;
std::set< std::vector<rule_item> > rule_set;
std::vector< std::vector<rule_item> > rule_vec;
std::vector< std::vector<rule_item> > ins_vec;
std::set< std::vector<rule_item> > del_set;

//2.packet classification data
std::vector< pkt_classify > pc_vec;

std::string data_prefix;
/***************************************************/

namespace pp {

std::string
createRules(int rule_nb, 
            int field_nb, 
            std::string & data_base_dir){
  
  //copy from set to vector
  std::insert_iterator< std::vector< std::vector<rule_item> > >copy_iterator(rule_vec, rule_vec.begin());
  std::copy(rule_set.begin(), rule_set.end(), copy_iterator);

  //copy last 10% rule to ins_vec
  std::vector< std::vector<rule_item> >::iterator update_begin_iter = rule_vec.begin() + rule_nb;
  std::insert_iterator< std::vector< std::vector<rule_item> > >ins_copy_iterator(ins_vec, ins_vec.begin());
  std::copy(update_begin_iter, rule_vec.end(), ins_copy_iterator);

  //earse the update rules in rule_vec after all of them has been copied to ins_vec
  rule_vec.erase(update_begin_iter, rule_vec.end());

  //create rule files
  char rule_path[256]={};
  char update_path[256]={};
  char update_temp_path[256]={};

  sprintf(rule_path, "%s/rule/%s.rule", data_base_dir.c_str(), data_prefix.c_str());
  FILE* rule_file = fopen(rule_path, "w");
  if(!rule_file){
    LOG_ERROR("can not open file %s", rule_path);
  }

  sprintf(update_path, "%s/update/%s.update", data_base_dir.c_str(), data_prefix.c_str());
  FILE* update_file = fopen(update_path, "w");
  if(!update_file){
    LOG_ERROR("can not open file %s", update_path);
  }

  sprintf(update_temp_path, "%s/rule/%s.rule.update.temp", data_base_dir.c_str(), data_prefix.c_str());
  FILE* update_temp_file = fopen(update_temp_path, "w");
  if(!update_temp_file){
    LOG_ERROR("can not open file %s", update_temp_path);
  }

  //write nFields and nRules
  int insert_nb = 0, delete_nb = 0;
  insert_nb = ins_vec.size();
  delete_nb = rule_nb * 10 / 100;

  fprintf(rule_file, "%d %d\n", field_nb, rule_nb);
  fprintf(update_temp_file, "%d %d\n", field_nb, rule_nb + insert_nb - delete_nb);

  //write nFields, insert_nb, delete_nb
  fprintf(update_file, "%d %d %d\n", field_nb, insert_nb, delete_nb);

  //write rule
  for(int i = 0; i < rule_nb; i++){
    //write priority
    fprintf(rule_file, "%d ", (i+1));

    //write action
    fprintf(rule_file, "%d ", (i+1));

    //write one delete rule
    if( i < delete_nb ){
      //type
      fprintf(update_file, "0 ");

      //priority
      fprintf(update_file, "%d ", (i+1));

      //action
      fprintf(update_file, "%d", (i+1));
    }
    //write update template rule
    else{//write priority
      fprintf(update_temp_file, "%d ", (i+1));

      //write action
      fprintf(update_temp_file, "%d ", (i+1));
    }

    std::vector< rule_item > del_temp;

    for(int j = 0; j<rule_vec[i].size(); j++){

      fprintf(rule_file, " %.8x/%u", rule_vec[i][j].data, rule_vec[i][j].prefix_len);

      //write delete rule
      if(i < delete_nb){
        fprintf(update_file, " %.8x/%u", rule_vec[i][j].data, rule_vec[i][j].prefix_len);
        del_temp.push_back( rule_vec[i][j] );
      }
      //write update temp rule
      else{
        fprintf(update_temp_file, " %.8x/%u", rule_vec[i][j].data, rule_vec[i][j].prefix_len);
      }
    }
    if( i < delete_nb){
      del_set.insert(del_temp);
      fprintf(update_file, "\n");
    }
    else{
      fprintf(update_temp_file, "\n");
    }

    fprintf(rule_file ,"\n");
  }

  //write insert rule
  // for(int i = 0; i < insert_nb ins_vec.size(); i++){
    for(int i = 0; i < ins_vec.size(); i++){
      //type
      fprintf(update_file, "1 ");

      //priority
      fprintf(update_file, "%d ", (rule_nb + i+1));
      fprintf(update_temp_file, "%d ", (rule_nb + i+1));

      //action
      fprintf(update_file, "%d", (rule_nb + i+1));
      fprintf(update_temp_file, "%d ", (rule_nb + i+1));

      for(int j = 0; j<ins_vec[i].size(); j++){
        fprintf(update_file, " %.8x/%u", ins_vec[i][j].data, ins_vec[i][j].prefix_len);
        fprintf(update_temp_file, " %.8x/%u", ins_vec[i][j].data, ins_vec[i][j].prefix_len);
    }

    fprintf(update_file, "\n");
    fprintf(update_temp_file, "\n");

  }

  fclose(rule_file);
  fclose(update_file);
  fclose(update_temp_file);
  return std::string(rule_path);
}

std::string
generateRuleFile(const char* filePath, 
					int rule_nb,
					int field_nb,
					std::string data_base_dir)
{
  FILE* file = fopen(filePath, "r");
  if(!file){
    LOG_ERROR("can not open file %s", filePath);
  }

  int nRules = 0, rule_ip = 0, prefix_len = 0,tot_rrc = 0;

  while( fscanf(file, "%x/%d", &rule_ip, &prefix_len) != EOF){
    rule_item r_it;
    r_it.data = rule_ip;
    r_it.prefix_len = prefix_len;

    rrc_vec.push_back(r_it);
    tot_rrc += 1;
    // printf("%d\n", tot_rrc);
  }

  std::default_random_engine e(time(nullptr));
  std::uniform_int_distribution<> u(0, rrc_vec.size() - 1);

  //insert into rule set line by line
  //rule_nb + update_nb( rule_nb * 10% )
  int tot_nb = rule_nb + (rule_nb * 10 / 100);

  while(nRules < tot_nb){
    std::vector<rule_item> rule_temp;
    for(int i=0; i<field_nb; i++){
      rule_temp.push_back(rrc_vec[u(e)]);
    }
    rule_set.insert(rule_temp);
    nRules = rule_set.size();//update rule number
  }
  // nRules = rule_nb;

  char data_prefix_tmp[256] = "";
  sprintf(data_prefix_tmp, "%d_%d", field_nb, rule_nb);
  data_prefix = std::string(data_prefix_tmp);

  fclose(file);

  return createRules(rule_nb, field_nb, data_base_dir);

}

#define FILL_ITEM(item, data_tmp, mask_tmp_len){\
  item.data = (uint32_t)(data_tmp);\
  item.prefix_len = (uint8_t)(mask_tmp_len);\
  }

std::string
generatePcRuleFile(const char* filePath, std::string data_base_dir)
{
  FILE * file = fopen(filePath, "r");
  if(!file){
    LOG_ERROR("can not open file %s", filePath);
  }

  //init packet classification vector
  int pc_nb = init_pc_vec(file);

  if(pc_nb <= 0){
    printf("No packet classification rules..\n");
    fclose(file);
    return std::string("");
  }

  for(auto & iter : pc_vec){
    std::vector< rule_item > rule_temp;
    rule_item item_temp;

    //write s_addr
    FILL_ITEM(item_temp, iter.s_addr, iter.sa_mask);
    rule_temp.push_back(item_temp);
    
    //write d_addr
    FILL_ITEM(item_temp, iter.d_addr, iter.da_mask);
    rule_temp.push_back(item_temp);

    //write s_port
    FILL_ITEM(item_temp, iter.s_port.port, iter.s_port.mask + 16);
    rule_temp.push_back(item_temp);

    //write d_port
    FILL_ITEM(item_temp, iter.d_port.port, iter.d_port.mask + 16);
    rule_temp.push_back(item_temp);

    //write proto
    FILL_ITEM(item_temp, iter.proto, /*iter.pr_mask*/(uint8_t)32);
    rule_temp.push_back(item_temp);

    rule_set.insert(rule_temp);
  }

  //last 10% used to update
  int rule_nb = rule_set.size() * 10 / 11;

  //last '/' postion to get data file name prefix
  auto x = strrchr(filePath, '/');
  data_prefix = std::string(++ x);

  return createRules(rule_nb, 5, data_base_dir);

}

void generateDataFile(const char* filePath,
						int data_nb,
						int field_nb,
					  std::string data_base_dir,
            int type,
		        std::function<pp::ProcessingUnit*(void)> newPU)
{
  //rule file path
  std::string rule_path, update_rule_path;

  //traffic file path
  std::string match_path, random_path, update_path;

  //multi_field data
  if(type == 0){
    printf("[Multi fields data mode]\n");

    printf("read original rrc data from \"%s\"...\n", filePath);

	  rule_path = generateRuleFile(filePath, data_nb, field_nb, data_base_dir);
    update_rule_path = rule_path + std::string(".update.temp");
    printf("done!\n");

    //multi fields random traffic
    printf("Create random traffic and result...\n");
    random_path = generateTrafficByRandom(data_nb, field_nb, data_base_dir, type, rrc_vec);
    pp::ProcessingUnit::generateMatchResult(rule_path.c_str(), random_path.c_str(), newPU());
    printf("done!\n");
  }
  //packet classification data
  else if(type == 1){
    printf("[Packet classification data mode]\n");
    
    printf("read original pc rule from \"%s\"...\n", filePath);

    rule_path = generatePcRuleFile(filePath, data_base_dir);
    update_rule_path = rule_path + std::string(".update.temp");
    data_nb = rule_vec.size();
    field_nb = 5;
    printf("done!\n");

    //packet classification traffic
    printf("Create random traffic and result...\n");
    char pc_random_ori_file_path[256] = "";
    sprintf(pc_random_ori_file_path, "%s_trace", filePath);
    random_path = generateTrafficOfPcFromFile(pc_random_ori_file_path, data_base_dir);
    pp::ProcessingUnit::generateMatchResult(rule_path.c_str(), random_path.c_str(), newPU());
    printf("done!\n");
  }
  else{
    return;
  }

  //all match traffic
  printf("Create all match traffic and result...\n");
  match_path = generateTrafficFromVec(data_nb, field_nb, data_base_dir);
  pp::ProcessingUnit::generateMatchResult(rule_path.c_str(), match_path.c_str(), newPU());
  printf("done!\n");

  //update traffic
  printf("Create update traffic and result...\n");
  update_path = generateTrafficOfUpadate(data_nb + ins_vec.size() - del_set.size(), field_nb, data_base_dir);
  pp::ProcessingUnit::generateMatchResult(update_rule_path.c_str(), update_path.c_str(), newPU());
  printf("done!\n");

  printf("Remove template update rule file...\n");
  remove(update_rule_path.c_str());//remove template rule file of update
  printf("done!\n");

}

int
convertRuleM2P(const char * filePath, const char * dst_dir)
{
  //src rule file
  FILE* file = fopen(filePath, "r");
  int field_nb = 0, rule_nb = 0;

  if(!file){
    LOG_ERROR("open %s error", filePath);
  }
  else{
    printf("Read rule from %s \n", filePath);
  }

  fscanf(file, "%d %d\n", &field_nb, &rule_nb);
  assert(field_nb != 0 && rule_nb != 0);

  //dst rule file
  FILE* dst_file;
  char dst_path[256] = "";

  sprintf(dst_path, "%s\%d_%d_pc.rules", dst_dir, field_nb, rule_nb);
  dst_file = fopen(dst_path, "w");
  if(!dst_file){
    fclose(file);
    LOG_ERROR("open %s error", filePath);
  }

  char buf[1024] = "";
  char * fields = NULL;
  char * saveptr = NULL;
  struct in_addr addr_temp;
  uint32_t field = 0, mask = 0;
  int count = 0;

  uint32_t src, dst;
  int sm, dm;
  int pr, ac;
  while (fscanf(file, "%d %d %x/%d %x/%d", &pr, &ac, &src, &sm, &dst, &dm) != EOF) {
    //printf("%d %d %x %d %x %d\n", pr, ac, src, sm, dst, dm);
    uint8_t* sip = (uint8_t*)(&src);
    uint8_t* dip = (uint8_t*)(&dst);
    fprintf(dst_file, "@%d.%d.%d.%d/%d\t%d.%d.%d.%d/%d\t0 : 0\t0 : 0\t0x06/0xFF\n",
	    sip[3], sip[2], sip[1], sip[0], sm,
	    dip[3], dip[2], dip[1], dip[0], dm);
  }
  
  // while(fgets(buf, 1024, file) != NULL){

  //   //delete enter char
  //   buf[strlen(buf) - 1] = '\0';

  //   //write @
  //   fprintf(dst_file, "@");

  //   //write address
  //   fields = strtok_r(buf, " ", &(saveptr));
  //   while(fields != NULL){
  //     count++;

  //     //strip priority and action
  //     if(count > 2){
  //       // fprintf(dst_file, "%s\t", fields);
  //       assert(sscanf(fields, "%x/%u", &field, &mask) == 2);

  //       //convert little end byte order to big end byte order (from host to network byte order)
  //       //field = htobe32(field);
  //       addr_temp.s_addr = field;

  //       //convert to "x.x.x.x" address and write
  //       fprintf(dst_file, "%s/%u\t", inet_ntoa(addr_temp), mask);
  //       // for(int i=0; i<4; i++){
  //       //   if(i != 0)
  //       //     fprintf(dst_file, ".");
  //       //   fprintf(dst_file, "%u", field % 256);
  //       //   field >>= 8;
  //       // }
  //       // fprintf(dst_file, "/%u\t", mask);
  //     }

  //     fields = strtok_r(NULL, " ", &(saveptr));
  //   }
  //   count = 0;
  //   fields = NULL;
  //   saveptr = NULL;
    
  //   //write src port scope
  //   fprintf(dst_file, "0 : 0\t");

  //   //write dst port scope
  //   fprintf(dst_file, "0 : 0\t");

  //   //write protocol
  //   fprintf(dst_file, "0x06/0xFF\n");

  //   memset(buf, 0 ,1024);
  // }
  
  fclose(file);
  fclose(dst_file);
  return 0;
}

}//end namespace pp

#define FROM_IP_TO_VIP(ip, vip, vid)		\
    vip.vid = vid;				\
    vip.addr = ip.addr;				\
    vip.len = ip.len;				\
    vip.route = ip.route;			\

template<typename T, int L = sizeof(T) * 8> void
generateTrafficFromRule(std::vector<pp::vip_prefix_t<T>>& rules, FILE* trFile, FILE* resFile)
{
    std::unordered_map<T, pp::vip_prefix_p<T>> maps[L + 1];
    for (auto&& prefix : rules) {
	auto key = MAP_KEY(prefix.addr, prefix.len, L);
	maps[prefix.len][key] = &prefix;
    }

    int nRules = rules.size();
    std::vector<pp::vip_packet_t<T>> packets(nRules);
    std::vector<pp::route_t> routes(nRules);
    for (int i = 0; i < nRules; ++ i) {
	const auto& prefix = rules[i];
	packets[i].vid = prefix.vid;
	packets[i].addr = prefix.addr;
	routes[i] = 0;	
	for (int j = L; j > 0; -- j) {
	    auto res = maps[j].find(MAP_KEY(prefix.addr, prefix.len, L));
	    if (res != maps[j].end()) {
		routes[i] = res->second->route;
		break;
	    }
	}
    }

    // write traffic & result
    fwrite(packets.data(), sizeof(pp::vip_packet_t<T>), nRules, trFile);
    fwrite(routes.data(), sizeof(pp::route_t), nRules, resFile);
}

template<typename T, int L = sizeof(T) * 8> void
mergeUpdate(std::vector<pp::vip_prefix_t<T>>& rules, const std::vector<pp::vip_update_t<T>>& updates)
{
  std::vector<pp::vip_prefix_t<T>> ins;
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
  rules.erase(std::remove_if(rules.begin(), rules.end(), [maps] (const pp::vip_prefix_t<T>& prefix) {
	return maps[prefix.len].find(MAP_KEY(prefix.addr, prefix.len, L)) != maps[prefix.len].end();
  }), rules.end());
}

template<typename T> void
generateVirtualRouterDataInstance(int vid, int nRules, int nUpdates,
				  const std::vector<pp::ip_prefix<T>>& allPrefixes,
				  const std::vector<pp::ip_update<T>>& allUpdates,
				  FILE* ruleFile, FILE* trFile, FILE* resFile,
				  FILE* updateFile, FILE* updateTrFile, FILE* updateResFile)
{
    // from ip to vip with vid
    std::vector<pp::vip_prefix_t<T>> prefixes(nRules);
    std::vector<pp::vip_update_t<T>> updates(nUpdates);
    for (int i = 0; i < nRules; ++ i) {
	FROM_IP_TO_VIP(allPrefixes[i], prefixes[i], vid);
    }
    for (int i = 0; i < nUpdates; ++ i) {
	updates[i].time = allUpdates[i].time;
	FROM_IP_TO_VIP(allUpdates[i].prefix, updates[i].prefix, vid);
    }
    std::sort(prefixes.begin(), prefixes.end(), [] (const pp::vip_prefix_t<T>&x,
						    const pp::vip_prefix_t<T>&y) {
		  return x.route < y.route;
	      });
    std::sort(updates.begin(), updates.end(), [] (const pp::vip_update_t<T>&x,
						  const pp::vip_update_t<T>&y) {
		  return x.prefix.route < y.prefix.route;
	      });

    // write rule & traffic
    fwrite(prefixes.data(), sizeof(pp::vip_prefix_t<T>), nRules, ruleFile);
    generateTrafficFromRule<T>(prefixes, trFile, resFile);

    // write updates & traffic
    fwrite(updates.data(), sizeof(pp::vip_update_t<T>), nUpdates, updateFile);
    mergeUpdate<T>(prefixes, updates);
    generateTrafficFromRule<T>(prefixes, updateTrFile, updateResFile);
}


template<typename T>
void load_ip_prefix_from2(std::string path, std::vector<pp::ip_prefix<T>>& container)
{
    SAFE_OPEN_FILE(file, path.c_str());
    uint64_t addr; uint8_t len; uint32_t route = 0;
    while (fscanf(file, "%llX/%hhu", &addr, &len) != EOF) {
	container.push_back({.addr = (T)addr, .len = len, .route = ++ route});
    }
    fclose(file);
}

template<typename T>
void load_ip_update_from2(std::string path, std::vector<pp::ip_update<T>>& container,
			 uint32_t startRoute)
{
    SAFE_OPEN_FILE(file, path.c_str());
    uint64_t time; int count;						
    uint64_t addr; uint8_t len;	int type;				
    pp::ip_prefix<T> prefix;				
    while (fscanf(file, "%llu %d", &time, &count) != EOF) {		
      for (int i = 0; i < count; ++ i) {				
	assert(fscanf(file, "%d %llX/%hhu", &type, &addr, &len) != EOF);	
	prefix = {.addr = (T)addr, .len = len, .route = type ? ++ startRoute : 0}; 
	container.push_back({.time = time, .prefix = prefix});		
      }									
    }
    fclose(file);
}

template<typename T> int
generateVirtualRouterDataIml(int nInstances, int nRules, const std::string& fibPath)
{
    std::string fibFile = fibPath + ".rule";
    std::string updFile = fibPath + ".update";
    std::vector<pp::ip_prefix<T>> prefixes;
    std::vector<pp::ip_update<T>> updates;
    load_ip_prefix_from2<T>(fibFile, prefixes);
    load_ip_update_from2<T>(updFile, updates, prefixes.size());
    
    int nUpdates = updates.size() * nRules / prefixes.size();
    std::string dataPath = fibPath +
	"_" + std::to_string(nInstances) + "_" + std::to_string(nRules);
    LOG_INFO("generate VR for %s\n", dataPath.c_str());

    SAFE_OPEN_FILE_OPT(ruleFile, dataPath + ".rule", "wb");
    SAFE_OPEN_FILE_OPT(trFile, dataPath + ".traffic", "wb");
    SAFE_OPEN_FILE_OPT(resFile, dataPath + ".traffic.result", "wb");
    SAFE_OPEN_FILE_OPT(updateFile, dataPath + ".update", "wb");
    SAFE_OPEN_FILE_OPT(updateTrFile, dataPath + ".update.traffic", "wb");
    SAFE_OPEN_FILE_OPT(updateResFile, dataPath + ".update.traffic.result", "wb");
    
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine e(seed);

    for (int i = 1; i <= nInstances; ++ i) {
	std::shuffle(prefixes.begin(), prefixes.end(), e);
	std::shuffle(updates.begin(), updates.end(), e);
	generateVirtualRouterDataInstance<T>(i, nRules, nUpdates, prefixes, updates,
					     ruleFile, trFile, resFile,
					     updateFile, updateTrFile, updateResFile);
    }

    SAFE_CLOSE_FILE(ruleFile);
    SAFE_CLOSE_FILE(trFile);
    SAFE_CLOSE_FILE(resFile);
    SAFE_CLOSE_FILE(updateFile);
    SAFE_CLOSE_FILE(updateTrFile);
    SAFE_CLOSE_FILE(updateResFile);
    
    return 0;
}

int
loadMultiFieldRule(FILE* file, pp::rule_t*& outRules)
{
  // line-1: nFileds nPackets
  // following nPackets lines, each of which includes [priority] [action] {[field]}
  int nFields, nRules;
  assert(fscanf(file, "%d %d", &nFields, &nRules) == 2);

  pp::pc_rule_t* rules = MEM_ALLOC(nRules, pp::pc_rule_t);
  assert(rules);
  
  for(int i = 0; i < nRules; ++ i) {
    rules[i] = (pp::pc_rule_t)MEM_ALLOC(1, pp::pc_rule);
    rules[i]->nFields = nFields;
    rules[i]->fields = MEM_ALLOC(nFields, uint32_t);
    rules[i]->masks = MEM_ALLOC(nFields, uint32_t);
    assert(rules[i]->fields != NULL && rules[i]->masks != NULL);
    
    assert(fscanf(file, "%d", &rules[i]->priority) == 1);
    assert(fscanf(file, "%u", &rules[i]->action) == 1);
    uint32_t field;
    uint8_t length;
    for (int j = 0; j < nFields; ++ j) {
      assert(fscanf(file, "%x/%hhu", &field, &length) == 2);
      rules[i]->masks[j] = pp::MASK_LEN_32[length];
      rules[i]->fields[j] = field & rules[i]->masks[j];
    }
  }

  outRules = (pp::rule_t*)rules;
  return nRules;
}

int
loadClassBenchRule(FILE* file, pp::rule_t*& rules)
{
  // TODO.
  int nFields, nRules;
  assert(fscanf(file, "%d %d", &nFields, &nRules) == 2);

  pp::pc_rule_t* rules_tmp = MEM_ALLOC(nRules, pp::pc_rule_t);
  assert(rules != NULL);

  for(int i = 0; i < nRules; i++){
    rules_tmp[i] = (pp::pc_rule_t)MEM_ALLOC(1, pp::pc_rule);
    rules_tmp[i]->nFields = nFields;
    rules_tmp[i]->fields = MEM_ALLOC(nFields, uint32_t);
    rules_tmp[i]->masks = MEM_ALLOC(nFields, uint32_t);
    assert(rules_tmp[i]->fields != NULL && rules_tmp[i]->masks != NULL);

    assert(fscanf(file, "%d", &rules_tmp[i]->priority) == 1);
    assert(fscanf(file, "%d", &rules_tmp[i]->action) == 1);
    uint32_t field;
    uint8_t length;
    for(int j = 0; j < nFields; ++j){
      assert(fscanf(file, "%x/%hhu", &field, &length) == 2);
      rules_tmp[i]->masks[j] = pp::MASK_LEN_32[length];
      rules_tmp[i]->fields[j] = field & rules_tmp[i]->masks[j];
    }
  }

  rules = (pp::rule_t *)rules_tmp;

  return nRules;
}

template<typename ip_addr>
int
loadIpPrefixes(const char* path, pp::rule_t*& rules)
{
    std::vector<pp::ip_prefix<ip_addr>> prefixes;
    load_ip_prefix_from2<ip_addr>(path, prefixes);
  
  int nRules = prefixes.size();
  pp::ip_prefix_p<ip_addr>* rules_tmp = MEM_ALLOC(nRules, pp::ip_prefix_p<ip_addr>);
  
  for (int i = 0; i < nRules; ++ i) {
    rules_tmp[i] = (pp::ip_prefix_p<ip_addr>)MEM_ALLOC(1, pp::ip_prefix<ip_addr>);
    rules_tmp[i]->addr = prefixes[i].addr;
    rules_tmp[i]->len = ADJUST_LEN(prefixes[i].len, ip_addr);
    rules_tmp[i]->route = i + 1;
  }

  rules = (pp::rule_t *)rules_tmp;
  return nRules;
}

template<typename T> int
loadVipPrefixes(const char* path, pp::rule_t*& rules)
{
    SAFE_OPEN_FILE_OPT(file, path, "rb");
    GET_FILE_SIZE_RB(file, size);

    auto elemSize = sizeof(pp::vip_prefix_t<T>);
    auto nRules = size / elemSize;
    assert(elemSize * nRules == size);

    auto rules_tmp = MEM_ALLOC(nRules, pp::vip_prefix_p<T>);
    for (int i = 0; i < nRules; ++ i) {
	rules_tmp[i] = MEM_ALLOC(1, pp::vip_prefix_t<T>);
	assert(rules_tmp[i] && 1 == fread(rules_tmp[i], elemSize, 1, file));
    }

    rules = (pp::rule_t *)rules_tmp;
    return nRules;
}

void
freeMultiFieldRule(pp::rule_t& outRule)
{
  pp::pc_rule_t rule = (pp::pc_rule_t)outRule;
  if (!rule) return;
  
  MEM_FREE(rule->fields);
  MEM_FREE(rule->masks);
  MEM_FREE(rule);
  
  outRule = NULL;
}

void
freeClassBenchRule(pp::rule_t& rule)
{
  // TODO.
  pp::pc_rule_t rule_tmp = (pp::pc_rule_t)rule;
  if(!rule) return;

  MEM_FREE(rule_tmp->fields);
  MEM_FREE(rule_tmp->masks);
  MEM_FREE(rule_tmp);

  rule = NULL;
}

template<typename ip_addr> void
freeIpPrefix(pp::rule_t& rule)
{
  if(!rule) return;
  pp::ip_prefix_p<ip_addr> rule_tmp = (pp::ip_prefix_p<ip_addr>)rule;
  MEM_FREE(rule_tmp);
  rule = NULL;
}

template<typename T> void
freeVipPrefix(pp::rule_t& rule)
{
  if(!rule) return;
  pp::vip_prefix_p<T> rule_tmp = (pp::vip_prefix_p<T>)rule;
  MEM_FREE(rule_tmp);
  rule = NULL;
}

void
freeRulesIml(int nRules, pp::rule_t*& rules, std::function<void(pp::rule_t&)> freeRule)
{
  if (!rules) return;
  
  for (int i = 0; i < nRules; ++i) {
    freeRule(rules[i]);
  }
  MEM_FREE(rules);
  rules = NULL;  
}

namespace pp {

int
generateVirtualRouterData(int type, int nInstances, int nRules, std::string fibPath)
{
  switch (type) {
  case DataType::VIP4:
      return generateVirtualRouterDataIml<pp::ipv4_addr>(nInstances, nRules, fibPath);
  case DataType::VIP6:
      return generateVirtualRouterDataIml<pp::ipv6_addr>(nInstances, nRules, fibPath);
  default : LOG_ERROR("invalid IP version\n");
  }

  return 0;    
}

int
loadRuleFromFile(const char* path, rule_t*& rules, int type)
{
  SAFE_OPEN_FILE(file, path);

  int res = 0;
  switch(type) {
  case DataType::MULTI_FIELD: res = loadMultiFieldRule(file, rules); break;
  case DataType::FIVE_TUPLE:  res = loadClassBenchRule(file, rules); break;
  case DataType::IPV4: fclose(file); return loadIpPrefixes<ipv4_addr>(path, rules);
  case DataType::IPV6: fclose(file); return loadIpPrefixes<ipv6_addr>(path, rules);
  case DataType::VIP4: fclose(file); return loadVipPrefixes<ipv4_addr>(path, rules);
  default : fclose(file); LOG_ERROR("INVALID DataType in loading rules\n");
  }

  return res;
}

void
freeRule(rule_t& rule, int type)
{
  switch(type) {
  case DataType::MULTI_FIELD: freeMultiFieldRule(rule); return;
  case DataType::FIVE_TUPLE:  freeClassBenchRule(rule); return;
  case DataType::IPV4:        freeIpPrefix<ipv4_addr>(rule); return;
  case DataType::IPV6:        freeIpPrefix<ipv6_addr>(rule); return;
  case DataType::VIP4:        freeVipPrefix<ipv4_addr>(rule); return;
  default : LOG_ERROR("INVALID DataType in freeing rule\n");
  }
}

void
freeRules(int nRules, rule_t*& rules, int type)
{
  switch(type) {
  case DataType::MULTI_FIELD: freeRulesIml(nRules, rules, freeMultiFieldRule); return;
  case DataType::FIVE_TUPLE:  freeRulesIml(nRules, rules, freeClassBenchRule); return;
  case DataType::IPV4:        freeRulesIml(nRules, rules, freeIpPrefix<ipv4_addr>); return;
  case DataType::IPV6:        freeRulesIml(nRules, rules, freeIpPrefix<ipv6_addr>); return;
  case DataType::VIP4:        freeRulesIml(nRules, rules, freeVipPrefix<ipv4_addr>); return;
  default : LOG_ERROR("INVALID DataType in freeing rules\n");
  }
}
  
} // namespace pp
