#include <stdio.h>
#include <functional>
#include "libstorm.h"
#include <malloc.h>
using namespace storm;

using namespace storm::future;

int main() {

  auto f = unbound<int, int>();
  f->then([](accept<int> A, reject<> R, int x, int y)
    {
      printf("got %d %d\n", x, y);
      return R();
      return A(x+y);
    }
  )->then([](accept<std::string> A, reject<> R, int x)
    {
      A("hello world");
    }
  )->then([](accept<> A, reject<std::string> R, std::string s)
    {
      printf("got s=%s\n", s.c_str());
      return R("sorry, you got rejected");
    }
  )->then([](accept<> A, reject<> R)
    {
      printf("I won't get run because previous rejected\n");
    }
  )->except([]{
    printf("got an exception\n");
  });
  f->run(2, 5);

}
