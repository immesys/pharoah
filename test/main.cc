#include <stdio.h>
#include "interface.h"
#include <functional>
#include "libstorm.h"
#include "SEGGER_RTT.h"
#include <malloc.h>
using namespace storm;

#if 0
future::Future<future::in<>, future::out<int>> dummycall()
{
  return future::value
}
#endif
int main() {


  //auto f = future::bound(5, 6, 7);
  auto f = future::unbound<int, int, int>();
  f.then([](future::accept<int, int> aa, future::reject<int> bb, int a, int b, int c)
  {
    printf("got %d %d %d\n", a, b, c);
  });
  f.run(5,6,7);



  //auto f2 = future::Future().then([])


/*  f.then([](auto aa, auto bb, int a, int b, int c)
  {
    printf("got %d %d %d\n", a, b, c);
  });*/

  /*auto f2 = future::resolve([](future::accept<> aa, future::reject<> bb, int a)
  {

  });*/
#if 0
  auto f2 = future::Future/*<future::in<int,int,int>, future::out<int>, future::err<int>>*/
    ::wrap([](future::accept<int> aa, future::reject<int> bb, int a, int b, int c)
  {
    printf("got %d %d %d\n", a, b, c);
    aa(5);
    bb(7);
  });
  f2.run(2,3,4);*/
  #endif
  //future::wrap()
  /*
  auto f2 = future::Future<>
  <future::in<int,int,int>, future::out<int>, future::err<int>>
  ([](future::accept<int> aa, future::reject<int> bb, int a, int b, int c)
  {
    printf("got %d %d %d\n", a, b, c);
    aa(5);
    bb(7);
  });
  f2.run(2,3,4);*/
  #if 0
  future::loop
  (
      future::exec([]{
        printf("line 1\n");
      })
      ->delay(500*MILLISECOND)
      ->then([]{
        printf("line 2\n");
        return 5;
      })
      ->then([](auto i){
        printf("line 3 %d\n", i);
      })
  );
  #endif
  #if 0
  fprintf(stderr, "is stderr working\n");
  gpio::set_mode(gpio::GP0, gpio::OUT);
  auto sock = UDPSocket::open(343, [](std::shared_ptr<UDPSocket::Packet>){
    printf("got packet\n");
    gpio::set(gpio::GP0, gpio::TOGGLE);
  });
  Timer::periodic(300*Timer::MILLISECOND, [=]{
    sock->sendto("ff02::1", 343, "hello world ~ a reasonable long packet");
  });
  Timer::periodic(2*Timer::SECOND, []{
    malloc_stats();
  });
  #endif
  tq::scheduler();
}
