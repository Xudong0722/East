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
#include <algorithm>
#include <functional>
#include "Elog.h"
#include "Macro.h"

namespace East {

/**
 * @brief 系统日志记录器，用于记录IOManager的运行日志
 */
static East::Logger::sptr g_logger = ELOG_NAME("system");

/**
 * @brief 根据事件类型获取对应的上下文
 * @param event 事件类型（READ或WRITE）
 * @return 对应事件的上下文引用
 * @throws std::invalid_argument 当事件类型无效时抛出异常
 */
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

/**
 * @brief 重置事件上下文，清空所有相关数据
 * @param event_ctx 要重置的事件上下文
 */
void IOManager::FdContext::resetContext(
    IOManager::FdContext::EventContext& event_ctx) {
  event_ctx.cb = nullptr;
  event_ctx.fiber = nullptr;
  event_ctx.scheduler = nullptr;
}

/**
 * @brief 触发指定事件，执行对应的回调函数或协程
 * @param event 要触发的事件类型
 * 
 * 注意：此函数假设事件已经被触发，会从监听列表中移除该事件
 */
void IOManager::FdContext::triggerEvent(IOManager::Event event) {
  EAST_ASSERT(events & event);

  // 事件触发后从监听列表中移除
  events = (Event)(events & ~event);
  EventContext& event_ctx = getContext(event);

  // 优先执行回调函数，如果没有回调函数则执行协程
  if (event_ctx.cb) {
    event_ctx.scheduler->schedule(&event_ctx.cb);
  } else if (event_ctx.fiber) {
    event_ctx.scheduler->schedule(&event_ctx.fiber);
  }

  // 清空上下文信息
  event_ctx.scheduler = nullptr;
  return;
}

/**
 * @brief 构造函数，初始化IO管理器
 * @param threads 工作线程数量
 * @param use_caller 是否使用调用线程作为工作线程
 * @param name 调度器名称
 * 
 * 初始化过程：
 * 1. 创建epoll实例
 * 2. 创建管道用于线程间通信
 * 3. 将管道读端添加到epoll监听
 * 4. 初始化文件描述符上下文数组
 * 5. 启动调度器
 */
IOManager::IOManager(size_t threads, bool use_caller, const std::string& name)
    : Scheduler(threads, use_caller, name) {
  // 创建epoll实例
  m_epfd = epoll_create1(0);
  EAST_ASSERT2(m_epfd != -1, "invalid epoll fd.");

  // 创建管道用于线程间通信
  int res = pipe(m_tickleFds);
  EAST_ASSERT2(res == 0, "pipe failed.");

  // 配置epoll事件，使用边缘触发模式
  epoll_event ep_event{};
  memset(&ep_event, 0, sizeof(ep_event));
  ep_event.events = EPOLLIN | EPOLLET;
  ep_event.data.fd = m_tickleFds[0];

  // 设置管道读端为非阻塞模式
  res = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
  EAST_ASSERT2(res == 0, "set pipe-0 non block failed.");

  // 将管道读端添加到epoll监听
  res = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &ep_event);
  EAST_ASSERT2(res == 0, "epoll ctl failed.");

  // 初始化文件描述符上下文数组
  contextResize(32);

  // 启动调度器
  start();
}

/**
 * @brief 析构函数，清理所有资源
 * 
 * 清理过程：
 * 1. 停止调度器
 * 2. 关闭epoll文件描述符
 * 3. 关闭管道文件描述符
 * 4. 释放所有文件描述符上下文
 */
IOManager::~IOManager() {
  stop();
  close(m_epfd);
  close(m_tickleFds[0]);
  close(m_tickleFds[1]);
  m_epfd = m_tickleFds[0] = m_tickleFds[1] = -1;

  // 释放所有文件描述符上下文
  for (size_t i = 0; i < m_fdContexts.size(); ++i) {
    delete m_fdContexts[i];
  }
}

/**
 * @brief 添加IO事件监听
 * @param fd 文件描述符
 * @param event 要监听的事件类型
 * @param cb 事件触发时的回调函数
 * @return 成功返回0，失败返回-1
 * 
 * 实现逻辑：
 * 1. 获取或创建文件描述符上下文
 * 2. 检查是否已经监听该事件类型
 * 3. 配置epoll事件并添加到监听列表
 * 4. 设置事件上下文（调度器、协程或回调函数）
 */
