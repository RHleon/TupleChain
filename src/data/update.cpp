#include <rule-traffic.hpp>
#include <vip-lookup-interface.hpp>
#include <common.hpp>

std::tuple<int, int>
loadMultiFieldUpdates(FILE* file, pp::update_t*& insertions, pp::update_t*& deletions)
{
  // line-1: nFileds nInsertions nDeletions
  // following nInsertions + nDeletions lines, each includes [type] [priority] [action] {[field]}
  
  int nFields, nInsertions, nDeletions;
  assert(fscanf(file, "%d %d %d", &nFields, &nInsertions, &nDeletions) == 3);

  insertions = MEM_ALLOC(nInsertions, pp::update_t);
  deletions = MEM_ALLOC(nDeletions, pp::update_t);
  assert(insertions != nullptr && deletions != nullptr);

  int ii = 0, dd = 0;
  int type;
  for (int k = nInsertions + nDeletions; k > 0; -- k) {
    auto rule = MEM_ALLOC(1, pp::pc_rule);
    assert(rule && fscanf(file, "%d", &type) == 1);
    
    rule->nFields = nFields;
    rule->fields = MEM_ALLOC(nFields, uint32_t);
    rule->masks = MEM_ALLOC(nFields, uint32_t);
    assert(rule->fields != NULL && rule->masks != NULL);
    
    assert(fscanf(file, "%d", &rule->priority) == 1);
    assert(fscanf(file, "%u", &rule->action) == 1);
    uint32_t field;
    uint8_t length;
    for (int j = 0; j < nFields; ++ j) {
      assert(fscanf(file, "%x/%hhu", &field, &length) == 2);
      rule->masks[j] = pp::MASK_LEN_32[length];
      rule->fields[j] = field & rule->masks[j];
    }
    
    if (type) {
      assert(ii < nInsertions);
      insertions[ii].rule = (pp::rule_t)rule;
      ii ++;
    }
    else {
      assert(dd < nDeletions);
      deletions[dd].rule = (pp::rule_t)rule;
      rule->action = 0; // flag as to be deleted
      dd ++;
    }
  }

  return std::make_tuple(nInsertions, nDeletions);
}

std::tuple<int, int>
loadClassBenchUpdates(FILE* file, pp::update_t*& insertions, pp::update_t*& deletions)
{
  // TODO.
  return std::make_tuple(0, 0);
}


