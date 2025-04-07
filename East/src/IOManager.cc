/*
 * @Author: Xudong0722 
 * @Date: 2025-04-01 22:55:58 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-04-07 14:31:15
 */

#include "IOManager.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <functional>
#include "Elog.h"
#include "Macro.h"

namespace East {
static East::Logger::sptr g_logger = ELOG_NAME("system");

IOManager::FdContext::EventContext& IOManager::FdContext::getContext(
    IOManager::Event event) {
  switch (event) {
    case READ:
      return read;
    case WRITE:
      return write;
    default:
      EAST_ASSERT2(false, "get context invalid");
  }
  throw std::invalid_argument("getContext invalid event type");
}

void IOManager::FdContext::resetContext(
    IOManager::FdContext::EventContext& event_ctx) {
  event_ctx.cb = nullptr;
  event_ctx.fiber = nullptr;
  event_ctx.scheduler = nullptr;
}

void IOManager::FdContext::triggerEvent(IOManager::Event event) {
  EAST_ASSERT(events & event);

  EventContext& event_ctx = getContext(event);
  if (event_ctx.cb) {
    event_ctx.scheduler->schedule(&event_ctx.cb);
  } else if (event_ctx.fiber) {
    event_ctx.scheduler->schedule(&event_ctx.fiber);
  }
  event_ctx.scheduler = nullptr;
  return;
}

IOManager::IOManager(size_t threads, bool use_caller, const std::string& name)
    : Scheduler(threads, use_caller, name) {
  m_epfd = epoll_create1(0);
  EAST_ASSERT2(m_epfd != -1, "invalid epoll fd.");

  int res = pipe(m_tickleFds);
  //ELOG_DEBUG(g_logger) << __FUNCTION__ << ", tickle fd: " << m_tickleFds[0] << ", " << m_tickleFds[1];
  EAST_ASSERT2(res == 0, "pipe failed.");

  epoll_event ep_event{};
  memset(&ep_event, 0, sizeof(ep_event));
  ep_event.events = EPOLLIN | EPOLLET;  //use ET mode
  ep_event.data.fd = m_tickleFds[0];

  res = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
  EAST_ASSERT2(res == 0, "set pipe-0 non block failed.");

  res = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &ep_event);
  EAST_ASSERT2(res == 0, "epoll ctl failed.");

  contextResize(32);

  start();
}

IOManager::~IOManager() {
  stop();
  close(m_epfd);
  close(m_tickleFds[0]);
  close(m_tickleFds[1]);
  m_epfd = m_tickleFds[0] = m_tickleFds[1] = -1;
  for (size_t i = 0; i < m_fdContexts.size(); ++i) {
    delete m_fdContexts[i];
  }
}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
  FdContext* fd_ctx{nullptr};
  RWMutexType::RLockGuard lock(m_mutex);
  if ((int)m_fdContexts.size() > fd) {
    fd_ctx = m_fdContexts[fd];
    lock.unlock();
  } else {
    lock.unlock();
    RWMutexType::WLockGuard lock1(m_mutex);
    contextResize(fd * 1.5);
    fd_ctx = m_fdContexts[fd];
  }
  //ELOG_INFO(g_logger) << "epfd: " << m_epfd << ", fd: " << fd <<", event: " << event <<", fd event: " << fd_ctx->events;
  FdContext::MutextType::LockGuard lock2(fd_ctx->mutex);
  if (fd_ctx->events & event) {  //如果该fd已经添加过此类型时间的话
    ELOG_ERROR(g_logger) << "Assert- addEvent, fd: " << fd
                         << ", event: " << event
                         << ", fd event: " << fd_ctx->events;
    return -1;
  }

  epoll_event ep_event{};
  memset(&ep_event, 0, sizeof(ep_event));
  ep_event.events = EPOLLET | fd_ctx->events | event;
  ep_event.data.ptr = fd_ctx;

  int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
  int res = epoll_ctl(m_epfd, op, fd, &ep_event);

  if (res != 0) {
    ELOG_ERROR(g_logger) << "epoll_ctl failed, ep fd: " << m_epfd
                         << ", op: " << op << ", event: " << event
                         << ", fd events: " << fd_ctx->events
                         << ", errno: " << errno
                         << ", strerrno: " << strerror(errno);
    return -1;
  }

  ++m_pendingEventCount;
  fd_ctx->events = static_cast<Event>(fd_ctx->events | event);
  FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
  EAST_ASSERT(!event_ctx.cb && !event_ctx.fiber && !event_ctx.scheduler);
  event_ctx.scheduler = Scheduler::GetThis();

  if (nullptr != cb) {
    event_ctx.cb.swap(cb);
  } else {
    event_ctx.fiber = Fiber::GetThis();  //TODO
  }
  //ELOG_INFO(g_logger) << __FUNCTION__ << "epfd: " << m_epfd << ", fd: " << fd <<", event: " << event <<", fd event: " << fd_ctx->events;
  return 0;
}

