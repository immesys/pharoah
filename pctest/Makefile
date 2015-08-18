#the objective of this makefile is to compile using g++ specs
CC       = g++-4.9
CFLAGS   = -ffunction-sections -std=c++14 -fdata-sections -fno-strict-aliasing -g3 -Wall
LDFLAGS  = -ffunction-sections -std=c++14 -fdata-sections -fno-strict-aliasing -g3
LDFLAGS += -Wl,--gc-sections

all: clean tester

tester: main.o libstorm.o
	$(CC) -o $@ $^ $(LDFLAGS)

main.o: main.cc
	$(CC) -c $(CFLAGS) $<

libstorm.o: libstorm.cc libstorm.h
	$(CC) -c $(CFLAGS) $<

install:
	sload program tester

.PHONY: clean

clean:
	rm -f *.o tester
