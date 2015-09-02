#include <stdio.h>
#include "interface.h"
#include <functional>
#include "libstorm.h"
#include "SEGGER_RTT.h"
#include <malloc.h>

using namespace storm;

int main()
{
  printf("I am here in the payload\n");
//  auto buf = mkbuf(100);
  int iteration = 0;
  auto buf = mkbuf({0x02, 0b01110100});
  i2c::write(i2c::TMP006, i2c::START | i2c::STOP, std::move(buf), 2, [&iteration](int status, buf_t buf)
  {
    Timer::periodic(1*Timer::SECOND, [&iteration](){
      printf("tick:\n");
      malloc_stats();
      auto buf = mkbuf(2);
      buf[0] = 0x1;
      i2c::write(i2c::TMP006, i2c::START, std::move(buf), 1, [&iteration](int status, buf_t buf){
        printf("got cb\n");
        if (status != i2c::OK)
        {
          printf("NOT OK\n");
          return;
        }
        buf[0] = 0x55; buf[1] = 0x55;
        i2c::read(i2c::TMP006, i2c::RSTART | i2c::STOP, std::move(buf), 2, [&iteration](int status, buf_t payload){
          printf("Got result: %d %x %x %x\n", status, payload[0], payload[1], payload[2]);
          uint16_t temp = ((uint16_t)payload[0] << 8) + payload[1];
          temp >>= 2;
          double rtemp = (double)temp * 0.03125;
          printf("iteration %d\n", iteration);
          iteration++;
          printf("got temperature: %d.%03d C \n", (int) rtemp, (int)(((rtemp-((int)rtemp))*1000)));
        });
      });
    });
  });

  tq::scheduler();
}
