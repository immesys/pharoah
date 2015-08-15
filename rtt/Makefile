#the objective of this makefile is to compile using g++ specs
NEWLIB =
CC       = arm-none-eabi-gcc
AR			 = arm-none-eabi-ar
CFLAGS   = -Wall -mfloat-abi=soft -mthumb -mcpu=cortex-m4

all: clean SEGGER_RTT_printf.o SEGGER_RTT.o lib

lib: SEGGER_RTT.o SEGGER_RTT_printf.o
	$(AR) rvs librtt.a $^

SEGGER_RTT_printf.o: SEGGER_RTT_printf.c
	$(CC) -c $(CFLAGS) $<

SEGGER_RTT.o: SEGGER_RTT.c
	$(CC) -c $(CFLAGS) $<

.PHONY: clean

clean:
	rm -f *.o librtt.a
