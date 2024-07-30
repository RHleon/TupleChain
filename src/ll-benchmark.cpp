#include <ll-benchmark.hpp>
#include <common.hpp>
#include <deque>
#include <tss.hpp>

namespace pp {

void TEST_IO(struct pkt_io_t* io)
{
    io_params_t params = {
	.pkt_file = "test-data/2_1000_match.traffic",
	.data_type = DataType::MULTI_FIELD,
	.tx_rate = 1,
	.test_seconds = 2
    };
    io->start(params);

    packet_t* pkts;
    int nPkts = 0;
    int rcvPkts = 0;
    int sntPkts = 0;
  
    TimeMeasurer measurer;
    while (!globalStopRequested) {
	pkts = io->pullPackets(nPkts, 256);
	rcvPkts += nPkts;
	sntPkts += io->pushPackets(pkts, NULL, nPkts);
    }

    auto statistic = io->stop();
    LOG_INFO("emulator run time: %d seconds, tx_rate: %.2f MPPS; rx_rate: %.2f MPPS\n",
	     statistic.testTime, statistic.txRate, statistic.rxRate);
}

void TEST_CLASSFIER(struct pc_unit_t* pu, const std::string& test_case)
{
    rule_t* rules;
    packet_t* packets;
    action_t* results;

    std::string rulePath = std::string("test-data/") + test_case + ".rule";
    std::string pktPath  = std::string("test-data/") + test_case + "_match.traffic";
    std::string resPath  = pktPath + ".result";

    int nRules   = loadRuleFromFile(rulePath.c_str(), rules);
    int nPackets = loadTrafficFromFile(pktPath.c_str(), packets);
    int nResults = loadResultFromFile(resPath.c_str(), results);
    LOG_ASSERT(nPackets == nResults, "invalid # of results");
  
    algorithm_args_t args;
    globalNumFields = args.num_fields = 2;

    auto metric = pu->construct(args, nRules, (pc_rule_t*)rules);
    LOG_INFO("consumed %d bytes\n", metric.nBytes);

    LOG_INFO("to match %d packets: ", nPackets);
    auto res = MEM_ALLOC(nPackets, action_t);

    uint32_t** fields = MEM_ALLOC(nPackets, uint32_t*);
    for (int i = 0; i < nPackets; ++i) fields[i] = (uint32_t*)packets[i];
    
    TimeMeasurer measurer;
    measurer.start();
    for (int i = 0; i < nPackets; ++i) {
	res[i] = pu->matchPacket(fields[i]);
    }
    auto tot = measurer.stop(TimeUnit::us);
    LOG_INFO("%llu us, %.2f us, %.2f MPPS\n", tot, tot * 1.0f / nPackets, nPackets * 1.0f / tot);

    for (int i = 0; i < nPackets; ++ i) {
	LOG_ASSERT(res[i] == results[i], "ERROR at %d, %d != %d", i, res[i], results[i]);
    }

    MEM_FREE(res);
    MEM_FREE(fields);
    freePackets(nPackets, packets);
  
    pu->release();
}

std::deque<std::string>
parseLine(const char* line)
{
    std::string input(line);
    std::deque<std::string> res;
    size_t pos = 0;
    while ((pos = input.find(" ")) != std::string::npos) {
	res.push_back(input.substr(0, pos));
	input = input.substr(pos + 1);
    }

    if ((pos = input.find_first_of("\n\r")) != std::string::npos) {
	input = input.substr(0, pos);
    }
    
    res.push_back(input);
    return res;
}

void runTest(std::string params)
{
    std::string key = LLConfigFile::parseLine(params);
    if (key == "io") {
	TEST_IO(selectIo(params));
    }
    else {
	auto pu = selectClassifier(key);
	if (pu) {
	    TEST_CLASSFIER(pu, params);
	}
	else {
	    LOG_ERROR("invialid test key : %s\n", key.c_str());
	}
    }
}

void runClass(std::string params, const char* cfg_file)
{
    std::string algo = LLConfigFile::parseLine(params);
    LOG_INFO("run classbench with @%s@ and %s\n", algo.c_str(), cfg_file);
    
    LLBenchMark bm(cfg_file, algo);
    bm.runClassBench(params);
}
	
void runLookup(const std::string& algo, const char* cfg_file)
{
    LOG_INFO("run lookup with @%s@ and %s\n", algo.c_str(), cfg_file);
    LLBenchMark bm(cfg_file, algo);

    bm.runConstruction();
    bm.runPCSimulation();
}

memory_metric_t
LLBenchMark::runConstruction()
{
    auto metric = m_classifier->construct(m_args, m_data.nRules, (pc_rule_t*)m_data.rules);
    LOG_INFO("consumed %d bytes (%.2f MBytes)\n", metric.nBytes, metric.nBytes / 1048576.0f);
    return metric;
}
    
lookup_metric_t
LLBenchMark::runPCSimulation()
{
    struct test_case_t {
	uint32_t* fields;
	action_t  action;
	test_case_t(packet_t pkt) {
	    fields = (uint32_t*)pkt;
	    action = 0;
	}
    };
    std::vector<test_case_t> tests;

    for (int i = 0; i < m_data.nPackets; ++ i) {
	tests.emplace_back(m_data.packets[i]);
    }
    auto matchFunc = m_classifier->matchPacket;
    
    TimeMeasurer measurer;
    measurer.start();
    for (auto& test : tests) {
	test.action = matchFunc(test.fields);
    }
    auto tot = measurer.stop(TimeUnit::us);

    for (int i = 0; i < m_data.nPackets; ++ i) {
	if (tests[i].action != m_data.actions[i]) {
	    LOG_INFO("-------------------------------------\n");
	    matchFunc(tests[i].fields);

		//print error match result
	    LOG_INFO("invalid result caused by %.2dth pkt: res=%u wanted=%u\n", i, tests[i].action, m_data.actions[i]);

		//print packet content causing error
		LOG_INFO("Pkt:");
		for (int j = 0; j < globalNumFields	; ++j){
			LOG_INFO("0x%x ", tests[i].fields[j]);
		}
		LOG_INFO("\n");
		LOG_ERROR("->Match Error\n");
	}
    }

    LOG_INFO("matched %6d packets, %2.1f us on average : %.2f MPPS\n",
	     m_data.nPackets, tot * 1.0f / m_data.nPackets, m_data.nPackets * 1.0f / tot);

    lookup_metric_t res;
    res.latency = tot * 1.0f / m_data.nPackets;
    res.throughput = m_data.nPackets * 1.0f / tot;

    return res;
}

void
LLBenchMark::runClassBench(const std::string& output)
{
    FILE* res;
    if (output.empty()) {
	LOG_INFO("open default output file : test.res\n");
	res = fopen("test.res", "a");
    }
    else {
	LOG_INFO("output to file %s\n", output.c_str());
	res = fopen(output.c_str(), "a");
    }

    LOG_ASSERT(res != NULL, "open file error\n");

    auto mem_res = runConstruction();
    auto lkp_res = runPCSimulation();

    fprintf(res, "%4.2f MB %2.1f us %5.2f MPPS\n",
	    mem_res.nBytes / 1048576.0f,
	    lkp_res.latency, lkp_res.throughput);
	
    fclose(res);
}
    
LLBenchMark::LLBenchMark(const char* file_name,
			 const std::string& algo,
			 const std::string& io)
    : m_io(selectIo(io))
    , m_classifier(selectClassifier(algo))
{
    LLConfigFile config(file_name);
    if (m_io) m_params = m_io->parseConfigFile(config);
    if (m_classifier) m_args = m_classifier->parseConfigFile(config);
    prepareTestData(config);
}

LLBenchMark::~LLBenchMark()
{
    if (m_classifier) m_classifier->release();
}

void
LLBenchMark::prepareTestData(LLConfigFile config)
{
    globalNumFields = 2;

    config.addHandler("num_field", [] (std::string params) {
	    globalNumFields = atoi(params.c_str());
	});
    config.addHandler("rule", [this] (std::string params) {
	    m_data.rule_file = params;
	    m_data.nRules = loadRuleFromFile(params.c_str(), m_data.rules);
	});
    config.addHandler("traffic", [this] (std::string params) {

	    m_data.pkt_file = params;
	    m_data.nPackets = loadTrafficFromFile(params.c_str(), m_data.packets);
	    auto result = params + ".result";
	    assert(loadResultFromFile(result.c_str(), m_data.actions) == m_data.nPackets);
	});
    config.addHandler("update", [this] (std::string params) {
	    std::tie(m_data.nInsertions, m_data.nDeletions)
		= loadUpdatesFromFile(params.c_str(), m_data.insertions, m_data.deletions,
				      DataType::MULTI_FIELD, m_data.nRules);
#ifdef PROFILER_ENABLED
	    auto tr = params + ".traffic";
	    auto res = tr + ".result";

	    m_data.nUpdatePackets = loadTrafficFromFile(tr.c_str(), m_data.updatePackets);
	    assert(loadResultFromFile(res.c_str(), m_data.updateActions) == m_data.nUpdatePackets);
#endif
	});

    config.parse("test-data");
}
    
void
LLBenchMark::run(const char* cfg_file)
{
    LOG_INFO("run benchmark with config file %s!\n", cfg_file);
    LLConfigFile config(cfg_file);

    config.addHandler("test", std::bind(runTest, _1));
    config.addHandler("lookup", std::bind(runLookup, _1, cfg_file));
    config.addHandler("class", std::bind(runClass, _1, cfg_file));

    config.parse("cmd");
}


    
} // namespace pp
