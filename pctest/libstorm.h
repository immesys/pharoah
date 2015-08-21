#include <memory>
#include <functional>
#include <queue>
#include <stdio.h>
namespace storm
{
  namespace future
  { //this namespace contains dragons. I don't know how the fuck any of this shit works
    //and I wrote it.
    template<class ... Tz> using accept = std::function<void(Tz...)>;
    template<class ... Tz> using reject = std::function<void(Tz...)>;
    //Define some nicely named structs to use to pass type signatures into
    //future classes. We could have used std::tuple but this makes it easier
    //to read
    template<class ... Tz> struct in{};
    template<class ... Tz> struct out{};
    template<class ... Tz> struct err{};
    namespace priv
    {
      //Taken from http://stackoverflow.com/questions/7943525/
      //roughly
      //also a good way if we need to store/retrieve the params
      //http://stackoverflow.com/questions/4691657/
      template <typename T>
      struct function_traits
          : public function_traits<decltype(&T::operator())>
      {};
      // For generic types, directly use the result of the signature of its 'operator()'

      template <typename ClassType, typename ReturnType, typename ...AcceptSig, typename ...RejectSig, typename... Args>
      struct function_traits<ReturnType(ClassType::*)(accept<AcceptSig...>, reject<RejectSig...>, Args...) const>
      // we specialize for pointers to member function
      {
          typedef out<AcceptSig...> accept_type;
          typedef err<RejectSig...> reject_type;
          typedef in<Args...> args_type;
      };

      //Taken from http://stackoverflow.com/questions/7943525/
      //roughly
      //also a good way if we need to store/retrieve the params
      //http://stackoverflow.com/questions/4691657/
      template <typename T>
      struct efunction_traits
          : public efunction_traits<decltype(&T::operator())>
      {};
      // For generic types, directly use the result of the signature of its 'operator()'

      template <typename ClassType, typename ReturnType, typename... Args>
      struct efunction_traits<ReturnType(ClassType::*)(Args...) const>
      // we specialize for pointers to member function
      {
          typedef err<Args...> args_type;
      };

      //taken from http://stackoverflow.com/questions/7858817/
      //and http://stackoverflow.com/questions/15537817/
      template < ::std::size_t... Indices>
      struct indices {};
      template < ::std::size_t N, ::std::size_t... Is>
      struct build_indices : build_indices<N-1, N-1, Is...> {};
      template < ::std::size_t... Is>
      struct build_indices<0, Is...> : indices<Is...> {};

      template <typename FuncT, typename ArgTuple, ::std::size_t... Indices>
      auto call(const FuncT &f, ArgTuple &&args, const indices<Indices...> &)
         -> decltype(f(::std::get<Indices>(::std::forward<ArgTuple>(args))...))
      {
         return ::std::move(f(::std::get<Indices>(::std::forward<ArgTuple>(args))...));
      }

      template <typename FuncT, typename ArgTuple>
      auto call(const FuncT &f, ArgTuple &&args)
           -> decltype(call(f, args,
                            build_indices< ::std::tuple_size<ArgTuple>::value>{}))
      {
          const build_indices< ::std::tuple_size<ArgTuple>::value> indices;
          return ::std::move(call(f, ::std::move(args), indices));
      }

    }
    class AbstractFuture
    {
    public:
    };