int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
  FdContext* fd_ctx{nullptr};

  // 首先尝试读锁获取文件描述符上下文
  RWMutexType::RLockGuard lock(m_mutex);
  if ((int)m_fdContexts.size() > fd) {
    fd_ctx = m_fdContexts[fd];
    lock.unlock();
  } else {
    // 如果数组大小不够，需要扩容
    lock.unlock();
    RWMutexType::WLockGuard lock1(m_mutex);
    SafeContextResize(fd * 1.5);
    fd_ctx = m_fdContexts[fd];
  }

  ELOG_DEBUG(g_logger) << "epfd: " << m_epfd << ", fd: " << fd
                       << ", event: " << event
                       << ", fd event: " << fd_ctx->events;

  // 对文件描述符上下文加锁
  FdContext::MutextType::LockGuard lock2(fd_ctx->mutex);

  // 检查是否已经监听该事件类型
  if (fd_ctx->events & event) {
    ELOG_ERROR(g_logger) << "Assert- addEvent, fd: " << fd
                         << ", event: " << event
                         << ", fd event: " << fd_ctx->events;
    return -1;
  }

  // 配置epoll事件
  epoll_event ep_event{};
  memset(&ep_event, 0, sizeof(ep_event));
  ep_event.events = EPOLLET | fd_ctx->events | event;
  ep_event.data.ptr = fd_ctx;

  // 根据是否已有事件决定操作类型
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

  // 更新事件计数和监听事件类型
  ++m_pendingEventCount;
  fd_ctx->events = static_cast<Event>(fd_ctx->events | event);

  // 设置事件上下文
  FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
  EAST_ASSERT(!event_ctx.cb && !event_ctx.fiber && !event_ctx.scheduler);
  event_ctx.scheduler = Scheduler::GetThis();

  if (nullptr != cb) {
    event_ctx.cb.swap(cb);
  } else {
    event_ctx.fiber = Fiber::GetThis();  // TODO: 使用当前协程
  }

  ELOG_DEBUG(g_logger) << __FUNCTION__ << "epfd: " << m_epfd << ", fd: " << fd
                       << ", event: " << event
                       << ", fd event: " << fd_ctx->events;
  return 0;
}

/**
 * @brief 删除指定的事件监听
 * @param fd 文件描述符
 * @param event 要删除的事件类型
 * @return 成功返回true，失败返回false
 * 
 * 实现逻辑：
 * 1. 检查文件描述符是否有效
 * 2. 验证是否正在监听该事件
 * 3. 从epoll中移除或修改事件监听
 * 4. 更新事件计数和上下文
 */
bool IOManager::removeEvent(int fd, Event event) {
  if (fd >= (int)m_fdContexts.size())
    return false;

  FdContext* fd_ctx{nullptr};
  {
    RWMutexType::RLockGuard lock(m_mutex);
    fd_ctx = m_fdContexts[fd];
  }

  FdContext::MutextType::LockGuard lock(fd_ctx->mutex);

  // 检查是否正在监听该事件
  if ((fd_ctx->events & event) == 0) {
    ELOG_DEBUG(g_logger) << "This fd " << fd << " doesn't have this event "
                         << event;
    return false;
  }

  // 配置epoll事件，移除指定事件类型
  epoll_event ep_event{};
  memset(&ep_event, 0, sizeof(ep_event));
  ep_event.events = EPOLLET | (fd_ctx->events & (~event));
  ep_event.data.ptr = fd_ctx;

  // 根据剩余事件决定操作类型
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

  // 更新事件计数和监听事件类型
  --m_pendingEventCount;
  fd_ctx->events = static_cast<Event>(fd_ctx->events & (~event));

  // 重置事件上下文
  FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
  fd_ctx->resetContext(event_ctx);
  return true;
}

/**
 * @brief 取消指定事件并触发回调
 * @param fd 文件描述符
 * @param event 要取消的事件类型
 * @return 成功返回true，失败返回false
 * 
 * 与removeEvent的区别：此函数会触发事件回调，然后移除监听
 */
bool IOManager::cancelEvent(int fd, Event event) {
  if (fd >= (int)m_fdContexts.size())
    return false;

  FdContext* fd_ctx{nullptr};
  {
    RWMutexType::RLockGuard lock(m_mutex);
    fd_ctx = m_fdContexts[fd];
  }

  ELOG_DEBUG(g_logger) << "epfd: " << m_epfd << ", fd: " << fd
                       << ", event: " << event
                       << ", fd event: " << fd_ctx->events;

  FdContext::MutextType::LockGuard lock(fd_ctx->mutex);

  // 检查是否正在监听该事件
  if ((fd_ctx->events & event) == 0) {
    ELOG_DEBUG(g_logger) << "This fd " << fd << " doesn't have this event "
                         << event;
    return false;
  }

  // 配置epoll事件，移除指定事件类型
  epoll_event ep_event{};
  memset(&ep_event, 0, sizeof(ep_event));
  ep_event.events = EPOLLET | (fd_ctx->events & (~event));
  ep_event.data.ptr = fd_ctx;

  // 根据剩余事件决定操作类型
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

  // 触发事件回调，然后移除监听
  fd_ctx->triggerEvent(event);
  --m_pendingEventCount;

  return true;
}