bool IOManager::removeEvent(int fd, Event event) {
  if (fd >= (int)m_fdContexts.size())
    return false;

  FdContext* fd_ctx{nullptr};
  {
    RWMutexType::RLockGuard lock(m_mutex);
    fd_ctx = m_fdContexts[fd];
  }

  FdContext::MutextType::LockGuard lock(fd_ctx->mutex);
  if ((fd_ctx->events & event) == 0) {
    ELOG_INFO(g_logger) << "This fd " << fd << " doesn't have this event "
                        << event;
    return false;
  }

  epoll_event ep_event{};
  memset(&ep_event, 0, sizeof(ep_event));
  ep_event.events = EPOLLET | (fd_ctx->events & (~event));
  ep_event.data.ptr = fd_ctx;

  int op = (fd_ctx->events & (~event)) ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
  int res = epoll_ctl(m_epfd, op, fd, &ep_event);

  if (res != 0) {
    ELOG_ERROR(g_logger) << "epoll_ctl failed, ep fd: " << m_epfd
                         << ", op: " << op << ", event: " << event
                         << ", fd events: " << fd_ctx->events
                         << ", errno: " << errno
                         << ", strerrno: " << strerror(errno);
    return false;
  }

  --m_pendingEventCount;
  fd_ctx->events = static_cast<Event>(fd_ctx->events & (~event));
  FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
  fd_ctx->resetContext(event_ctx);
  return true;
}

bool IOManager::cancelEvent(int fd, Event event) {
  if (fd >= (int)m_fdContexts.size())
    return false;

  FdContext* fd_ctx{nullptr};
  {
    RWMutexType::RLockGuard lock(m_mutex);
    fd_ctx = m_fdContexts[fd];
  }
  //ELOG_INFO(g_logger) << "epfd: " << m_epfd << ", fd: " << fd <<", event: " << event <<", fd event: " << fd_ctx->events;
  FdContext::MutextType::LockGuard lock(fd_ctx->mutex);
  if ((fd_ctx->events & event) == 0) {
    ELOG_INFO(g_logger) << "This fd " << fd << " doesn't have this event "
                        << event;
    return false;
  }

  epoll_event ep_event{};
  memset(&ep_event, 0, sizeof(ep_event));
  ep_event.events = EPOLLET | (fd_ctx->events & (~event));
  ep_event.data.ptr = fd_ctx;

  int op = (fd_ctx->events & (~event)) ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
  int res = epoll_ctl(m_epfd, op, fd, &ep_event);

  if (res != 0) {
    ELOG_ERROR(g_logger) << "epoll_ctl failed, ep fd: " << m_epfd
                         << ", op: " << op << ", event: " << event
                         << ", fd: " << fd_ctx->fd
                         << ", fd events: " << fd_ctx->events
                         << ", errno: " << errno
                         << ", strerrno: " << strerror(errno);
    return false;
  }

  fd_ctx->triggerEvent(event);
  --m_pendingEventCount;

  //ELOG_DEBUG(g_logger) << __FUNCTION__ << ", epoll ctl res: " << res;
  return true;
}

bool IOManager::cancelAll(int fd) {
  if (fd >= (int)m_fdContexts.size())
    return false;

  FdContext* fd_ctx{nullptr};
  {
    RWMutexType::RLockGuard lock(m_mutex);
    fd_ctx = m_fdContexts[fd];
  }

  FdContext::MutextType::LockGuard lock(fd_ctx->mutex);
  if (fd_ctx->events == 0) {
    ELOG_INFO(g_logger) << "This fd " << fd << " doesn't have event ";
    return false;
  }

  epoll_event ep_event{};
  memset(&ep_event, 0, sizeof(ep_event));
  ep_event.events = 0;
  ep_event.data.ptr = fd_ctx;

  int res = epoll_ctl(m_epfd, EPOLL_CTL_DEL, fd, &ep_event);

  if (res != 0) {
    ELOG_ERROR(g_logger) << "epoll_ctl failed, ep fd: " << m_epfd
                         << ", op: " << EPOLL_CTL_DEL << ", fd: " << fd
                         << ", fd events: " << fd_ctx->events
                         << ", errno: " << errno
                         << ", strerrno: " << strerror(errno);
    return false;
  }

  if (fd_ctx->events & READ) {
    fd_ctx->triggerEvent(READ);
    --m_pendingEventCount;
  }

  if (fd_ctx->events & WRITE) {
    fd_ctx->triggerEvent(WRITE);
    --m_pendingEventCount;
  }
  return true;
}

