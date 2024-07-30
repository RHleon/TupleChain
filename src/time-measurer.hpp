#ifndef TIME_MEASURER_HPP
#define TIME_MEASURER_HPP

#include <functional>
#include <inttypes.h>

#ifdef __linux__ 
#include <time.h> //time measure in ns level
#else // for all others
#include <chrono>
#endif

namespace pp {

enum TimeUnit {
  sec = 0,
  ms,
  us,
  ns
};

class TimeMeasurer
{
public:
  static void TEST();
  
public:
  TimeMeasurer();

  /** 
   * start a new measurer, typically reset the time counter
   * 
   */
  void start();

  /** 
   * @return tick in ns
   */
  uint64_t
  tick();

  /** 
   * stop the latest measurer
   * 
   * @return the elapsed time with the specified unit ({sec, ms, us, ns})
   */
  uint64_t
  stop(int TU = TimeUnit::ns);

  /** 
   * restart the measurer
   * 
   * @return the elapsed time of the last measurer
   */
  uint64_t
  restart(int TU = TimeUnit::ns);

  /** 
   * schedule an event after @wait time units
   * 
   * @param wait the number of time units ({sec, ms, us, ns}) to wait
   * @param callback the function to invoke when the scheduled event happens 
   */
  void scheduler(uint64_t wait, std::function<void(void)> callback, int TU = TimeUnit::ns);

private:
#ifdef __linux__ 
  struct timespec m_startStamp;
  struct timespec m_stopStamp;
#else // for all others
  std::chrono::high_resolution_clock::time_point m_startStamp;
  std::chrono::high_resolution_clock::time_point m_stopStamp;
#endif
};

} // namespace pp

#endif // TIME_MEASURER_HPP
