#include <memory>
#include <functional>
#include <queue>
namespace storm
{
  using buf_t = std::unique_ptr<uint8_t[]>;
  std::unique_ptr<uint8_t[]> mkbuf(size_t size);
  std::unique_ptr<uint8_t[]> mkbuf(std::initializer_list<uint8_t> contents);
  namespace _priv
  {
    uint32_t __attribute__((naked)) syscall_ex(...);
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
    void reset();
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
  namespace i2c
  {
    class I2CWOperation;
    class I2CROperation;
  }
  namespace _priv
  {
    void i2c_wcallback(i2c::I2CWOperation *op, int status);
    void i2c_rcallback(i2c::I2CROperation *op, int status);
  }
  namespace i2c
  {
    class I2CFlag
    {
    public:
      constexpr I2CFlag operator| (I2CFlag const& rhs)
      {
        return I2CFlag{val+rhs.val};
      }
      uint32_t val;
    };
    class I2CWOperation
    {
    public:
      I2CWOperation(std::unique_ptr<uint8_t[]> payload, std::function<void(int, buf_t)> callback)
        :payload(std::move(payload)), callback(callback)
      {
      }
      void invoke(int status)
      {
        callback(status, std::move(payload));
        self.reset();
      }
      std::unique_ptr<uint8_t[]> payload;
      std::function<void(int, buf_t)> callback;
      std::shared_ptr<I2CWOperation> self;
    };
    class I2CROperation
    {
    public:
      I2CROperation(std::unique_ptr<uint8_t[]> payload, size_t length, std::function<void(int, std::unique_ptr<uint8_t[]>)> callback)
        : length(length), payload(std::move(payload)), callback(callback)
      {
      }
      void invoke(int status)
      {
        callback(status, std::move(payload));
        self.reset();
      }
      size_t length;
      std::unique_ptr<uint8_t[]> payload;
      std::function<void(int, std::unique_ptr<uint8_t[]>)> callback;
      std::shared_ptr<I2CROperation> self;
    };
    template <typename T> std::shared_ptr<I2CWOperation> write(uint16_t address, I2CFlag const &flags, buf_t payload, uint16_t length, T callback)
    {
      auto rv = std::make_shared<I2CWOperation>(std::move(payload), std::function<void(int, buf_t)>(callback));
      rv->self = rv; //circular reference to prevent dealloc.
      _priv::syscall_ex(0x502, address, flags.val, rv->payload.get(), length, _priv::i2c_wcallback, rv.get());
      return rv;
    }
    template <typename T> std::shared_ptr<I2CROperation> read(uint16_t address, I2CFlag const &flags, buf_t target, uint16_t length, T callback)
    {
      auto rv = std::make_shared<I2CROperation>(std::move(target), length, std::function<void(int, buf_t)>(callback));
      rv->self = rv; //circular reference to prevent dealloc.
      _priv::syscall_ex(0x501, address, flags.val, rv->payload.get(), length, _priv::i2c_rcallback, rv.get());
      return rv;
    }
    constexpr uint16_t internal(uint8_t address)
    {
      return 0x200 + address;
    }
    constexpr uint16_t external(uint8_t address)
    {
      return 0x100 + address;
    }
    constexpr uint16_t TMP006 = internal(0x80);
    constexpr I2CFlag START = I2CFlag{1};
    constexpr I2CFlag RSTART = I2CFlag{1};
    constexpr I2CFlag ACKLAST = I2CFlag{2};
    constexpr I2CFlag STOP = I2CFlag{4};
    constexpr int OK = 0;
    constexpr int DNAK = 1;
    constexpr int ANAK = 2;
    constexpr int ERR = 3;
    constexpr int ARBLST = 4;
  }
}

//#define udp_sendto(sockid, buffer, bufferlen, addr, port) k_syscall_ex_ri32_cptr_u32_cptr_u32(0x304, (sockid), (buffer), (bufferlen), (addr), (port))