void IOManager::contextResize(size_t sz) {
  m_fdContexts.resize(sz);

  for (size_t i = 0; i < m_fdContexts.size(); ++i) {
    if (nullptr == m_fdContexts[i]) {
      m_fdContexts[i] = new FdContext;
      m_fdContexts[i]->fd = i;
    }
  }
}

IOManager* IOManager::GetThis() {
  return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

bool IOManager::stopping(uint64_t& time_out) {
  time_out = getNextTimer();
  return time_out == ~0ull && m_pendingEventCount == 0 && Scheduler::stopping();
}

void IOManager::tickle() {
  if (!hasIdleThreads())
    return;

  int cnt = write(m_tickleFds[1], "t", 1);
  EAST_ASSERT2(cnt == 1, "tickle pipe failed");
}

void IOManager::idle() {
  ELOG_INFO(g_logger) << "idle";
  constexpr uint32_t MAX_EVENTS = 256;
  std::unique_ptr<epoll_event[]> ep_events(new epoll_event[MAX_EVENTS]);

  while (true) {
    uint64_t next_timeout{0};
    if (stopping(next_timeout)) {

      ELOG_INFO(g_logger) << "IOManager stopping";
      break;
    }

    int res{0};
    do {
      constexpr int MAX_TIMEOUT = 3000;
      //看看现在最靠前的定时器是否小于这个超时时间，取较小的一个
      if (next_timeout != ~0ull)
        next_timeout = std::min(MAX_TIMEOUT, (int)next_timeout);
      else
        next_timeout = MAX_EVENTS;

      res = epoll_wait(m_epfd, ep_events.get(), MAX_EVENTS, (int)next_timeout);

      if (res < 0 && errno == EINTR) {
        continue;
      } else {
        break;
      }
    } while (true);

    std::vector<std::function<void()>> timer_cbs{};
    listExpiredCb(timer_cbs);
    schedule(timer_cbs.begin(),
             timer_cbs.end());  //将符合条件的timer的回调放进去
    //ELOG_DEBUG(g_logger) << "idle: epoll wait, res: " << res;
    for (int i = 0; i < res; ++i) {
      epoll_event& event = ep_events[i];
      if (event.data.fd == m_tickleFds[0]) {
        uint8_t dummy{};
        while (read(event.data.fd, &dummy, 1) > 0)
          ;
        continue;
      }

      FdContext* fd_ctx = static_cast<FdContext*>(event.data.ptr);
      FdContext::MutextType::LockGuard lock(fd_ctx->mutex);
      if (event.events & (EPOLLERR | EPOLLHUP)) {
        event.events |= EPOLLIN | EPOLLOUT;
      }
      int real_events{NONE};
      if (event.events & EPOLLIN) {
        real_events |= READ;
      }
      if (event.events & EPOLLOUT) {
        real_events |= WRITE;
      }

      int left_events =
          (~real_events) & fd_ctx->events;  //这次没有触发的事件之后继续触发
      int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
      event.events = left_events;
      //   ELOG_DEBUG(g_logger) << __FUNCTION__ << ", fd: " << event.data.fd
      //     << ", fd ctx's fd: " << fd_ctx->fd
      //     << ", fd ctx's events: " << fd_ctx->events
      //     << ", event: " << event.events
      //     << ", left_events: " << left_events << ", op: " << op;
      int res2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
      if (res2 != 0) {
        ELOG_ERROR(g_logger)
            << "epoll_ctl failed, ep fd: " << m_epfd << ", op: " << op
            << ", fd: " << fd_ctx->fd << ", fd events: " << fd_ctx->events
            << ", errno: " << errno << ", strerrno: " << strerror(errno);
        continue;
      }

      if (real_events & READ) {
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
      }
      if (real_events & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
      }
    }
    auto cur_fiber = Fiber::GetThis();
    auto raw_ptr = cur_fiber.get();
    cur_fiber.reset();
    raw_ptr->yield();
  }
}

bool IOManager::stopping() {
  //结束：scheduler结束 && 没有要执行的事件 && 没有要执行的定时器
  //return Scheduler::stopping() && m_pendingEventCount == 0 && !hasTimer();
  uint64_t time_out{0};
  return stopping(time_out);
}

void IOManager::onTimerInsertAtFront() {
  tickle();
}
};  // namespace East