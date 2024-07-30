#ifndef MEMORY_MANAGER_HPP
#define MEMORY_MANAGER_HPP

#include <stdlib.h>

namespace pp {

#define MEM_ALLOC(n, t) (t *)malloc((n) * sizeof(t))
#define MEM_FREE(p) if(p) { free(p); p = NULL; }

} // namespace pp

#endif // MEMORY_MANAGER_HPP
