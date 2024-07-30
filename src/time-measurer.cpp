#include <time-measurer.hpp>
#include <logger.hpp>

namespace pp {

TimeMeasurer::TimeMeasurer()
{
  start();
}
  
void
TimeMeasurer::start()
{
#ifdef __linux__
  clock_gettime(CLOCK_REALTIME ,&m_startStamp);
#else
  m_startStamp = std::chrono::high_resolution_clock::now();
#endif
}

uint64_t
TimeMeasurer::tick()
{
#ifdef __linux__
  clock_gettime(CLOCK_REALTIME ,&m_stopStamp);
  return ((uint64_t)m_stopStamp.tv_sec*1000000000 + m_stopStamp.tv_nsec) - 
    ((uint64_t)m_startStamp.tv_sec*1000000000 + m_startStamp.tv_nsec);//ns
#else
  using namespace std::chrono;
  m_stopStamp = high_resolution_clock::now();
  return duration_cast<nanoseconds>(m_stopStamp - m_startStamp).count();
#endif
}

uint64_t
TimeMeasurer::stop(int TU)
{
  auto duration = tick();
  switch(TU) {
  case TimeUnit::sec: return duration / 1000000000;
  case TimeUnit::ms:  return duration / 1000000;
  case TimeUnit::us:  return duration / 1000;
  }
  
  return duration; // default
}

uint64_t
TimeMeasurer::restart(int TU)
{
  auto res = stop(TU);
  m_startStamp = m_stopStamp;
  return res;
}
  
void
TimeMeasurer::scheduler(uint64_t wait, std::function<void(void)> callback, int TU)
{
  uint64_t curWait = 0;

  start();
  while(curWait < wait) {
    curWait = stop(TU);
  }

  callback();
}

void
TimeMeasurer::TEST()
{
  TimeMeasurer tm;
  LOG_INFO("duration: %llu s, %llu ms, %llu us, %llu ns\n",
	   tm.stop(TimeUnit::sec), tm.stop(TimeUnit::ms), tm.stop(TimeUnit::us), tm.tick());

  tm.scheduler(2, [] { LOG_INFO("time is up ...\n"); });
}

} // namespace pp
