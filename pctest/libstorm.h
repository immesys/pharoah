#include <memory>
#include <functional>
#include <queue>
#include <stdio.h>
namespace storm
{
  namespace future
  {
    //TODO replace these with custom classes to prevent functions in the signature
    //from accidentally being recognised as accept/reject
    template<class ... Tz> using accept = std::function<void(Tz...)>;
    template<class ... Tz> using reject = std::function<void(Tz...)>;

    //Define some nicely named structs to use to pass type signatures into
    //future classes. We could have used std::tuple but this makes it easier
    //to read
    template<class ... Tz> struct in{};
    template<class ... Tz> struct out;
    template<class T, class ... Tz> struct out<T, Tz...>
    {
      typedef T first;
    };
    template<> struct out<>
    {
      typedef void first;
    };
    template<class ... Tz> struct err{};
    //To enable multiple return values
    template<class ... Tz> class Multi;

    namespace priv
    {
      enum FutureMode
      {
        ACC_REJ, //The lambda takes both accept and reject functions and returns void
        REJ_MULTI, //The lambda takes a reject function and returns a Multi
        REJ_SINGLE, //The lambda takes a reject function and returns a single/void
        MULTI, //The lambda takes no fun, and returns a Multi
        SINGLE, //The lambda takes no fun and returns a single/void
        BOUND, //The values were determined at construction time and are passed directly through
        UNBOUND, //The values are determined by run() and we pass them directly through
        EXCEPT, //We are an except promise, we don't pass forward
      };
    }

    namespace priv
    {
      //Used to differentiate otherwise identical functions that are
      //statically selected for inclusion
      typedef std::integral_constant<int, 0> t0;
      typedef std::integral_constant<int, 1> t1;
      typedef std::integral_constant<int, 2> t2;
      typedef std::integral_constant<int, 3> t3;
      typedef std::integral_constant<int, 4> t4;
      typedef std::integral_constant<int, 5> t5;

      //taken from http://stackoverflow.com/questions/7858817/
      //and http://stackoverflow.com/questions/15537817/
      template < ::std::size_t... Indices>
      struct indices {};
      template < ::std::size_t N, ::std::size_t... Is>
      struct build_indices : build_indices<N-1, N-1, Is...> {};
      template < ::std::size_t... Is>
      struct build_indices<0, Is...> : indices<Is...> {};
      template <typename T, bool matchingLambda=false>
      struct lambda_traits
          : public lambda_traits<decltype(&T::operator())>
      {
      };
      template <typename ClassType, typename ...AcceptSig, typename ...RejectSig, typename... Args>
      struct lambda_traits<void(ClassType::*)(accept<AcceptSig...>, reject<RejectSig...>, Args...) const>
      {
          typedef in<Args...> args_type;
          typedef out<AcceptSig...> accept_type;
          typedef err<RejectSig...> reject_type;

          static constexpr FutureMode mode = ACC_REJ;

          //N/A for this mode
          typedef std::false_type ismultiple;
          typedef std::false_type isvoid;
      };
      template <typename ClassType, typename ...AcceptSig, typename ...RejectSig, typename... Args>
      struct lambda_traits<Multi<AcceptSig...>(ClassType::*)(reject<RejectSig...>, Args...) const>
      // we specialize for pointers to member function
      {
          typedef in<Args...> args_type;
          typedef out<AcceptSig...> accept_type;
          typedef err<RejectSig...> reject_type;

          static constexpr FutureMode mode = REJ_MULTI;

          typedef std::true_type ismultiple;
          typedef std::false_type isvoid;
      };
      template <typename ClassType, typename AcceptSig, typename ...RejectSig, typename... Args>
      struct lambda_traits<AcceptSig(ClassType::*)(reject<RejectSig...>, Args...) const>
      // we specialize for pointers to member function
      {
          typedef typename std::is_same<void, AcceptSig> isvoid;

          typedef in<Args...> args_type;
          typedef typename std::conditional<isvoid::value, out<>, out<AcceptSig>>::type accept_type;
          typedef err<RejectSig...> reject_type;

          static constexpr FutureMode mode = REJ_SINGLE;

          typedef std::false_type ismultiple;

      };
      template <typename ClassType, typename ...AcceptSig, typename... Args>
      struct lambda_traits<Multi<AcceptSig...>(ClassType::*)(Args...) const>
      // we specialize for pointers to member function
      {
          typedef in<Args...> args_type;
          typedef out<AcceptSig...> accept_type;
          typedef err<> reject_type;

          static constexpr FutureMode mode = MULTI;

          typedef std::true_type ismultiple;
          typedef std::false_type isvoid;
      };
      template <typename ClassType, typename AcceptSig, typename... Args>
      struct lambda_traits<AcceptSig(ClassType::*)(Args...) const>
      // we specialize for pointers to member function
      {
          typedef typename std::is_same<void, AcceptSig> isvoid;

          typedef in<Args...> args_type;
          typedef typename std::conditional<isvoid::value, out<>, out<AcceptSig>>::type accept_type;
          typedef err<> reject_type;

          static constexpr  FutureMode mode = SINGLE;

          typedef std::false_type ismultiple;
      };

