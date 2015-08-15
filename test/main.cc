#include <stdio.h>
#include "interface.h"
#include "SEGGER_RTT.h"
static void _Delay(int period) {
  uint32_t i = 100000*period;
  do { ; } while (i--);
}
extern "C" {
extern int _write(int fd, const void *buf, uint32_t count);
}
int main() {
  while(1) {
    SEGGER_RTT_printf(0, "Hello world\n");
    _write(0,"hi\n",3);
    printf("hello world");
    fflush(stdout);
    _Delay(100);
  }
}
