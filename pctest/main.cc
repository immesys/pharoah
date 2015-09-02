#include <stdio.h>
#include <functional>
#include "libstorm.h"
#include <malloc.h>
using namespace storm;

using namespace storm::future;

int main() {

  //Futures can be created by themselves using the
  //future::bound() or future::unbound() helpers.
  //Or they can be created by using the then() and except() methods
  //of an existing future object.

  //This creates a future that accepts two integers when it is resolved
  //or run().
  auto f0 = unbound<int, int>();
  //A bound future will have the values already specified.
  auto f1 = bound(2, 5);
  //You can create another future that takes in the values output
  //by the previous future using ::then(). The future can
  //have explicit accept() and reject() functions:
  auto f2 = f1->then([](accept<int> A, reject<std::string> R, int a, int b)
  {
    printf("got two numbers: %d %d\n", a, b);
    //Accept and pass on the returned value.
    A(a+b);
  });
  //You can also do accept via return for synchronous bodies by leaving
  //the accept out of the lambda signature
  auto f3 = f2->then([](reject<> R, int c)
  {
    return c + 1;
  });
  //You can also assume that the body never rejects and omit the reject parameter
  auto f4 = f3->then([](int d)
  {
    return d + 1;
  });
  f4->then([](int e){
    printf("got this: %d\n", e);
  });

  //The type signature of these futures is pretty hectic:
  std::shared_ptr<Future<in<int,int>,out<int>,err<std::string>,future::priv::ACC_REJ>> f2_ = f2;

  //You can add an "except" handler to catch any unhandled rejects earlier in
  //the chain. As the reject types may differ, this is really a fallback
  //error handler for when you don't care about the data in the rejection.
  //you should use the alternate form of then() that takes two lambdas if you
  //want to explicitly handle the rejection from the previous future with
  //the full type. (currently commented out)
  f4->except([]()
  {
    printf("Whoops got rejected\n");
  });

  //The chain is run by invoking run() on the first future in the chain
  f1->run();

  //This is all great, but I stopped working here because of the following
  //questions:
  //Q: How do I invoke an asynchronous function inside a promise?
  // option A: asynchronous function takes callback:
  //           pass in a lambda that invokes A() or R(). Sure but thats
  //           essentially just a callback system.
  // option B: function returns a promise. Ok but WHEN does it return the promise?
  //   option B.1: when you add it onto an existing future chain. This increases complexity
  //               because now a functon has three parts: setup which returns a future but
  //               doesn't DO anything (like starting operation) because the previous future
  //               has not resolved yet so its not allowed to do anything. Then the actual
  //               starting of the future body when the previous future finishes resolving
  //               then the resolving of the returned future when the operation completes.
  //  option B.2:  when you resolve the previous future. Ok, but now you need a proxy
  //               future to take the place of the promised future (haha) in the chain
  //               otherwise the types do not match up. This is not the end of the world.
  //
  // Q: How many closures are created, how much code emitted and how much memory used to
  //    do all this?
  // A: strictly greater than the purely callback method.
  //
  // Q: Is the resulting code neater or easier to write?
  // A: No. Especially considering that the callback method can be more easily hidden
  //    by a preprocessing step
  //
  // Q: Why am I doing this again?
  // A: People smarter than me think futures and promises are universally good.
  //
  // Q: Could they be wrong?
  // A: ...

  // TL;DR: futures/promises are completely possible in a static environment but
  //        not sure they are worth it.

}
