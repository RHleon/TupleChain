#include <dummy-update-manager.hpp>
#include <common.hpp>
#include <pp-thread.hpp>
#include <vector>
#include <tuple>

namespace pp {
namespace dum {

static UpdateManager dummyUpdateManager = {
  .start        = dum::start,
  .stop         = dum::stop,
  .pullUpdates  = dum::pullUpdates,
  .getStatistic = dum::getStatistic
};

DummyUpdateManagerParameters configs;
UpdateStatistic statistics;
update_t* updatesBuffer = nullptr;
int bufferHead, bufferTail;
pp_thread_t updateEmulator;

void
emulatorLoop(pp_thread_t thisThread)
{
  int maxInsertions, maxDeletions;
  update_t* insertions = nullptr;
  update_t* deletions = nullptr;
  std::tie(maxInsertions, maxDeletions) =
    loadUpdatesFromFile(configs.update_file.c_str(), insertions, deletions, configs.data_type);
  assert(insertions != nullptr && deletions != nullptr);

  int nInsertions = 0, nDeletions = 0, nUpdates = 0;
  int bufferEnd = maxInsertions + maxDeletions;
  if (bufferEnd > configs.max_updates) bufferEnd = configs.max_updates;
  
  double insertPerNs = configs.insert_rate / 1000000.0f; // from KUPS to GUPS
  double deletePerNs = configs.delete_rate / 1000000.0f; // from KUPS to GUPS
  uint64_t tickCount = 0;
  
  TimeMeasurer measurer;
  while (thisThread->running && bufferTail < bufferEnd &&
	 nInsertions < maxInsertions && nDeletions < maxDeletions) {
    tickCount = measurer.tick();
    
    nUpdates = tickCount * insertPerNs;
    if (nUpdates > maxInsertions) nUpdates = maxInsertions;
    while (bufferTail < bufferEnd && nInsertions < nUpdates) {
      updatesBuffer[bufferTail ++] = insertions[nInsertions ++];
    }

    nUpdates = tickCount * deletePerNs;
    if (nUpdates > maxDeletions) nUpdates = maxDeletions;
    while (bufferTail < bufferEnd && nDeletions < nUpdates) {
      updatesBuffer[bufferTail ++] = deletions[nDeletions ++];
    }
  }

  globalStopRequested = true;

  while (thisThread->running);
  freeUpdates(maxInsertions, insertions);
  freeUpdates(maxDeletions, deletions);

  statistics.nInsertions = nInsertions;
  statistics.nDeletions = nDeletions;
  statistics.insRate = configs.insert_rate;
  statistics.delRate = configs.delete_rate;
  
  LOG_DEBUG("insertions: [%d/%d]; deletions: [%d/%d]; buffer: [%d %d %d]\n",
	    nInsertions, maxInsertions, nDeletions, maxDeletions,
	    bufferHead, bufferTail, bufferEnd);
}

void
start()
{
  updatesBuffer = MEM_ALLOC(configs.max_updates, update_t);
  LOG_ASSERT(updatesBuffer != nullptr, "allocate updates buffer");

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
  if (bufferHead >= bufferTail) return 0;

  nUpates = bufferTail - bufferHead;
  if (nUpates > max_num) nUpates = max_num;
  //LOG_INFO("%d: [%d %d]\n", num, bufferHead, bufferTail);

  auto res = updatesBuffer + bufferHead;
  bufferHead += nUpates;
  
  return res;
}

UpdateStatistic
getStatistic()
{
  return statistics;
}

} // namespace dum

UpdateManager*
getDummyUpdateManager(DummyUpdateManagerParameters params)
{
  dum::configs = params;
  return &dum::dummyUpdateManager;
}

} // namespace pp
