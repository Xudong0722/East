/*
 * @Author: Xudong0722 
 * @Date: 2025-04-08 23:00:31 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-04-10 21:01:19
 */

#include "Hook.h"
#include <dlfcn.h>
#include "Fiber.h"
#include "IOManager.h"
#include "FdManager.h"

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

struct timer_info {
  int cancelled{0};
};

//将非阻塞的IO调用函数改成协程异步调用，可以指定超时时间
template<class OriginalFunc, class ...OriginalFuncParams>
ssize_t do_io(int fd, OriginalFunc func, const char* hook_func_name, uint32_t event
    , int timeout_so, OriginalFuncParams&&... params) {
  if(!is_hook_enable()) {
    //调用原始接口
    return func(fd, std::forward<OriginalFuncParams>(params)...);
  }

  //获取fd状态信息
  auto fd_status = FdMgr::GetInst()->getFd(fd);
  if(nullptr == fd_status) {
    return func(fd, std::forward<OriginalFuncParams>(params)...);
  }

  //EBADF: Bad file descriptor
  if(fd_status->isClosed()) {
    errno = EBADF;
    return -1;
  }
  
  //如果这个fd不是socket或者用户之前已经设置过这个fd是非阻塞的，直接调用原函数即可
  if(!fd_status->isSocket() || fd_status->isUserNonBlock()) {
    return func(fd, std::forward<OriginalFuncParams>(params)...);
  }

  uint64_t timeout{0};
  if(timeout_so == SO_SNDTIMEO) {
    timeout = fd_status->getSendTimeout();
  }else if(timeout_so == SO_RECVTIMEO) {
    timeout = fd_status->getRecvTimeout();
  }
  
  std::shared_ptr<timer_info> tinfo = std::make_shared<timer_info>();
retry:

  ssize_t res = func(fd, std::forward<OriginalFuncParams>(params)...);
  while(res == -1 && errno == EINTR) { 
    //如果我们的调用被中断了，继续调用
    res = func(fd, std::forward<OriginalFuncParams>(params)...);
  }

  if(res == -1 && errno == EAGAIN) {
    //非阻塞IO，常见资源不可用，所以我们可以通过协程调度，设置一个超时时间，之后再次调用
    
    auto io_mgr = East::IOManager::GetThis();

    if(timeout != (uint64_t)-1)
    auto Timer::sptr = io_mgr->addConditionTimer()
  }
  return res;
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

int nanosleep(const struct timespec* req, struct timespec* rem) {
  if(!East::is_hook_enable()) {
    return nanosleep_f(req, rem);
  }

  auto fiber = East::Fiber::GetThis();
  auto io_mgr = East::IOManager::GetThis();
  if(nullptr != fiber && nullptr != io_mgr) {
    io_mgr->addTimer(req->tv_sec * 1000 + req->tv_nsec / 1000000,
      [io_mgr, fiber]() {
        if(nullptr != io_mgr) {
          io_mgr->schedule(fiber);
        }
      });
  }
  fiber->yield();
  return 0;
}

int socket(int domain, int type, int protocol) {
  if(!East::is_hook_enable()) {
    return socket_f(domain, type, protocol);
  }

  int fd = socket_f(domain, type, protocol);
  if(fd < 0) return fd;

  East::FdMgr::GetInst()->getFd(fd, true);   //放到FdManager中管理，方便后续判断
  return fd;
}

}
