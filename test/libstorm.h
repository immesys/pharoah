#include <memory>
#include <functional>
#include <queue>
namespace storm
{
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
  
}
