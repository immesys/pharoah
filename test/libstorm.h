#include <memory>
#include <functional>
#include <queue>
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
      //    enum { arity = sizeof...(Args) };
          // arity is the number of arguments.

          typedef out<AcceptSig...> accept_type;
          typedef err<RejectSig...> reject_type;
        /*  typedef ReturnType result_type;

          template <size_t i>
          struct arg
          {
              typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
              // the i-th argument is equivalent to the i-th tuple element of a tuple
              // composed of those arguments.
          };*/
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
    template< class, class, class, bool=false> class Future;
    //Specialise, and restrict the packs to only being
    //used in the types of v* otherwise you don't
    //know where they are specified.
    template <
      template< class... > class vIn, class...vInType,
      template< class... > class vOut, class...vOutType,
      template< class... > class vErr, class...vErrType,
      bool bound
      > class Future< vIn<vInType...>, vOut<vOutType...>, vErr<vErrType...>, bound> : public AbstractFuture
    {
    public:
      typedef std::function<void(vOutType...)> acceptfun_t ;
      typedef std::function<void(vErrType...)> rejectfun_t ;
      Future() = default;
      /*
      Future(std::shared_ptr<std::function<void(acceptfun_t, rejectfun_t, vInType...)>> callable)
      {
        body = callable;
      }*/

/*
      static auto value(vOutType... args)
      {
        auto rv = Future<in<>, out<vOutType...>, err<>>();
        rv._value = std::make_tuple(args...);
        return rv;
      }*/
      //If we are a bound future
      template <bool B = bound, typename std::enable_if<B>::type* = nullptr>
      void run()
      {
        //printf("are bound\n");
        //TODO use gens
      }
      // or there are no in parameters, declare a void run()
      template <bool B = bound, typename std::enable_if<!B>::type* = nullptr>
      void run(vInType... args)
      {
        if (passthru)
        {
          //Call next promise
          return;
        }
        auto acceptfun = [&](vOutType... args)
        {
          _value = std::make_tuple(args...);
          complete = true;
          accepted = true;
          //priv::call(/*target*/, std::move(_value));
          //onAccept(args...);
        };
        auto rejectfun = [&](vErrType... args)
        {
          _errvalue = std::make_tuple(args...);
          complete = true;
          accepted = false;
          //onReject(args...);
        };
        (*body)(acceptfun, rejectfun, args...);

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
      //Bind the forward function (reject to a non-handling future)
      void bindforward(std::function<void(void)> &f)
      {
        f = /*std::function<void(vInType...)>*/[this](){
          if (this->isError)
          {
            (*(this->body))();
          }
          else
          {
            if (this->nxtForward)
            {
              this->nxtForward();
            }
            //TODO maybe crash hard on uncaught error?
          }
        };
      }
      template <class T> auto then(T lambda)
      {
        //Split the lambda
        typedef priv::function_traits<decltype(lambda)> traits;
        auto nxtfuture = Future<in<vOutType...>, typename traits::accept_type, typename traits::reject_type>();
        nxtfuture.passthru = false;
        typedef typename decltype(nxtfuture)::acceptfun_t afun;
        typedef typename decltype(nxtfuture)::rejectfun_t rfun;
        nxtfuture.body = std::make_shared<std::function<void(afun, rfun, vOutType...)>>(lambda);
        nxtfuture.bindaccept(nxtAccept);
        nxtfuture.bindforward(nxtForward);
        return nxtfuture;
      }
/*
      auto makeBindAccept()
      {
        return std::make_unique<std::function<void(vInType...)>>([&](vInType... args){
          run(args...);
        });
      }*/
      //this was an attempt at trying to get then() to take a closure.
      //problem is that you need to know what the out type is.
      //its apparently difficult to do the double param pack
      //method with a function

      //idea: use a class or whatever, but if you make the lambda
      //pass in specific types for accept and reject in the parameters
      //you can probably auto deduce them.
      //maybe do this for the promise constructor, and make then
      //forward everything as one giant pack list to the promise constructor?
      //i think that'll work.
      /*
      template <class, class, class T> auto then(T target);
      template <
        template< class... > class nOut, class... nOutType,
        template< class... > class nErr, class... nErrType,
          class T
        > auto then< nOut<nOutType...>, nErr<nErrType...>, T> (T target)
      {
        //auto rv = std::make_shared<Future<in<vOutType...>, out<>, R>>();
      //  auto nxt = wrap(target);
      //  onAccept = nxt;
      //  auto bar = nxt->makeBindAccept();
    //    bindAcceptFun = nxt->makeBindAccept();
        //std::function<void(vOutType...)> afun = (*nxt).run;
        //onAcceptFun = std::make_shared<std::function<void(vOutType...)>>(std::bind(&nxt->run, nxt));
      }
      */
      #if 0
      //Otherwise, if we are not bound and there are in parameters, declare run with params
      template <bool B = bound, bool P = sizeof...(vInType)==0, typename std::enable_if<!(B||P)>::type* = nullptr>
      void run(vInType... args)
      {

      }
      #endif
      /*
      template <class T, class R=out<>> auto wrap(T target)
      {

        rv->body = std::make_shared<std::function<void(acceptfun_t, rejectfun_t, vOutType...)>>(target);
        return rv;
      }*/

      //template<class A> auto then(A accept);
      //template<class A, class R> auto then(A accept, R reject);
//    private:
      std::shared_ptr<std::function<void(acceptfun_t, rejectfun_t, vInType...)>> body;
      std::tuple<vOutType...> _value;
      std::tuple<vErrType...> _errvalue;
      std::shared_ptr<AbstractFuture> onAccept;
      std::function<void(vOutType...)> nxtAccept;
      std::function<void(vErrType...)> nxtReject;
      std::function<void(void)> nxtForward;
      bool complete;
      bool accepted;
      bool isError;
      bool passthru = true;
    };


    /*
    template <class vOut, class vErr, class T, class vIn, class ... vInZ > auto resolve(const std::function<void(vOut, vErr, vInZ...)>& callable)
    {
      auto rv = Future<in<vInZ...>, vOut, vErr, false>();
      //decltype(rv)::foobaz();
      //std::function<void(std::function<void(void)>, std::function<void(void)>, int)> a = std::function<void(std::function<void(void)>, std::function<void(void)>, int)>(callable);
    //  auto a = std::function<void(decltype(rv)::acceptfun_t, decltype(rv)::rejectfun_t, int)>(callable);
      //rv.body = std::make_shared<>(callable);

      return rv;
    }
    */
    template<class ... Tz> auto bound(Tz ...args)
    {
      auto rv = Future<in<>, out<Tz...>, err<>, true>();
      rv._value = std::make_tuple(args...);
      rv.passthru = true;
      return rv;
    }
    template<class ... Tz> auto unbound()
    {
      auto rv = Future<in<Tz...>, out<Tz...>, err<>, false>();
      rv.passthru = true;
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
  namespace gpio
  {
    struct Pin {
      const uint16_t idx;
      const uint16_t spec;
    };
    struct Dir {
      const uint8_t dir;
    };
    struct Edge {
      const uint8_t edge;
    };
    struct Pull {
      const uint8_t dir;
    };

    //Pin directions
    extern const Dir OUT;
    extern const Dir IN;
    //Pins
    extern const Pin D0;
    extern const Pin D1;
    extern const Pin D2;
    extern const Pin D3;
    extern const Pin D4;
    extern const Pin D5;
    extern const Pin D6;
    extern const Pin D7;
    extern const Pin D8;
    extern const Pin D9;
    extern const Pin D10;
    extern const Pin D11;
    extern const Pin D12;
    extern const Pin D13;
    extern const Pin A0;
    extern const Pin A1;
    extern const Pin A2;
    extern const Pin A3;
    extern const Pin A4;
    extern const Pin A5;
    extern const Pin GP0;
    //Pin values
    extern const uint8_t HIGH;
    extern const uint8_t LOW;;
    extern const uint8_t TOGGLE;;
    //Edges
    extern const Edge RISING;
    extern const Edge FALLING;
    extern const Edge CHANGE;
    //Pull
    extern const Pull UP;
    extern const Pull DOWN;
    extern const Pull KEEP;
    extern const Pull NONE;

    //GPIO functions
    uint32_t set_mode(Pin pin, Dir dir);
    uint32_t set(Pin pin, uint8_t value);
    uint8_t get(Pin pin);
    uint8_t get_shadow(Pin pin);
    void set_pull(Pin pin, Pull pull);
    void enable_irq(Pin pin, Edge edge, std::shared_ptr<std::function<void()>> callback);
    void disable_irq(Pin pin);
  }

  class Timer
  {
  public:
    template<typename T> static std::shared_ptr<Timer> once(uint32_t ticks, T callback)
    {
      auto rv = std::shared_ptr<Timer>(new Timer(false, ticks, std::make_shared<std::function<void(void)>>(callback)));
      rv->self = rv; //Circle reference, we cannot be deconstructed
      return rv;
    }
    template<typename T> static std::shared_ptr<Timer> periodic(uint32_t ticks, T callback)
    {
      auto rv = std::shared_ptr<Timer>(new Timer(true, ticks, std::make_shared<std::function<void(void)>>(callback)));
      rv->self = rv; //Circle reference, we cannot be deconstructed
      return rv;
    }

    void cancel();
    void fire();
    static constexpr uint32_t MILLISECOND = 375;
    static constexpr uint32_t SECOND = MILLISECOND*1000;
    static constexpr uint32_t MINUTE = SECOND*60;
    static constexpr uint32_t HOUR = MINUTE*60;


  private:
    Timer(bool periodic, uint32_t ticks, std::shared_ptr<std::function<void(void)>> callback);
    uint16_t id;
    bool is_periodic;
    const std::shared_ptr<std::function<void(void)>> callback;
    std::shared_ptr<Timer> self;
  };
  namespace sys
  {
    struct Shift {
      const uint16_t code;
    };
    uint32_t now();
    uint32_t now(Shift shift);
    extern const Shift SHIFT_0;
    extern const Shift SHIFT_16;
    extern const Shift SHIFT_48;
  }
  class UDPSocket;
  namespace _priv
  {
    struct udp_recv_params_t;
    void udp_callback(UDPSocket *sock, udp_recv_params_t *recv, char *addrstr);
  }
  class UDPSocket
  {
  public:
    class Packet
    {
    public:
      std::string payload;
      std::string strsrc;
      uint8_t src[16];
      uint16_t port;
      uint8_t lqi;
      uint8_t rssi;
    };
    template<typename T> static std::shared_ptr<UDPSocket> open(uint16_t port, T callback)
    {
      auto rv = std::shared_ptr<UDPSocket>(new UDPSocket(port, std::make_shared<std::function<void(std::shared_ptr<Packet>)>>(callback)));
      if (!rv->okay)
      {
        return std::shared_ptr<UDPSocket>();
      }
      rv->self = rv; //Circle reference, we cannot be deconstructed
      return rv;
    }
    void close();
    void _handle(_priv::udp_recv_params_t *recv, char *addrstr);
    bool sendto(const std::string &addr, uint16_t port, const std::string &payload);
    bool sendto(const std::string &addr, uint16_t port, const uint8_t *payload, size_t length);
  private:
    UDPSocket(uint16_t port, std::shared_ptr<std::function<void(std::shared_ptr<Packet>)>> callback);
    int32_t id;
    bool okay;
    const std::shared_ptr<std::function<void(std::shared_ptr<Packet>)>> callback;
    std::shared_ptr<UDPSocket> self;
  };

}

//#define udp_sendto(sockid, buffer, bufferlen, addr, port) k_syscall_ex_ri32_cptr_u32_cptr_u32(0x304, (sockid), (buffer), (bufferlen), (addr), (port))
