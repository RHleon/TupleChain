#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <stdio.h>
#include <cassert>

#define MAX_LOGGER_STR_LEN 16384

#define LOG_RED    "\033[0;31m"        /* 0 -> normal ;  31 -> red */
#define LOG_CYAN   "\033[0;36m"        /* 1 -> bold ;  36 -> cyan */
#define LOG_GREEN  "\033[0;32m"        /* 4 -> underline ;  32 -> green */
#define LOG_BLUE   "\033[0;34m"        /* 9 -> strike ;  34 -> blue */
 
#define LOG_BLACK   "\033[0;30m"
#define LOG_BROWN   "\033[0;33m"
#define LOG_MAGENTA "\033[0;35m"
#define LOG_GRAY    "\033[0;37m"
 
#define LOG_NONE    "\033[0m"        /* to flush the previous property */

#define _DEBUG_

#define LOG_INFO(format, ...) fprintf(stderr, format, ##__VA_ARGS__)

#ifdef _DEBUG_
#define LOG_ERROR(format, ...) {\
    fprintf(stderr, "ERROR: " format "\n", ##__VA_ARGS__);\
    assert(false);						  \
  }
#else
#define LOG_ERROR(format, ...) {}
#endif

#ifdef _DEBUG_
#define LOG_DEBUG(format, ...) LOG_INFO(format, ##__VA_ARGS__)
#else
#define LOG_DEBUG(format, ...) {}
#endif

#define LOG_FILE(file, format, ...) {		\
    if(file) {					\
      fprintf(file, format, ##__VA_ARGS__);	\
    }						\
    else {					\
      LOG_INFO(format, ##__VA_ARGS__);		\
    }						\
  }

#define LOG_ASSERT(exp, format, ...) if (!(exp)) LOG_ERROR(format, ##__VA_ARGS__)

#define LOG_STEP(process, msg) {	     \
    LOG_DEBUG("%s ... ", msg);		     \
    process;				     \
    LOG_DEBUG("... DONE\n");		     \
  }

#endif // LOGGER_HPP
