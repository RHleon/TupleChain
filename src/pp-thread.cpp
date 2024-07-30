#include <pp-thread.hpp>
#include <numa-arch-helper.hpp>

namespace pp {
namespace MultiThreading {

static std::list<ThreadArgs> m_threads;

pp_thread_t
launchThread(pp_thread_func func)
{
  auto it = m_threads.insert(m_threads.end(), ThreadArgs());
  it->thread = std::thread(func, it);

#ifdef __linux__
  nm::thread_migrate(it->thread);
#endif

  return it;
}

pp_thread_t
terminateThread(pp_thread_t target)
{
  if (target != m_threads.end()) {
    target->running = false;
    target->thread.join();
    m_threads.erase(target);
  }

  return m_threads.end();
}

bool
isValid(pp_thread_t target)
{
  return target != m_threads.end();
}

} // namespace MultiThreading
} // namespace pp
