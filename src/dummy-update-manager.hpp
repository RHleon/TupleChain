#ifndef DUMMY_UPDATE_MANAGER_HPP
#define DUMMY_UPDATE_MANAGER_HPP

#include <update-manager.hpp>
#include <string>

namespace pp {

struct DummyUpdateManagerParameters
{
  int data_type;
  std::string update_file;
  double insert_rate; // kups
  double delete_rate; // kups
  int max_updates;

  DummyUpdateManagerParameters()
    : insert_rate(0)
    , delete_rate(0)
    , max_updates(0)
  {}
};

namespace dum {

void
start();

void
stop();
  
update_t* pullUpdates(int& nUpates, int max_num);

UpdateStatistic
getStatistic();

} // namespace dum

UpdateManager*
getDummyUpdateManager(DummyUpdateManagerParameters params);

} // namespace pp

#endif // DUMMY_UPDATE_MANAGER_HPP