      //Traits for an error function
      template <typename T>
      struct efunction_traits
          : public efunction_traits<decltype(&T::operator())>
      {};
      template <typename ClassType, typename ReturnType, typename... Args>
      struct efunction_traits<ReturnType(ClassType::*)(Args...) const>
      {
          typedef err<Args...> args_type;
      };

    }
    //Unfortunately required so you can maintain a pointer to the next
    //future without knowing it's type
    class AbstractFuture
    {
    public:
    };

    template <class ...Tz> class Multi
    {
    public:
      Multi(Tz ...args) : _tup (std::tuple<Tz...>(args...))
      {
      }

      Multi()
      {
      }

      template <typename F, typename T, std::size_t... Idx>
      auto call(const F &f, T &&args, const priv::indices<Idx...> &)
      {
        return f(std::get<Idx>(std::forward<T>(args))...);
      }
      template <typename F>
      auto call(F const &f)
      {
        const priv::build_indices<std::tuple_size<std::tuple<Tz...>>::value> indices;
        return call(f, std::move(_tup), indices);
      }
      std::tuple<Tz...> _tup;
    };
    template<typename ...Tz>
    auto multi(Tz... args)
    {
      return Multi<Tz...>(args...);
    }
    //Forward declare the class in a sort of generic way
    //the real definition is a specialisation and anything
    //that doesn't match the specialisation will fail to link
    template< class, class, class, priv::FutureMode=priv::REJ_SINGLE> class Future;
    //Specialise, and restrict the packs to only being
    //used in the types of v* otherwise you don't
    //know where they are specified.
    template <
      template< class... > class vIn, class...vInType,
      template< class... > class vOut, class...vOutType,
      template< class... > class vErr, class...vErrType,
      priv::FutureMode futureMode >
    class Future< vIn<vInType...>, vOut<vOutType...>, vErr<vErrType...>, futureMode> : public AbstractFuture
    {
    public:
      //Some static information about the future used for conditional compilation
      typedef typename std::conditional<futureMode == priv::ACC_REJ, std::true_type, std::false_type>::type hasAcc;
      typedef typename std::conditional<futureMode == priv::REJ_MULTI || futureMode == priv::REJ_SINGLE, std::true_type, std::false_type>::type hasRej;
      typedef typename std::conditional<(futureMode == priv::REJ_MULTI) || (futureMode == priv::MULTI), std::true_type, std::false_type>::type retMulti;
      typedef typename std::conditional<(futureMode == priv::REJ_SINGLE) || (futureMode == priv::SINGLE), std::true_type, std::false_type>::type retSingle;
      typedef typename std::conditional<(futureMode == priv::MULTI) || (futureMode == priv::SINGLE), std::true_type, std::false_type>::type noFun;
      //The return type of the body of this future
      typedef typename std::conditional<retMulti::value, Multi<vOutType...>,
              typename std::conditional<retSingle::value, typename out<vOutType...>::first, void>::type>::type bodyret_t;
      //The storage type for the return of the body. Default to int if body doesn't return.
      //If this is a bound future, then use a Multi
      typedef typename std::conditional<(futureMode == priv::BOUND), Multi<vOutType...>,
              typename std::conditional<std::is_same<bodyret_t, void>::value, int, bodyret_t>::type>::type value_t;
      typedef std::function<void(vOutType...)> acceptfun_t ;
      typedef std::function<void(vErrType...)> rejectfun_t ;
      typedef in<vInType...> in_t;
      typedef out<vOutType...> out_t;
      typedef err<vErrType...> err_t;

      //We don't use the constructor except from construction functions
      //TODO: do the friend declarations and sort out the private/public members
      Future() = default;

