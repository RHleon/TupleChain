#ifdef __linux__
#include <thread>

namespace nm{

/**
 * init numa architechture cpu layout
*/
int init_numa_cpu_env();

/**
 * migrate target thread to a cpu which is in the same socket node as process who create this thread 
*/
int thread_migrate(std::thread & m_thread);

}

#endif