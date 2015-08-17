
#include "interface.h"
#include "libstorm.h"
#include <array>
#include <queue>
#include <cstring>

namespace storm
{
  namespace tq
  {
    void Task::fire()
    {
      (*target)();
    }
    std::queue<Task> dyn_tq;
    template <> bool add(std::shared_ptr<std::function<void(void)>> target)
    {
      dyn_tq.push(Task(target));
      return true;
    }
    bool run_one()
    {
      if (dyn_tq.empty())
      {
        return false;
      }
      dyn_tq.front().fire();
      dyn_tq.pop();
      return true;
    }
    void __attribute__((noreturn)) scheduler()
    {
      while(1)
      {
        while(run_one());
        k_wait_callback();
      }
    }
  }
  namespace _priv
  {
    uint32_t __attribute__((naked)) syscall_ex(...)
    {
      asm volatile (\
          "push {r4-r11}\n\t"\
          "svc 8\n\t"\
          "pop {r4-r11}\n\t"\
          "bx lr":::"memory", "r0");
    }
    void irq_callback(uint32_t idx);
  }

  namespace gpio
  {
    //Pin directions
    const Dir OUT={0};
    const Dir IN={1};
    //Pins
    const Pin D0 = {0,0x0109};
    const Pin D1 = {1,0x010A};
    const Pin D2 = {2,0x0010};
    const Pin D3 = {3,0x000C};
    const Pin D4 = {4,0x0209};
    const Pin D5 = {5,0x000A};
    const Pin D6 = {6,0x000B};
    const Pin D7 = {7,0x0013};
    const Pin D8 = {8,0x000D};
    const Pin D9 = {9,0x010B};
    const Pin D10= {10,0x010C};
    const Pin D11= {11,0x010F};
    const Pin D12= {12,0x010E};
    const Pin D13= {13,0x010D};
    const Pin A0 = {14,0x0105};
    const Pin A1 = {15,0x0104};
    const Pin A2 = {16,0x0103};
    const Pin A3 = {17,0x0102};
    const Pin A4 = {18,0x0007};
    const Pin A5 = {19,0x0005};
    const Pin GP0= {20,0x020A};
    //Pin values
    const uint8_t HIGH = 1;
    const uint8_t LOW = 0;
    const uint8_t TOGGLE = 2;
    //Edges
    const Edge RISING = {1};
    const Edge FALLING = {2};
    const Edge CHANGE = {0};
    //Pulls
    const Pull UP = {1};
    const Pull DOWN = {2};
    const Pull KEEP = {3};
    const Pull NONE = {0};

    static std::shared_ptr<std::function<void()>> irq_ptrs [20] = {nullptr};

    uint32_t set_mode(Pin pin, Dir dir)
    {
      return _priv::syscall_ex(0x101, dir.dir, pin.spec);
    }
    uint32_t set(Pin pin, uint8_t value)
    {
      return _priv::syscall_ex(0x102, value, pin.spec);
    }
    void disable_irq(Pin pin)
    {
      if (irq_ptrs[pin.idx])
      {
        _priv::syscall_ex(0x108, pin.spec);
        irq_ptrs[pin.idx].reset();
      }
    }
    void enable_irq(Pin pin, Edge edge, std::shared_ptr<std::function<void(void)>> callback)
    {
      disable_irq(pin);
      irq_ptrs[pin.idx] = callback;
      _priv::syscall_ex(0x106, pin.spec, edge.edge, static_cast<void(*)(uint32_t)>(_priv::irq_callback), pin.idx);
    }
  }
  namespace _priv
  {
    void irq_callback(uint32_t idx)
    {
      if (idx <= 20 && gpio::irq_ptrs[idx])
      {
        tq::add(gpio::irq_ptrs[idx]);
      }
    }
  }

  namespace _priv
  {
    void tmr_callback(Timer *self);
  }