      //This is if the bodyret is a Multi
      template <bool B = retMulti::value, typename std::enable_if<B,priv::t2>::type* = nullptr>
      void handle_bodyret()
      {
        if(errored) return;
        if(nxtAccept) value.call(nxtAccept);
      }
      //This is if the bodyret is a normal value
      template <bool B = retSingle::value, typename std::enable_if<B,priv::t1>::type* = nullptr>
      void handle_bodyret()
      {
        if(errored) return;
        if(nxtAccept) nxtAccept(value);
      }
      //This is used to assign value if the bodyret is not void
      template <bool B = std::is_same<bodyret_t, void>::value, typename std::enable_if<!B>::type* = nullptr, typename ...Tz>
      void callbody(Tz&& ...args)
      {
        value = body(std::forward<Tz>(args)...);
        handle_bodyret();
      }
      //This is used to call body without assigning the return value if its void
      template <bool B = std::is_same<bodyret_t, void>::value, typename std::enable_if<B>::type* = nullptr, typename ...Tz>
      void callbody(Tz&& ...args)
      {
        body(std::forward<Tz>(args)...);
      }
      //If we are a bound future (passthru)
      template <priv::FutureMode FM = futureMode, typename std::enable_if<FM==priv::BOUND,priv::t0>::type* = nullptr>
      void run()
      {
        if (nxtAccept) value.call(nxtAccept);
      }
      //If we are an Unbound future (passthru)
      //template <bool AC = hasAcc::value, bool NF = noFun::value, bool UB = (futureMode == priv::UNBOUND), typename std::enable_if<!AC && UB && NF>::type* = nullptr>
      template <priv::FutureMode FM = futureMode, typename std::enable_if<FM==priv::UNBOUND,priv::t1>::type* = nullptr>
      void run(vInType&&... args)
      {
        if (nxtAccept) nxtAccept(std::forward<vInType>(args)...);
        return;
      }
      //We are a payload future, with both accept and reject parameters
      template <bool HA = hasAcc::value, typename std::enable_if<HA,priv::t2>::type* = nullptr>
      void run(vInType&&... args)
      {
        if(!body) return;
        auto acceptfun = [&](vOutType&&... aargs)
        {
          if(nxtAccept) nxtAccept(std::forward<vOutType>(aargs)...);
        };
        auto rejectfun = [&](vErrType&&... eargs)
        {
          if(nxtReject) nxtReject(std::forward<vErrType>(eargs)...);
          else nxtForward();
          return;
        };
        body(acceptfun, rejectfun, std::forward<vInType>(args)...);
      }
      //We are a payload future, with just reject parameter
      template <bool HR = hasRej::value, typename std::enable_if<HR,priv::t3>::type* = nullptr>
      void run(vInType&&... args)
      {
        if(!body) return;
        auto rejectfun = [&](vErrType... eargs)
        {
          errored=true;
          if(nxtReject) nxtReject(eargs...);
          else nxtForward();
          return;
        };
        callbody(rejectfun, std::forward<vInType>(args)...);
      }
      //We are a payload future, with no parameters
      template <bool NF = noFun::value, typename std::enable_if<NF,priv::t4>::type* = nullptr>
      void run(vInType&&... args)
      {
        if(!body) return;
        callbody(std::forward<vInType>(args)...);
      }
      //These bind methods allow future N-1 to not know the full type
      //of promise N at construction time.
      //Bind the accept function
      void bindaccept(std::function<void(vInType...)> &f)
      {
        //we can bind by reference because the previous future
        //will stop the next future from being destructed
        f = [this](vInType&&... args){
          this->run(std::forward<vInType>(args)...);
        };
      }
      //If this is a catch future, and it doesn't take params,
      //then it can be used as a forward error target
      template <priv::FutureMode FM = futureMode, typename std::enable_if<(FM==priv::EXCEPT) && (sizeof...(vInType)==0)>::type* = nullptr>
      void bindforward(std::function<void(void)> &f)
      {
        f = [this](){
          this->errbody();
        };
      }
      //Otherwise if this is not a catch future, it just
      //forwards to the next one.
      template <priv::FutureMode FM = futureMode, typename std::enable_if<(FM!=priv::EXCEPT)>::type* = nullptr>
      void bindforward(std::function<void(void)> &f)
      {
        f = [this](){
          if (this->nxtForward)
          {
            this->nxtForward();
          }
          //TODO maybe crash hard on uncaught error?
        };
      }
      //If this is a catch future, and it DOES take params
      //then it can be used as a reject target
      template <priv::FutureMode FM = futureMode, typename std::enable_if<(FM==priv::EXCEPT) && (sizeof...(vInType)!=0)>::type* = nullptr>
      void bindreject(std::function<void(vInType...)> &f)
      {
        f = [this](vInType&&... args){
          this->errbody(std::forward<vInType>(args)...);
        };
      }
      template <class T> auto then(T const &lambda)
      {
        //Split the lambda
        typedef priv::lambda_traits<T> traits;
        static_assert(std::is_same<typename traits::args_type, in<vOutType...>>::value, "Future arguments don't match previous future output");
        auto nxtfuture = std::make_shared<Future<in<vOutType...>, typename traits::accept_type, typename traits::reject_type, traits::mode>>();
        nxtfuture->body = lambda;
        nxtfuture->bindaccept(nxtAccept);
        nxtfuture->bindforward(nxtForward);
        nxt = nxtfuture;
        return nxtfuture;
      }

