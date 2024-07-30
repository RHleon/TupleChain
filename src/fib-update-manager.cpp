#include <fib-update-manager.hpp>
#include <common.hpp>
#include <pp-thread.hpp>
#include <vector>
#include <tuple>
#include <ip-lookup-interface.hpp>

namespace pp {
namespace fib {

static UpdateManager manager = {
  .start        = start,
  .stop         = stop,
  .pullUpdates  = pullUpdates,
  .getStatistic = getStatistic
};

FibUpdateManagerParameters configs;
UpdateStatistic statistics;
update_t* updatesBuffer = nullptr;
int bufferHead, bufferTail, totalUpdates;
pp_thread_t updateEmulator;

void
emulatorLoop(pp_thread_t thisThread)
{
    int maxInsertions, maxDeletions;
    update_t* insertions = nullptr;
    update_t* deletions = nullptr;
    std::tie(maxInsertions, maxDeletions) =
	loadUpdatesFromFile(configs.update_file.c_str(),
			    insertions, deletions, configs.data_type);
    assert(insertions != nullptr && deletions != nullptr);

    updatesBuffer = MEM_ALLOC(maxInsertions + maxDeletions, update_t);
    LOG_ASSERT(updatesBuffer != nullptr, "allocate updates buffer");

    
    auto isEarlier = [] (const update_time_t& x, const update_time_t& y) ->bool {
	if (x.s != y.s) return x.s < y.s;
	if (x.ms != y.ms) return x.ms < y.ms;
	if (x.us != y.us) return x.us < y.us;
	return x.od < y.od;
    };
  
    int nInsertions = 0, nDeletions = 0, nUpdates = 0;
    while (nInsertions < maxInsertions && nDeletions < maxDeletions) {
	updatesBuffer[nUpdates ++] =
	    isEarlier(insertions[nInsertions].time, deletions[nDeletions].time) ?
	    insertions[nInsertions ++] : deletions[nDeletions ++];
    }
    while (nInsertions < maxInsertions) {
	updatesBuffer[nUpdates ++] = insertions[nInsertions ++];
    }
    while (nDeletions < maxDeletions) {
        updatesBuffer[nUpdates ++] = deletions[nDeletions ++];
    }
    LOG_ASSERT(nUpdates == maxInsertions + maxDeletions, "invalid nUpdates");
  

    totalUpdates = maxInsertions + maxDeletions;
    
    uint32_t temp = (uint32_t)1;
    uint32_t tot_update_mask = (uint32_t)1;

    while( (temp-1) <= totalUpdates){
        tot_update_mask = temp;
        temp <<= 1;
    }
    LOG_INFO("[Update info: tot=%d, mask=0x%x]\n", totalUpdates, tot_update_mask);

    int nProcessedUpdates = 0;
    double updatesPerNs = configs.average_rate / 1000000.0f; // from KUPS to GUPS
    uint64_t tickCount = 0;

    TimeMeasurer measurer;
    while (thisThread->running) {
	tickCount = measurer.tick();
    
	nUpdates = tickCount * updatesPerNs;
	if (nUpdates > nProcessedUpdates) {
	    nProcessedUpdates = nUpdates;
	    //bufferTail = nProcessedUpdates % totalUpdates;
	    bufferTail = nProcessedUpdates & tot_update_mask;
	}
    }

  globalStopRequested = true;

  while (thisThread->running);
  freeUpdates(maxInsertions, insertions, configs.data_type);
  freeUpdates(maxDeletions, deletions, configs.data_type);

  statistics.nInsertions = nInsertions;
  statistics.nDeletions = nDeletions;
  statistics.insRate = configs.average_rate;
  statistics.delRate = configs.average_rate;
  
  LOG_DEBUG("insertions: [%d/%d]; deletions: [%d/%d]; buffer: [%d %d %d]\n",
	    nInsertions, maxInsertions, nDeletions, maxDeletions,
	    bufferHead, bufferTail, totalUpdates);
}

void
start()
{
  statistics = UpdateStatistic();
  
  bufferHead = bufferTail = 0;
  updateEmulator = MultiThreading::launchThread(emulatorLoop);
}

void
stop()
{
  updateEmulator = MultiThreading::terminateThread(updateEmulator);
  MEM_FREE(updatesBuffer);
}
  
update_t* pullUpdates(int& nUpates, int max_num)
{
    int tail = bufferTail;
    auto res = updatesBuffer + bufferHead;

    if (tail < bufferHead) {
	nUpates = totalUpdates - bufferHead;
    }
    else {
	nUpates = tail - bufferHead;
    }

    if (nUpates > max_num) max_num = nUpates;
    bufferHead += nUpates;
    if (bufferHead == totalUpdates) bufferHead = 0;
  
    return res;
}

UpdateStatistic
getStatistic()
{
  return statistics;
}

} // namespace fib

UpdateManager*
getFibUpdateManager(FibUpdateManagerParameters params)
{
  fib::configs = params;
  return &fib::manager;
}

} // namespace pp