    //Forward declare the class in a sort of generic way
    //the real definition is a specialisation and anything
    //that doesn't match the specialisation will fail to link
    template< class, class, class, bool=false, bool=false, bool=false> class Future;
    //Specialise, and restrict the packs to only being
    //used in the types of v* otherwise you don't
    //know where they are specified.
    template <
      template< class... > class vIn, class...vInType,
      template< class... > class vOut, class...vOutType,
      template< class... > class vErr, class...vErrType,
      bool isBound, bool isCatch, bool isPassthru
      > class Future< vIn<vInType...>, vOut<vOutType...>, vErr<vErrType...>, isBound, isCatch, isPassthru> : public AbstractFuture
    {
    public:
      typedef std::function<void(vOutType...)> acceptfun_t ;
      typedef std::function<void(vErrType...)> rejectfun_t ;
      typedef in<vInType...> in_t;
      typedef out<vOutType...> out_t;
      typedef err<vErrType...> err_t;

      Future() = default;

      //If we are a bound future
      template <bool B = isBound, bool P = isPassthru, typename std::enable_if<B>::type* = nullptr>
      void run()
      {
        printf("are bound\n");
        //TODO use gens
      }
      // or there are no in parameters, declare a void run()
      template <bool B = isBound, bool P = isPassthru, typename std::enable_if<!B && P>::type* = nullptr>
      void run(vInType... args)
      {
        printf("are passthru\n");
        if (nxtAccept) nxtAccept(args...);
        //Call next promise
        return;
      }

      // or there are no in parameters, declare a void run()
      template <bool B = isBound, bool P = isPassthru, typename std::enable_if<!B && !P>::type* = nullptr>
      void run(vInType... args)
      {
        if(!body)
        {
          return;
        }
        auto acceptfun = [&](vOutType... args)
        {
          _value = std::make_tuple(args...);
          complete = true;
          accepted = true;
          if(nxtAccept) nxtAccept(args...);
          //priv::call(/*target*/, std::move(_value));
          //onAccept(args...);
        };
        auto rejectfun = [&](vErrType... args)
        {
          _errvalue = std::make_tuple(args...);
          complete = true;
          accepted = false;
          if(nxtReject) nxtReject(args...);
          else nxtForward();
          //onReject(args...);
        };
        body(acceptfun, rejectfun, args...);
      }
      //These bind methods allow future N-1 to not know the full type
      //of promise N at construction time.
      //Bind the accept function
      void bindaccept(std::function<void(vInType...)> &f)
      {
        //we can bind by reference because the previous future
        //will stop the next future from being destructed
        f = [this](vInType... args){
          this->run(args...);
        };
      }
      //If this is a catch future, and it doesn't take params,
      //then it can be used as a forward error target
      template <bool B = isCatch, bool P = sizeof...(vInType) == 0, typename std::enable_if<B&&P>::type* = nullptr>
      void bindforward(std::function<void(void)> &f)
      {
        f = [this](){
          this->errbody();
        };
      }
      //Otherwise if this is not a catch future, it just
      //forwards to the next one.
      template <bool B = isCatch, typename std::enable_if<!B>::type* = nullptr>
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
      template <bool B = isCatch, typename std::enable_if<B>::type* = nullptr>
      void bindreject(std::function<void(vInType...)> &f)
      {
        f = [this](vInType... args){
          this->errbody(args...);
        };
      }

      template <class T> auto then(T const &lambda)
      {
        //Split the lambda
        typedef priv::function_traits<T> traits;
        static_assert(std::is_same<typename traits::args_type, in<vOutType...>>::value, "Future arguments don't match previous future output");
        auto nxtfuture = std::make_shared<Future<in<vOutType...>, typename traits::accept_type, typename traits::reject_type, false, false, false>>();
        nxtfuture->body = lambda;
        nxtfuture->bindaccept(nxtAccept);
        nxtfuture->bindforward(nxtForward);
        nxt = nxtfuture;
        return nxtfuture;
      }
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
      template <class E> void except(E const &lambda)
      {
        typedef priv::efunction_traits<E> etraits;
        static_assert(std::is_same<typename etraits::args_type, err<>>::value, "Catch lambda is not void");

        auto nxtfuture = std::make_shared<Future<in<>, out<>, err<>, false, true, false>>();
        nxtfuture->errbody = lambda;
        nxtfuture->bindforward(nxtForward);
        nxtF = nxtfuture;
      }

      std::function<void(acceptfun_t, rejectfun_t, vInType...)> body;
      std::function<void(vInType...)> errbody;
      std::tuple<vOutType...> _value;
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
    };

    template<class ... Tz> auto bound(Tz ...args)
    {
      auto rv = std::make_shared<Future<in<>, out<Tz...>, err<>, true, false, true>>();
      rv->_value = std::make_tuple(args...);
      return rv;
    }
    template<class ... Tz> auto unbound()
    {
      auto rv = std::make_shared<Future<in<Tz...>, out<Tz...>, err<>, false, false, true>>();
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