      #if 0
      template <class T, class E> auto then(T const &lambda, E const &ehandler)
      {

        //Split the lambda
        typedef priv::function_traits<T> Ttraits;
        //typedef priv::function_traits<E> Etraits;

        static_assert(std::is_same<typename Ttraits::args_type, in<vOutType...>>::value, "Future arguments don't match previous future output");

        //ACCEPT future
        auto nxtfuture = std::make_shared<Future<in<vOutType...>, typename Ttraits::accept_type, typename Ttraits::reject_type, false, false, false>>();
        nxtfuture->body = lambda;
        nxtfuture->bindaccept(nxtAccept);
        nxtfuture->bindforward(nxtForward);
        nxt = nxtfuture;

        //REJECT future
        auto rejfuture = std::make_shared<Future<in<vErrType...>, out<>, err<>, false, true, false>>();
        rejfuture->errbody = ehandler;
        rejfuture->bindreject(nxtReject);
        nxtE = rejfuture;

        return nxtfuture;
      }
      #endif
      template <class E> void except(E const &lambda)
      {
        typedef priv::efunction_traits<E> etraits;
        static_assert(std::is_same<typename etraits::args_type, err<>>::value, "Catch lambda is not void");

        auto nxtfuture = std::make_shared<Future<in<>, out<>, err<>, priv::EXCEPT>>();
        nxtfuture->errbody = lambda;
        nxtfuture->bindforward(nxtForward);
        nxtF = nxtfuture;
      }



      typename std::conditional<hasAcc::value,
                         std::function<bodyret_t(acceptfun_t, rejectfun_t, vInType...)>,
                         typename std::conditional<hasRej::value,
                                  std::function<bodyret_t(rejectfun_t, vInType...)>,
                                  std::function<bodyret_t(vInType...)>
                                  >::type
                         >::type body;
      std::function<void(vInType...)> errbody;
      //std::tuple<vOutType...> _value;
      value_t value;
      std::tuple<vErrType...> _errvalue;
      std::shared_ptr<AbstractFuture> nxt;
      std::shared_ptr<AbstractFuture> nxtE;
      std::shared_ptr<AbstractFuture> nxtF;
      std::function<void(vOutType...)> nxtAccept;
      std::function<void(vErrType...)> nxtReject;
      std::function<void(void)> nxtForward;
      bool complete;
      bool accepted;
      bool isError;
      bool errored;
    };

    template<class ... Tz> auto bound(Tz ...args)
    {
      auto rv = std::make_shared<Future<in<>, out<Tz...>, err<>, priv::BOUND>>();
      rv->value = Multi<Tz...>(args...);
      return rv;
    }
    template<class ... Tz> auto unbound()
    {
      auto rv = std::make_shared<Future<in<Tz...>, out<Tz...>, err<>, priv::UNBOUND>>();
      return rv;
    }
  }
  namespace tq
  {
    class Task
    {
    public:
      Task(std::shared_ptr<std::function<void(void)>> target) : target(target){}
      void fire();
    private:
      const std::shared_ptr<std::function<void(void)>> target;
    };
    extern std::queue<Task> dyn_tq;
    template <typename T> bool add(T target)
    {
      dyn_tq.push(Task(std::make_shared<std::function<void(void)>>(target)));
      return true;
    }
    void __attribute__((noreturn)) scheduler();
  }

}
