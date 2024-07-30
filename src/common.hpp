#ifndef COMMON_HPP
#define COMMON_HPP

#include <profiler-flag.hpp>
#include <logger.hpp>
#include <time-measurer.hpp>
#include <rule-traffic.hpp>
#include <memory-manager.hpp>
#include <key-value.hpp>

#include <cassert>
#include <functional>
using namespace std::placeholders;

extern volatile bool globalStopRequested;
extern int globalNumFields;
extern bool globalDebugFlag;

#endif
