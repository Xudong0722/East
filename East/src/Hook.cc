/*
 * @Author: Xudong0722 
 * @Date: 2025-04-08 23:00:31 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-04-10 21:01:19
 */

#include "Hook.h"
#include <dlfcn.h>
#include "FdManager.h"
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

struct timer_info {
  int cancelled{0};
};

//将非阻塞的IO调用函数改成协程异步调用，可以指定超时时间
template <class OriginalFunc, class... OriginalFuncParams>
ssize_t do_io(int fd, OriginalFunc func, const char* hook_func_name,
              uint32_t event, int timeout_so, OriginalFuncParams&&... params) {
  if (!is_hook_enable()) {
    //调用原始接口
    return func(fd, std::forward<OriginalFuncParams>(params)...);
  }

  //获取fd状态信息
  auto fd_status = FdMgr::GetInst()->getFd(fd);
  if (nullptr == fd_status) {
    return func(fd, std::forward<OriginalFuncParams>(params)...);
  }

  //EBADF: Bad file descriptor
  if (fd_status->isClosed()) {
    errno = EBADF;
    return -1;
  }

  //如果这个fd不是socket或者用户之前已经设置过这个fd是非阻塞的，直接调用原函数即可
  if (!fd_status->isSocket() || fd_status->isUserNonBlock()) {
    return func(fd, std::forward<OriginalFuncParams>(params)...);
  }

  uint64_t timeout{0};
  if (timeout_so == SO_SNDTIMEO) {
    timeout = fd_status->getSendTimeout();
  } else if (timeout_so == SO_RCVTIMEO) {
    timeout = fd_status->getRecvTimeout();
  }

  std::shared_ptr<timer_info> tinfo = std::make_shared<timer_info>();
retry:
  East::Timer::sptr timer{nullptr};

  ssize_t res = func(fd, std::forward<OriginalFuncParams>(params)...);
  while (res == -1 && errno == EINTR) {
    //如果我们的调用被中断了，继续调用
    res = func(fd, std::forward<OriginalFuncParams>(params)...);
  }

  if (res == -1 && errno == EAGAIN) {
    //非阻塞IO，常见资源不可用，所以我们可以通过协程调度，设置一个超时时间，之后再次调用

    auto io_mgr = East::IOManager::GetThis();

    if (timeout != (uint64_t)-1) {
      //如果设置了超时时间，我们就添加一个条件定时器，在超时后取消这个事件
      std::weak_ptr<timer_info> weak_tinfo(tinfo);
      timer = io_mgr->addConditionTimer(
          timeout,
          [io_mgr, weak_tinfo, fd, event] {
            auto shared_tinfo = weak_tinfo.lock();
            if (nullptr == shared_tinfo || shared_tinfo->cancelled) {
              return;
            }
            shared_tinfo->cancelled = ETIMEDOUT;
            if (nullptr != io_mgr) {
              io_mgr->cancelEvent(fd,
                                  static_cast<East::IOManager::Event>(event));
            }
          },
          weak_tinfo);
    }

    //添加对应的事件到队列中去，然后让出执行权，恢复后做检查
    int res = io_mgr->addEvent(fd, static_cast<East::IOManager::Event>(event));

    if (res != 0) {
      //添加事件失败，先取消timer，再返回错误
      if (nullptr != timer) {
        timer->cancel();
      }
      return -1;
    } else {
      //添加成功了， 先让出执行权
      East::Fiber::YieldToHold();

      //恢复后取消定时器，说明没有超时
      if (nullptr != timer) {
        timer->cancel();
      }

      //检查这次resume是否是上面的条件定时器触发的
      if (tinfo->cancelled) {
        errno = tinfo->cancelled;
        return -1;
      }

      //否则重试
      goto retry;  //TODO, why use goto
    }
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
  if (!East::is_hook_enable()) {
    return nanosleep_f(req, rem);
  }

  auto fiber = East::Fiber::GetThis();
  auto io_mgr = East::IOManager::GetThis();
  if (nullptr != fiber && nullptr != io_mgr) {
    io_mgr->addTimer(req->tv_sec * 1000 + req->tv_nsec / 1000000,
                     [io_mgr, fiber]() {
                       if (nullptr != io_mgr) {
                         io_mgr->schedule(fiber);
                       }
                     });
  }
  fiber->yield();
  return 0;
}

int socket(int domain, int type, int protocol) {
  if (!East::is_hook_enable()) {
    return socket_f(domain, type, protocol);
  }

  int fd = socket_f(domain, type, protocol);
  if (fd < 0)
    return fd;

  East::FdMgr::GetInst()->getFd(fd, true);  //放到FdManager中管理，方便后续判断
  return fd;
}

int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {}

int accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
  int fd = do_io(sockfd, accept_f, "accept", East::IOManager::READ, SO_RCVTIMEO,
                 addr, addrlen);

  if (fd >= 0) {
    East::FdMgr::GetInst()->getFd(fd, true);
  }
  return fd;
}

