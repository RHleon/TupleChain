#ifndef PP_THREAD
#define PP_THREAD

#include <thread>
#include <functional>
#include <list>

namespace pp {

struct ThreadArgs
{
  std::thread thread;
  volatile bool running;
  ThreadArgs() : running(true) {};
};

typedef std::list<ThreadArgs>::iterator pp_thread_t;
typedef std::function<void(pp_thread_t)> pp_thread_func;
  
namespace MultiThreading {

pp_thread_t
launchThread(pp_thread_func func);

pp_thread_t
terminateThread(pp_thread_t target);

bool
isValid(pp_thread_t target);
  
} // namespace MultiThreading
} // namespace pp

#endif // PP_THREAD
