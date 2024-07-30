#ifndef UPDATE_MANAGER_HPP
#define UPDATE_MANAGER_HPP

#include <cinttypes>
#include <processing-unit.hpp>

namespace pp {

struct UpdateStatistic
{
  uint64_t nInsertions;
  uint64_t nDeletions;
  double insRate;
  double delRate;

  UpdateStatistic()
    : nInsertions(0)
    , nDeletions(0)
    , insRate(0)
    , delRate(0)
  {}
};

struct UpdateManager
{
  void (*start)();
  void (*stop)();
  
  /** 
   * pull at most @max_num update requests
   * 
   * @param updates
   * @param max_num 
   * 
   * @return the actual number of pending update requests
   */
  update_t* (*pullUpdates)(int& nUpates, int max_num);

  UpdateStatistic (*getStatistic)();
};

} // namespace pp

#endif // UPDATE_MANAGER_HPP
