/*
 * @Author: Xudong0722 
 * @Date: 2025-04-01 22:55:54 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-04-07 14:16:16
 */

#pragma once
#include "Scheduler.h"
#include "Timer.h"

namespace East {
class IOManager : public Scheduler, public TimerManager {
 public:
  using sptr = std::shared_ptr<IOManager>;
  using RWMutexType = RWLock;

  enum Event {
    NONE = 0x0,
    READ = 0x1,   //EPOLL_EVENTS::EPOLLIN
    WRITE = 0x4,  //EPOLL_EVENTS::EPOLLOUT
  };

 private:
  struct FdContext {
    using MutextType = Mutex;
    struct EventContext {
      Scheduler* scheduler{nullptr};  //事件执行所属的调度器
      Fiber::sptr fiber{nullptr};     //事件执行所属的协程
      std::function<void()> cb;       //事件执行回调函数
    };

    EventContext& getContext(Event event);
    void resetContext(EventContext& event_ctx);
    void triggerEvent(Event event);

    int fd;
    EventContext read;
    EventContext write;
    Event events{NONE};
    MutexType mutex;
  };

 public:
  IOManager(size_t threads = 1, bool use_caller = true,
            const std::string& name = "");
  ~IOManager();

  int addEvent(int fd, Event event,
               std::function<void()> cb = nullptr);  //增加需要监听的事件
  bool removeEvent(int fd, Event event);             //删除事件
  bool cancelEvent(int fd, Event event);             //TODO
  bool cancelAll(int fd);                            //TODO
  void contextResize(size_t sz);
  void SafeContextResize(size_t sz);

  static IOManager* GetThis();

  bool stopping(uint64_t& time_out);

 protected:
  //Scheduler virtual functions:
  void tickle() override;
  void idle() override;
  bool stopping() override;

  //TimerManager virtual functions:
  void onTimerInsertAtFront() override;

 private:
  int m_epfd{-1};      //调度线程创建的epoll fd
  int m_tickleFds[2];  //pipe，用于唤醒其他线程

  std::atomic<size_t> m_pendingEventCount{0};  //剩余的事件数目
  RWMutexType m_mutex;
  std::vector<FdContext*> m_fdContexts;  //存放所有的fd上下文
};
}  // namespace East
