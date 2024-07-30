#ifndef FIB_UPDATE_MANAGER_HPP
#define FIB_UPDATE_MANAGER_HPP

#include <update-manager.hpp>
#include <string>

namespace pp {

struct FibUpdateManagerParameters
{
  int data_type;
  std::string update_file;
  double average_rate; // kups
  int max_updates;

  FibUpdateManagerParameters()
    : average_rate(0)
    , max_updates(0)
  {}
};

namespace fib {
  
void
start();

void
stop();
  
update_t* pullUpdates(int& nUpates, int max_num);

UpdateStatistic
getStatistic();
} // namespace dum

UpdateManager*
getFibUpdateManager(FibUpdateManagerParameters params);

} // namespace pp

#endif // FIB_UPDATE_MANAGER_HPP
