/*
 * @Author: Xudong0722 
 * @Date: 2025-04-08 23:00:31 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-04-09 01:27:30
 */

#include "Hook.h"
#include <dlfcn.h>
#include "Fiber.h"
#include "IOManager.h"

namespace East {
static thread_local bool t_hook_enable = false;

//宏定义：要hook的所有函数
#define HOOK_FUNC(func)                                                  \
  func(sleep) func(usleep) func(nanosleep) func(socket) func(connect)    \
      func(accept) func(read) func(readv) func(recv) func(recvfrom)      \
          func(recvmsg) func(write) func(writev) func(send) func(sendto) \
              func(sendmsg) func(close) func(fcntl) func(ioctl)          \
                  func(getsockopt) func(setsockopt)

void hook_init() {
  static bool s_inited = false;
  if (s_inited) {
    return;
  }
  s_inited = true;
//将要hook的函数从动态库中找出来赋值, HOOK_FUNC(hook)会展开成hook(sleep) 等所有要hook的函数，然后展开： hook(sleep) sleep_f = (sleep_func)dlsym(RTLD_NEXT, "sleep")
//从下一个动态库（通常是 libc）中查找 "sleep" 函数的地址（避开当前这个hook）把结果赋给 sleep_f
#define hook(name) name##_f = (name##_func)dlsym(RTLD_NEXT, #name);
  HOOK_FUNC(hook)
#undef hook
}

struct _HookIniter {
  _HookIniter() { hook_init(); }
};

static _HookIniter s_hook_initer;  //main函数之前初始化，

bool is_hook_enable() {
  return t_hook_enable;
}
void set_hook_enable(bool enable) {
  t_hook_enable = enable;
}
}  // namespace East

extern "C" {
//define the function pointer variables
#define def_func(name) name##_func name##_f = nullptr;
HOOK_FUNC(def_func)
#undef def_func

unsigned int sleep(unsigned int seconds) {
  if (!East::is_hook_enable()) {
    return sleep_f(seconds);
  }

  auto fiber = East::Fiber::GetThis();
  auto io_mgr = East::IOManager::GetThis();

  if (nullptr != fiber && nullptr != io_mgr) {
    ELOG_INFO(ELOG_ROOT()) << "enter sleep, seconds: " << seconds;
    io_mgr->addTimer(seconds * 1000, [io_mgr, fiber]() {
      if (nullptr != io_mgr) {
        io_mgr->schedule(fiber);
      }
    });
  }
  //East::Fiber::YieldToHold();
  fiber->yield();
  return 0u;
}

int usleep(useconds_t usec) {
  if (!East::is_hook_enable()) {
    return usleep_f(usec);
  }
  auto fiber = East::Fiber::GetThis();
  auto io_mgr = East::IOManager::GetThis();

  if (nullptr != fiber && nullptr != io_mgr) {
    io_mgr->addTimer(usec / 1000, [io_mgr, fiber]() {
      if (nullptr != io_mgr) {
        io_mgr->schedule(fiber);
      }
    });
  }

  //East::Fiber::YieldToHold();
  fiber->yield();
  return 0;
}
}