//read
ssize_t read(int fd, void* buf, size_t count) {
  return do_io(fd, read_f, "read", East::IOManager::READ, SO_RCVTIMEO, buf,
               count);
}

ssize_t readv(int fd, const struct iovec* iov, int iovcnt) {
  return do_io(fd, readv_f, "readv", East::IOManager::READ, SO_RCVTIMEO, iov,
               iovcnt);
}

ssize_t recv(int sockfd, void* buf, size_t len, int flags) {
  return do_io(sockfd, recv_f, "recv", East::IOManager::READ, SO_RCVTIMEO, buf,
               len, flags);
}

ssize_t recvfrom(int sockfd, void* buf, size_t len, int flags,
                 struct sockaddr* src_addr, socklen_t* addrlen) {
  return do_io(sockfd, recvfrom_f, "recvfrom", East::IOManager::READ,
               SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr* msg, int flags) {
  return do_io(sockfd, recvmsg_f, "recvmsg", East::IOManager::READ, SO_RCVTIMEO,
               msg, flags);
}

//write
ssize_t write(int fd, const void* buf, size_t count) {
  return do_io(fd, write_f, "write", East::IOManager::WRITE, SO_SNDTIMEO, buf,
               count);
}

ssize_t writev(int fd, const struct iovec* iov, int iovcnt) {
  return do_io(fd, writev_f, "writev", East::IOManager::WRITE, SO_SNDTIMEO, iov,
               iovcnt);
}

ssize_t send(int sockfd, const void* buf, size_t len, int flags) {
  return do_io(sockfd, send_f, "send", East::IOManager::WRITE, SO_SNDTIMEO, buf,
               len, flags);
}

ssize_t sendto(int sockfd, const void* buf, size_t len, int flags,
               const struct sockaddr* dest_addr, socklen_t addrlen) {
  return do_io(sockfd, sendto_f, "sendto", East::IOManager::WRITE, SO_SNDTIMEO,
               buf, len, flags, dest_addr, addrlen);
}

ssize_t sendmsg(int sockfd, const struct msghdr* msg, int flags) {
  return do_io(sockfd, sendmsg_f, "sendmsg", East::IOManager::WRITE,
               SO_SNDTIMEO, msg, flags);
}

int close(int fd) {
  if (!East::is_hook_enable()) {
    return close_f(fd);
  }

  auto fd_status = East::FdMgr::GetInst()->getFd(fd);
  if (nullptr != fd_status) {
    auto io_mgr = East::IOManager::GetThis();
    if (nullptr != io_mgr) {
      io_mgr->cancelAll(fd);
    }
    East::FdMgr::GetInst()->deleteFd(fd);
  }
  return close_f(fd);
}

//socket operations
int fcntl(int fd, int cmd, ... /* arg */) {}

int ioctl(int fd, unsigned long request, ...) {}

int getsockopt(int sockfd, int level, int optname, void* optval,
               socklen_t* optlen) {
  return getsockopt_f(sockfd, level, optname, optval, optlen);
}

int setsockopt(int sockfd, int level, int optname, const void* optval,
               socklen_t optlen) {
  if (!East::is_hook_enable()) {
    return setsockopt_f(sockfd, level, optname, optval, optlen);
  }
}
}
