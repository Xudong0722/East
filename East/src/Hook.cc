/*
 * @Author: Xudong0722 
 * @Date: 2025-04-08 23:00:31 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-04-14 23:01:49
 */

#include "Hook.h"
#include <dlfcn.h>
#include "Config.h"
#include "FdManager.h"
#include "Fiber.h"
#include "IOManager.h"

namespace East {
static thread_local bool t_hook_enable = false;
static East::Logger::sptr g_logger = ELOG_NAME("system");

static East::ConfigVar<int>::sptr g_tcp_connect_timeout =
    East::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

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

static uint64_t s_connect_timeout = -1;
struct _HookIniter {
  _HookIniter() {
    hook_init();
    s_connect_timeout = g_tcp_connect_timeout->getValue();
    g_tcp_connect_timeout->addListener(
        [](const auto& old_v, const auto& new_v) {
          ELOG_INFO(g_logger) << "tcp connect timeout changed from " << old_v
                              << " to " << new_v;
          s_connect_timeout = new_v;
        });
  }
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

      ELOG_ERROR(g_logger) << hook_func_name << " addEvent(" << fd << ", "
                           << event << ")";
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

  // auto func = std::bind(
  //   (void (East::IOManager::*)(East::Fiber::sptr, int thread_id))(&East::IOManager::schedule),
  //   io_mgr, fiber, -1);

  if (nullptr != fiber && nullptr != io_mgr) {
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

int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen,
                         uint64_t timeout) {
  ELOG_DEBUG(East::g_logger)
      << "connect_with_timeout fd: " << fd << ", timeout: " << timeout
      << ", is current thread hook: " << East::is_hook_enable();
  if (!East::is_hook_enable()) {
    return connect_f(fd, addr, addrlen);
  }

  auto fd_status = East::FdMgr::GetInst()->getFd(fd);
  if (nullptr == fd_status || fd_status->isClosed()) {
    errno = EBADF;
    return -1;
  }

  if (!fd_status->isSocket() || fd_status->isUserNonBlock()) {
    return connect_f(fd, addr, addrlen);
  }

  int res = connect_f(fd, addr, addrlen);
  ELOG_DEBUG(East::g_logger)
      << "connect_f fd: " << fd << ", res: " << res << ", errno: " << errno;
  if (!res)
    return res;
  else if (res != -1 || errno != EINPROGRESS) {
    return res;
  }

  auto io_mgr = East::IOManager::GetThis();
  East::Timer::sptr timer{nullptr};
  std::shared_ptr<East::timer_info> tinfo =
      std::make_shared<East::timer_info>();
  std::weak_ptr<East::timer_info> weak_tinfo(tinfo);

  if (timeout != static_cast<uint64_t>(-1)) {
    timer = io_mgr->addConditionTimer(
        timeout,
        [io_mgr, weak_tinfo, fd] {
          auto shared_tinfo = weak_tinfo.lock();
          if (nullptr == shared_tinfo || shared_tinfo->cancelled) {
            return;
          }
          shared_tinfo->cancelled = ETIMEDOUT;
          if (nullptr != io_mgr) {
            io_mgr->cancelEvent(fd, East::IOManager::WRITE);
          }
        },
        weak_tinfo);
  }

  int ret = io_mgr->addEvent(fd, East::IOManager::WRITE);

  if (ret != 0) {
    ELOG_ERROR(East::g_logger) << "connect_with_timeout addEvent(" << fd << ", "
                               << East::IOManager::WRITE << ")";
    if (nullptr != timer) {
      timer->cancel();
    }
    return -1;
  } else {
    East::Fiber::GetThis()->yield();
    if (nullptr != timer) {
      timer->cancel();
    }
    if (tinfo->cancelled) {
      errno = tinfo->cancelled;
      return -1;
    }
  }

  int error{0};
  socklen_t len = sizeof(int);
  if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)) {
    return -1;
  }

  ELOG_DEBUG(East::g_logger)
      << "connect_with_timeout getsockopt fd: " << fd << ", error: " << error;

  if (error) {
    errno = error;
    return -1;
  }
  return 0;
}