template<typename T>
void load_ip_update_from1(std::string path, std::vector<pp::ip_update<T>>& container,
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

template<typename ip_addr> std::tuple<int, int>
loadIpUpdates(const char* path, pp::update_t*& insertions, pp::update_t*& deletions, int n)
{
    std::vector<pp::ip_update<ip_addr>> updates;
  load_ip_update_from1<ip_addr>(path, updates, n);
  int nDeletions = std::count_if(updates.begin(), updates.end(),
				 [](const pp::ip_update<ip_addr>& upd) {
				   return upd.prefix.route == 0; });
  int nInsertions = updates.size() - nDeletions;
  LOG_DEBUG("\n%lu from %llx to %llx, period: %lld s, %lld ms\n",
	   updates.size(), updates.front().time, updates.back().time,
	   (updates.back().time >> 32) - (updates.front().time >> 32),
	   (updates.back().time & 0xffffffff) - (updates.front().time & 0xffffffff));
  
  insertions = MEM_ALLOC(nInsertions, pp::update_t);
  deletions = MEM_ALLOC(nDeletions, pp::update_t);
  assert(insertions != nullptr && deletions != nullptr);

  int ii = 0, dd = 0;
  uint64_t lastTimeSlot = 0;
  uint64_t order = 0;
  std::sort(updates.begin(), updates.end(), [] (const pp::ip_update<ip_addr>&x,
						const pp::ip_update<ip_addr>&y) {
		return x.prefix.route < y.prefix.route;
	    });
  for (const auto& update : updates) {
    auto& entry = update.prefix.route ? insertions[ii ++] : deletions[dd ++];
    entry.time = {.s = update.time >> 32, .ms = update.time & 0xffffffff, .us = 0, .od = 0};
    if (update.time == lastTimeSlot) {
      entry.time.od = ++ order;
    } else {
      lastTimeSlot = update.time;
      order = 0;
    }
    
    auto prefix = MEM_ALLOC(1, pp::ip_prefix<ip_addr>);
    prefix->addr  = update.prefix.addr;
    prefix->len   = ADJUST_LEN(update.prefix.len, ip_addr);
    prefix->route = update.prefix.route;
    entry.rule = (pp::rule_t)prefix;
  }

  return std::make_tuple(nInsertions, nDeletions);
}

template<typename T> std::tuple<int, int>
loadVipUpdates(const char* path, pp::update_t*& insertions, pp::update_t*& deletions)
{
    SAFE_OPEN_FILE_OPT(file, path, "rb");
    GET_FILE_SIZE_RB(file, size);

    auto elemSize = sizeof(pp::vip_update_t<T>);
    auto nUpdates = size / elemSize;
    assert(elemSize * nUpdates == size);

    std::vector<pp::vip_update_t<T>> updates(nUpdates);
    assert(nUpdates == fread(updates.data(), elemSize, nUpdates, file));
    sort(updates.begin(), updates.end(), [] (const pp::vip_update_t<T>& x,
					     const pp::vip_update_t<T>& y) {
	     return x.prefix.route < y.prefix.route;
	 });

    int nDeletions = std::count_if(updates.begin(), updates.end(),
				   [](const pp::vip_update_t<T>& upd) {
				       return upd.prefix.route == 0; });
    int nInsertions = updates.size() - nDeletions;
    LOG_DEBUG("\n%lu from %llx to %llx, period: %lld s, %lld ms\n",
	      updates.size(), updates.front().time, updates.back().time,
	      (updates.back().time >> 32) - (updates.front().time >> 32),
	      (updates.back().time & 0xffffffff) - (updates.front().time & 0xffffffff));
  
    insertions = MEM_ALLOC(nInsertions, pp::update_t);
    deletions = MEM_ALLOC(nDeletions, pp::update_t);
    assert(insertions && deletions);

    int ii = 0, dd = 0;
    uint64_t lastTimeSlot = 0;
    uint64_t order = 0;
    for (const auto& update : updates) {
	auto& entry = update.prefix.route ? insertions[ii ++] : deletions[dd ++];
	entry.time = {.s = update.time >> 32, .ms = update.time & 0xffffffff, .us = 0, .od = 0};
	if (update.time == lastTimeSlot) {
	    entry.time.od = ++ order;
	} else {
	    lastTimeSlot = update.time;
	    order = 0;
	}
    
	auto prefix = MEM_ALLOC(1, pp::vip_prefix_t<T>);
	*prefix = update.prefix;
	entry.rule = (pp::rule_t)prefix;
    }

    return std::make_tuple(nInsertions, nDeletions);
}

namespace pp {

std::tuple<int, int>
loadUpdatesFromFile(const char* path, update_t*& insertions, update_t*& deletions, int type, int n)
{
  SAFE_OPEN_FILE(file, path);

  switch(type) {
  case DataType::MULTI_FIELD: return loadMultiFieldUpdates(file, insertions, deletions);
  case DataType::FIVE_TUPLE:  return loadClassBenchUpdates(file, insertions, deletions);
  case DataType::IPV4: fclose(file); return loadIpUpdates<ipv4_addr>(path, insertions, deletions, n);
  case DataType::IPV6: fclose(file); return loadIpUpdates<ipv6_addr>(path, insertions, deletions, n);
  case DataType::VIP4: fclose(file); return loadVipUpdates<ipv4_addr>(path, insertions, deletions);
  default : fclose(file); LOG_ERROR("INVALID DataType in loading updates\n");
  }

  return std::make_tuple(0, 0);
}

void
freeUpdates(int nUpdates, update_t*& updates, int type)
{
  if (!updates) return;
  for (int i = 0; i < nUpdates; ++i) {
    freeRule(updates[i].rule, type);
  }
  MEM_FREE(updates);
}

} // namespace pp