/**
 * @brief 取消文件描述符上的所有事件
 * @param fd 文件描述符
 * @return 成功返回true，失败返回false
 * 
 * 实现逻辑：
 * 1. 从epoll中完全移除该文件描述符
 * 2. 分别触发读事件和写事件（如果存在）
 * 3. 更新事件计数
 */
bool IOManager::cancelAll(int fd) {
  if (fd >= (int)m_fdContexts.size())
    return false;

  FdContext* fd_ctx{nullptr};
  {
    RWMutexType::RLockGuard lock(m_mutex);
    fd_ctx = m_fdContexts[fd];
  }

  FdContext::MutextType::LockGuard lock(fd_ctx->mutex);

  // 检查是否有事件在监听
  if (fd_ctx->events == 0) {
    ELOG_DEBUG(g_logger) << "This fd " << fd << " doesn't have event ";
    return false;
  }

  // 从epoll中完全移除该文件描述符
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

  // 分别触发读事件和写事件（如果存在）
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

/**
 * @brief 调整文件描述符上下文数组大小
 * @param sz 新的数组大小
 * 
 * 注意：此函数主要用于初始化，后续扩容应使用SafeContextResize
 */
void IOManager::contextResize(size_t sz) {
  m_fdContexts.resize(sz);

  // 为新的数组位置创建文件描述符上下文
  for (size_t i = 0; i < m_fdContexts.size(); ++i) {
    if (nullptr == m_fdContexts[i]) {
      m_fdContexts[i] = new (std::nothrow) FdContext;
      m_fdContexts[i]->fd = i;
    }
  }
}

/**
 * @brief 安全地调整文件描述符上下文数组大小
 * @param sz 新的数组大小
 * 
 * 实现逻辑：
 * 1. 如果新大小小于等于当前大小，直接调整
 * 2. 否则创建临时数组，复制现有数据
 * 3. 为新的数组位置创建文件描述符上下文
 */
void IOManager::SafeContextResize(size_t sz) {
  if (sz <= m_fdContexts.size()) {
    m_fdContexts.resize(sz);
    return;
  }

  // 创建临时数组并复制现有数据
  auto tmp = m_fdContexts;
  tmp.resize(sz);
  copy(m_fdContexts.begin(), m_fdContexts.end(), tmp.begin());
  m_fdContexts.swap(tmp);

  // 为新的数组位置创建文件描述符上下文
  for (size_t i = 0; i < m_fdContexts.size(); ++i) {
    if (nullptr == m_fdContexts[i]) {
      m_fdContexts[i] = new (std::nothrow) FdContext;
      m_fdContexts[i]->fd = i;
    }
  }
}

/**
 * @brief 获取当前线程的IOManager实例
 * @return 当前线程的IOManager指针，如果不存在则返回nullptr
 */
IOManager* IOManager::GetThis() {
  return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

/**
 * @brief 检查是否正在停止，并获取下一个定时器的超时时间
 * @param time_out 输出参数，下一个定时器的超时时间
 * @return 如果正在停止返回true，否则返回false
 */
bool IOManager::stopping(uint64_t& time_out) {
  time_out = getNextTimer();
  return time_out == ~0ull && m_pendingEventCount == 0 && Scheduler::stopping();
}

/**
 * @brief 唤醒空闲线程
 * 
 * 通过管道向epoll写入数据来唤醒等待的线程
 */
void IOManager::tickle() {
  if (!hasIdleThreads())  // 如果没有空闲的线程，直接返回，没必要唤醒
    return;

  // 约定：m_tickleFds[0]是读端，m_tickleFds[1]是发送端
  // 在这里写入数据，其他线程在epoll_wait就可以读取到事件，从而达到唤醒的目的
  int cnt = write(m_tickleFds[1], "t", 1);
  EAST_ASSERT2(cnt == 1, "tickle pipe failed");
}

/**
 * @brief 空闲状态处理，主要的IO事件循环
 * 
 * 实现逻辑：
 * 1. 循环处理epoll事件和定时器
 * 2. 使用epoll_wait等待IO事件
 * 3. 处理超时的定时器
 * 4. 处理IO事件，触发对应的回调或协程
 * 5. 协程切换和调度
 */
void IOManager::idle() {
  ELOG_DEBUG(g_logger) << "idle";
  constexpr uint32_t MAX_EVENTS = 256;  // 一次epoll_wait最多处理的事件数

  // 使用智能指针管理epoll_event数组，避免内存泄漏
  std::unique_ptr<epoll_event[]> ep_events(new epoll_event[MAX_EVENTS]);

  while (true) {
    uint64_t next_timeout{0};

    // 获取下一个定时器的超时时间，同时返回IOManager是否正在停止
    if (stopping(next_timeout)) {
      ELOG_DEBUG(g_logger) << "IOManager stopping";
      break;
    }

    int res{0};
    do {
      constexpr int MAX_TIMEOUT = 3000;

      // 看看现在最靠前的定时器是否小于这个超时时间，取较小的一个
      if (next_timeout != ~0ull)
        next_timeout = std::min(MAX_TIMEOUT, (int)next_timeout);
      else
        next_timeout = MAX_EVENTS;

      ELOG_DEBUG(g_logger) << "epoll wait, timeout: " << next_timeout;
      res = epoll_wait(m_epfd, ep_events.get(), MAX_EVENTS, (int)next_timeout);

      if (res < 0 && errno == EINTR) {
        continue;
      } else {
        break;
      }
    } while (true);

    // 处理超时的定时器
    std::vector<std::function<void()>> timer_cbs{};
    listExpiredCb(timer_cbs);  // 获取所有已经超时的定时器的回调函数
    if (!timer_cbs.empty()) {
      schedule(timer_cbs.begin(),
               timer_cbs.end());  // 将符合条件的timer的回调放进去
      timer_cbs.clear();
    }

    ELOG_DEBUG(g_logger) << "idle: epoll wait, res: " << res;

    // 处理epoll事件
    for (int i = 0; i < res; ++i) {
      epoll_event& event = ep_events[i];

      // 处理管道唤醒事件
      if (event.data.fd == m_tickleFds[0]) {
        uint8_t dummy{};
        // 如果是被tickle唤醒的，将所有的数据全都读取出来
        while (read(event.data.fd, &dummy, 1) > 0)
          ;
        continue;
      }

      // 处理文件描述符的IO事件
      FdContext* fd_ctx = static_cast<FdContext*>(event.data.ptr);

      // 对fd_ctx加锁，防止其他线程在epoll_wait期间修改fd_ctx的状态
      FdContext::MutextType::LockGuard lock(fd_ctx->mutex);

      ELOG_DEBUG(g_logger) << "epoll wait, event:" << event.events;

      // 处理错误和挂起事件
      if (event.events & (EPOLLERR | EPOLLHUP)) {
        event.events |= EPOLLIN | EPOLLOUT;
      }

      // 确定实际触发的事件类型
      int real_events{NONE};
      if (event.events & EPOLLIN) {
        real_events |= READ;
      }
      if (event.events & EPOLLOUT) {
        real_events |= WRITE;
      }

      // 计算剩余未触发的事件
      int left_events = (~real_events) & fd_ctx->events;
      int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

      ELOG_DEBUG(g_logger) << __FUNCTION__ << ", fd: " << event.data.fd
                           << ", fd ctx's fd: " << fd_ctx->fd
                           << ", fd ctx's events: " << fd_ctx->events
                           << ", event: " << event.events
                           << ", left_events: " << left_events << ", op: " << op
                           << ", real_events: " << real_events;

      // 重新配置epoll事件，使用边缘触发模式
      event.events = left_events | EPOLLET;

      // 将没有处理完的事件继续放进去或者是处理完了就删除掉不再监听
      int res2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
      if (res2 != 0) {
        ELOG_ERROR(g_logger)
            << "epoll_ctl failed, ep fd: " << m_epfd << ", op: " << op
            << ", fd: " << fd_ctx->fd << ", fd events: " << fd_ctx->events
            << ", errno: " << errno << ", strerrno: " << strerror(errno);
        continue;
      }

      // 触发读事件，会将回调函数放入调度器的任务队列中
      if (real_events & READ) {
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
      }

      // 触发写事件，会将回调函数放入调度器的任务队列中
      if (real_events & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
      }
    }

    // 协程切换和调度
    auto cur_fiber = Fiber::GetThis();
    auto raw_ptr = cur_fiber.get();
    cur_fiber.reset();

    ELOG_DEBUG(g_logger) << "ready to yield, cur fiber id: " << raw_ptr->getId()
                         << ", cur fiber state: " << raw_ptr->getState();

    // 将当前协程切换到后台执行，进入调度器协程，开始执行任务
    raw_ptr->yield();
  }
}

/**
 * @brief 检查调度器是否正在停止
 * @return 如果正在停止返回true，否则返回false
 * 
 * 停止条件：scheduler结束 && 没有要执行的事件 && 没有要执行的定时器
 */
bool IOManager::stopping() {
  uint64_t time_out{0};
  return stopping(time_out);
}

/**
 * @brief 当定时器插入到队列前端时的回调
 * 
 * 唤醒空闲线程以处理新插入的定时器
 */
void IOManager::onTimerInsertAtFront() {
  tickle();
}

};  // namespace East