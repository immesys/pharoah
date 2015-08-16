#include <stdio.h>
#include "interface.h"
#include <functional>
#include "libstorm.h"
#include "SEGGER_RTT.h"

using namespace storm;

int main() {
  tq::add([]{
    printf("hello world (from tq lambda)\n");
  });
  int count = 0;
  std::shared_ptr<storm::Timer> t = Timer::periodic(3*Timer::SECOND, [&]{
    printf("count is %d\n", count);
    count++;
    if (count == 10) {
      t->cancel();
    }
  });
  tq::scheduler();
}