int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
  return connect_with_timeout(sockfd, addr, addrlen,
                              East::g_tcp_connect_timeout->getValue());
}

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
int fcntl(int fd, int cmd, ... /* arg */) {

  va_list ap;
  va_start(ap, cmd);
  switch (cmd) {
    case F_SETFL: {
      int arg = va_arg(ap, int);
      va_end(ap);
      //因为就算用户没有设置为nonblock(user)，但是我们下面是会使用nonblock(system)的，所以我们这里要调整下入参
      auto fd_status = East::FdMgr::GetInst()->getFd(fd);
      if (nullptr == fd_status || fd_status->isClosed() ||
          !fd_status->isSocket()) {
        return fcntl_f(fd, cmd, arg);
      }

      fd_status->setUserNonBlock(arg & O_NONBLOCK);
      if (fd_status->isSysNonBlock()) {
        arg |= O_NONBLOCK;
      } else {
        arg &= ~O_NONBLOCK;
      }
      return fcntl_f(fd, cmd, arg);
    }
    case F_GETFL: {
      va_end(ap);
      int arg = fcntl_f(fd, cmd);
      auto fd_status = East::FdMgr::GetInst()->getFd(fd);
      if (nullptr == fd_status || fd_status->isClosed() ||
          !fd_status->isSocket()) {
        return arg;
      }
      if (fd_status->isUserNonBlock()) {
        arg |= O_NONBLOCK;
      } else {
        arg &= ~O_NONBLOCK;
      }
      return arg;
    }
    case F_DUPFD:
    case F_DUPFD_CLOEXEC:
    case F_SETFD:
    case F_SETOWN:
    case F_SETSIG:
    case F_SETLEASE:
    case F_NOTIFY:
    case F_SETPIPE_SZ: {
      int arg = va_arg(ap, int);
      va_end(ap);
      return fcntl_f(fd, cmd, arg);
    }
    case F_GETFD:
    case F_GETOWN:
    case F_GETSIG:
    case F_GETLEASE:
    case F_GETPIPE_SZ: {
      va_end(ap);
      return fcntl_f(fd, cmd);
    }
    case F_SETLK:
    case F_SETLKW:
    case F_GETLK: {
      struct flock* arg = va_arg(ap, struct flock*);
      va_end(ap);
      return fcntl_f(fd, cmd, arg);
    }
    case F_GETOWN_EX:
    case F_SETOWN_EX: {
      struct f_owner_ex* arg = va_arg(ap, struct f_owner_ex*);
      va_end(ap);
      return fcntl_f(fd, cmd, arg);
    }
    default: {
      va_end(ap);
      return fcntl_f(fd, cmd);
    }
  }

  return fcntl_f(fd, cmd);
}

int ioctl(int fd, unsigned long request, ...) {
  //TODO

  va_list ap;
  va_start(ap, request);
  void* arg = va_arg(ap, void*);
  va_end(ap);

  if (FIONBIO == request) {  //也用于设置fd的非阻塞模式
    bool user_nonblock = !!(*static_cast<int*>(arg));
    auto fd_status = East::FdMgr::GetInst()->getFd(fd);
    if (nullptr == fd_status || fd_status->isClosed() ||
        !fd_status->isSocket()) {
      return ioctl_f(fd, request, arg);
    }
    fd_status->setUserNonBlock(user_nonblock);
  }
  return ioctl_f(fd, request, arg);
}

int getsockopt(int sockfd, int level, int optname, void* optval,
               socklen_t* optlen) {
  return getsockopt_f(sockfd, level, optname, optval, optlen);
}

int setsockopt(int sockfd, int level, int optname, const void* optval,
               socklen_t optlen) {
  if (!East::is_hook_enable()) {
    return setsockopt_f(sockfd, level, optname, optval, optlen);
  }

  if (level == SOL_SOCKET) {
    if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
      auto fd_status = East::FdMgr::GetInst()->getFd(sockfd);
      if (nullptr != fd_status) {
        const timeval* t = static_cast<const timeval*>(optval);
        if (nullptr != t) {
          if (optname == SO_RCVTIMEO) {
            fd_status->setRecvTimeout(t->tv_sec * 1000 + t->tv_usec / 1000);
          } else if (optname == SO_SNDTIMEO) {
            fd_status->setSendTimeout(t->tv_sec * 1000 + t->tv_usec / 1000);
          }
        }
      }
    }
  }
  return setsockopt_f(sockfd, level, optname, optval, optlen);
}
}
