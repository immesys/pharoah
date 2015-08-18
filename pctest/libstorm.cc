
#include "libstorm.h"
#include <array>
#include <queue>
#include <cstring>

namespace storm
{
  namespace tq
  {
    void Task::fire()
    {
      (*target)();
    }
    std::queue<Task> dyn_tq;
    template <> bool add(std::shared_ptr<std::function<void(void)>> target)
    {
      dyn_tq.push(Task(target));
      return true;
    }
    bool run_one()
    {
      if (dyn_tq.empty())
      {
        return false;
      }
      dyn_tq.front().fire();
      dyn_tq.pop();
      return true;
    }
    void __attribute__((noreturn)) scheduler()
    {
      while(1)
      {
        while(run_one());
      }
    }
  }
}
