#include <stdio.h>
#include <functional>
#include "libstorm.h"
#include <malloc.h>
using namespace storm;

using namespace storm::future;
#if 0
future::Future<future::in<>, future::out<int>> dummycall()
{
  return future::value
}
#endif
int main() {

  auto f = unbound<int, int>();
  f->then([](accept<int> A, reject<> R, int x, int y)
  {
    printf("got %d %d\n", x, y);
    return A(x+y);
  }
  )->then([](accept<std::string> A, reject<> R, int x)
  {
    return A("hello world");
  }
)->then([](accept<> A, reject<std::string> R, std::string s)
  {
    printf("got s=%s\n", s.c_str());
    return R("sorry, you got rejected");
  }
  )->then([](accept<> A, reject<> R)
  {
    printf("I won't get run because previous rejected\n");
  },
  [](std::string msg)
  {
    printf("got rejection message: %s\n", msg.c_str());
  }
  );
  f->run(2, 5);

#if 0
  //auto f = future::bound(5, 6, 7);
  auto f = future::unbound<int, int, int>();
/*
  f.then([](future::accept<int, int> aa, future::reject<int> bb, int a, int b, int c)
  {
    //printf("got %d %d %d\n", a, b, c);
  });*/
  auto l = [](future::accept<int, int> aa, future::reject<int> bb, int a, int b, int c)
  {
    //printf("got %d %d %d\n", a, b, c);
  };
  f->then(l);
  f->run(5,6,7);
  /*
  f.run(5,6,7);*/
#endif




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
}