  template<> std::shared_ptr<Timer> Timer::once(uint32_t ticks, std::shared_ptr<std::function<void(void)>> callback)
  {
    auto rv = std::shared_ptr<Timer>(new Timer(false, ticks, callback));
    rv->self = rv; //Circle reference, we cannot be deconstructed
    return rv;
  }
  template<> std::shared_ptr<Timer> Timer::periodic(uint32_t ticks, std::shared_ptr<std::function<void(void)>> callback)
  {
    auto rv = std::shared_ptr<Timer>(new Timer(true, ticks, callback));
    rv->self = rv; //Circle reference, we cannot be deconstructed
    return rv;
  }
  void Timer::cancel()
  {
    //TODO: purge any queued callbacks from the task queue
    if(self)
    {
      _priv::syscall_ex(0x205, id);
      self.reset();
    }
  }
  void Timer::fire()
  {
    tq::add(callback);
    if (!is_periodic)
    {
      self.reset(); //undangle ourselves to be deconstructed
    }
  }
  Timer::Timer(bool periodic, uint32_t ticks, std::shared_ptr<std::function<void(void)>> callback):
    is_periodic(periodic), callback(callback)
  {
    id = _priv::syscall_ex(0x201, ticks, periodic, static_cast<void(*)(Timer*)>(_priv::tmr_callback), this);
  }
  namespace _priv
  {
    /*
     I expect this usage of a raw pointer to be safe under the following conditions:
      - The timer will not be deleted until cancel() is called
      - Cancel stops the kernel from creating more timer callbacks +
        also purges any previously issued callbacks from the task queue
     */
    void tmr_callback(Timer *self)
    {
      self->fire();
    }
    struct udp_recv_params_t
    {
        uint32_t reserved1;
        uint32_t reserved2;
        uint8_t* buffer;
        uint32_t buflen;
        uint8_t src_address [16];
        uint32_t port;
        uint8_t lqi;
        uint8_t rssi;
    } __attribute__((__packed__));
  }
  namespace sys
  {
    uint32_t now()
    {
      return _priv::syscall_ex(0x202);
    }
    uint32_t now(Shift shift)
    {
      return _priv::syscall_ex(shift.code);
    }
    const Shift SHIFT_0 = {0x202};
    const Shift SHIFT_16 = {0x203};
    const Shift SHIFT_48 = {0x204};
  }

  template<> std::shared_ptr<UDPSocket> UDPSocket::open(uint16_t port, std::shared_ptr<std::function<void(std::shared_ptr<UDPSocket::Packet>)>> callback)
  {
    auto rv = std::shared_ptr<UDPSocket>(new UDPSocket(port, callback));
    if (!rv->okay)
    {
      return std::shared_ptr<UDPSocket>();
    }
    rv->self = rv; //Circle reference, we cannot be deconstructed
    return rv;
  }
  void UDPSocket::close()
  {
    if(self)
    {
      //TODO purge callbacks from tq
      _priv::syscall_ex(0x303, id);
      self.reset();
    }
  }
  void UDPSocket::_handle(_priv::udp_recv_params_t *recv, char *addrstr)
  {
    auto rv = std::make_shared<Packet>();
    rv->payload = std::string(reinterpret_cast<const char*>(recv->buffer), static_cast<size_t>(recv->buflen));
    rv->strsrc = std::string(addrstr);
    std::memcpy(rv->src, recv->src_address, 16);
    rv->port = recv->port;
    rv->lqi = recv->lqi;
    rv->rssi = recv->rssi;
    (*callback)(rv);
  }
  UDPSocket::UDPSocket(uint16_t port, std::shared_ptr<std::function<void(std::shared_ptr<UDPSocket::Packet>)>> callback)
    :okay(false), callback(callback)
  {
    //create
    id = (int32_t) _priv::syscall_ex(0x301);
    if (id == -1) {
      return;
    }
    //bind
    int rv = _priv::syscall_ex(0x302, id, port);
    if (rv == -1) {
      return;
    }
    rv = _priv::syscall_ex(0x305, id, static_cast<void(*)(UDPSocket*, _priv::udp_recv_params_t*, char*)>(_priv::udp_callback), this);
    if (rv == -1) {
      return;
    }
    okay = true;
  }
  bool UDPSocket::sendto(const std::string &addr, uint16_t port, const std::string &payload)
  {
    return sendto(addr, port, reinterpret_cast<const uint8_t*>(&payload[0]), payload.size());
  }
  bool UDPSocket::sendto(const std::string &addr, uint16_t port, const uint8_t *payload, size_t length)
  {
    int rv = _priv::syscall_ex(0x304, id, payload, length, addr.data(), port);
    return rv == 0;
  }
  namespace _priv
  {
    void udp_callback(UDPSocket *sock, udp_recv_params_t *recv, char *addrstr)
    {
      sock->_handle(recv, addrstr);
    }
  }
  #if 0

