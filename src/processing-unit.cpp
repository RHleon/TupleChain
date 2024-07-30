#include <processing-unit.hpp>
#include <common.hpp>

namespace pp {

int
ProcessingUnit::generateMatchResult(const char* rule_path, const char* traffic_path,
				    ProcessingUnit* pu)
{
  LOG_ASSERT(pu, "invalid pu");
  
  auto readNumFields = [] (const char* path) -> int {
    assert(path);
    FILE* file = fopen(path, "r");				
    if (!file) LOG_ERROR("can not open file: %s\n", path);

    int nFields;
    fscanf(file, "%d", &nFields);
    fclose(file);
    
    return nFields;
  };

  LOG_INFO("generate result for %s with %s using %s\n",
	   traffic_path, pu->getName().c_str(), rule_path);
  
  std::string result_path = std::string(traffic_path) + ".result";
  FILE* file = fopen(result_path.c_str(), "w");
  if (!file) LOG_ERROR("can not open file to write result: %s\n", result_path.c_str());
  
  pp::rule_t* rules = nullptr;
  pp::packet_t* packets = nullptr;
  int nFields = readNumFields(rule_path);
  assert(nFields == readNumFields(traffic_path));

  int nRules = pp::loadRuleFromFile(rule_path, rules, pp::DataType::MULTI_FIELD);
  int nPackets = pp::loadTrafficFromFile(traffic_path, packets, pp::DataType::MULTI_FIELD);
  assert(rules != nullptr && packets != nullptr);

  pu->initializeMatchProfiler(nPackets);
  pu->constructWithRules(nRules, rules);

  fprintf(file, "%d\n", nPackets);
  for (int i = 0; i < nPackets; ++ i) {
    fprintf(file, "%u\n", pu->matchPacket(packets[i]));
  }
  fclose(file);

  delete pu;
  
  return nPackets;
}

} // namespace pp