  //---------- UDP
  #define udp_socket() k_syscall_ex_ri32(0x301)
  #define udp_bind(sockid,port) k_syscall_ex_ri32_u32_u32(0x302, (sockid), (port))
  #define udp_close(sockid) k_syscall_ex_ri32_u32(0x303, (sockid))
  #define udp_sendto(sockid, buffer, bufferlen, addr, port) k_syscall_ex_ri32_cptr_u32_cptr_u32(0x304, (sockid), (buffer), (bufferlen), (addr), (port))
  #define udp_set_recvfrom(sockid, cb, r) k_syscall_ex_ri32_u32_cb_vptr(0x305, (sockid), (cb), (r))
  #define udp_get_blipstats() k_syscall_ex_rvoid(0x306)
  #define udp_clear_blipstats() k_syscall_ex_rvoid(0x307)
  #define udp_get_retrystats() k_syscall_ex_rvoid(0x308)
  #define udp_clear_retrystats() k_syscall_ex_rvoid(0x309)

  //#define udp_unset_recvfrom(sockid) k_syscall_ex_ri32_u32(0x306, (sockid))

  //---------- SysInfo
  #define sysinfo_nodeid() k_syscall_ex_ru32(0x401)
  #define sysinfo_getmac(buffer) k_syscall_ex_ru32_u32(0x402, (buffer))
  #define sysinfo_getipaddr(buffer) k_syscall_ex_ru32_u32(0x403, (buffer))
  #define sysinfo_reset() k_syscall_ex_ru32(0x404);
  #define sysinfo_setlocklevel(level) k_syscall_ex_ru32_u32(0x405, (level))

  //---------- I2C
  #define i2c_transact(iswrite, address, flags, buffer, len, callback, r) k_syscall_ex_ri32_u32_u32_u32_buf_u32_vptr_vptr((0x500 + (iswrite)), (address), (flags), (buffer), (len), (callback), (r))

  //---------- Bluetooth
  #define bl_enable(on_ready, on_ready_r, on_changed, on_changed_r, adv, advlen) k_syscall_ex_ri32_cb_vptr_cb_vptr_cptr_u32(0x601, (on_ready), (on_ready_r), (on_changed), (on_changed_r), (adv), (advlen))
  #define bl_addservice(uuid) k_syscall_ex_ri32_u32(0x602, (uuid))
  #define bl_addcharacteristic(svc_handle, uuid, on_write, on_write_r) k_syscall_ex_ri32_u32_u32_cb_vptr(0x603, (svc_handle), (uuid), (on_write), (on_write_r))
  #define bl_notify(char_handle, buffer_len, buffer) k_syscall_ex_ri32_u32_u32_cptr(0x604, (char_handle), (buffer_len), (buffer) )

  //---------- Routing Table
  #define routingtable_addroute(prefix, prefix_len, nexthop, ifindex) k_syscall_ex_rcptr_u32_cptr_u32(0x701, (prefix), (prefix_len), (nexthop), (ifindex))
  #define routingtable_delroute(key) k_syscall_ex_ri32_u32(0x702, (key))
  #define routingtable_getroute(key, buffer) k_syscall_ex_ri32_u32_u32(0x703, (key), (buffer))
  #define routingtable_lookuproute(prefix, prefix_len, buffer) k_syscall_ex_rcptr_u32_u32(0x704, (prefix), (prefix_len), (buffer))
  #define routingtable_gettable(size, buffer) k_syscall_ex_ri32_u32_u32(0x705, (size), (buffer))

  #define aes_encrypt(iv, mlen, message, dest) \
      k_syscall_ex_ri32_cptr_u32_cptr_cptr(0x801, (iv),(mlen),(message),(dest))
  #define aes_decrypt(iv, mlen, message, dest) \
      k_syscall_ex_ri32_cptr_u32_cptr_cptr(0x802, (iv),(mlen),(message),(dest))
  #define aes_setkey(key) k_syscall_ex_ri32_cptr(0x803, (key))

  #define spi_set_cs(state) k_syscall_ex_ri32_u32(0x901, (state))
  #define spi_init(mode, baudrate) k_syscall_ex_ri32_u32_u32(0x902, (mode), (baudrate))
  #define spi_write(txbuf, rxbuf, len, cb, r) k_syscall_ex_ri32_vptr_vptr_uint32_vptr_vptr(0x903, (txbuf), (rxbuf), (len), (cb), (r))

  #define flash_write(addr, buf, len, cb, r) k_syscall_ex_ri32_uint32_vptr_uint32_vptr_vptr(0xa02, (addr), (buf),(len),(cb),(r))
  #define flash_read(addr, buf, len, cb, r) k_syscall_ex_ri32_uint32_vptr_uint32_vptr_vptr(0xa01, (addr), (buf),(len),(cb),(r))
  #define flash_erase() k_syscall_ex_ri32(0xa03)
  #define flash_isbusy() k_syscall_ex_ri32(0xa04)
  #endif
}
