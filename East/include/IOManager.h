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

/**
 * @brief IO管理器类，继承自调度器和定时器管理器
 * 
 * IOManager负责管理文件描述符的IO事件，使用epoll机制实现高效的IO多路复用。
 * 它结合了协程调度和定时器管理功能，为异步IO操作提供统一的接口。
 * 
 * 主要功能：
 * - 文件描述符的IO事件监听和管理
 * - 协程调度和切换
 * - 定时器管理
 * - 线程间通信和唤醒机制
 */
class IOManager : public Scheduler, public TimerManager {
 public:
  using sptr = std::shared_ptr<IOManager>;
  using RWMutexType = RWLock;

  /**
   * @brief IO事件类型枚举
   * 
   * 定义了可监听的IO事件类型，对应epoll的事件类型
   */
  enum Event {
    NONE = 0x0,    ///< 无事件
    READ = 0x1,    ///< 读事件，对应EPOLL_EVENTS::EPOLLIN
    WRITE = 0x4,   ///< 写事件，对应EPOLL_EVENTS::EPOLLOUT
  };

 private:
  /**
   * @brief 文件描述符上下文结构体
   * 
   * 存储每个文件描述符的IO事件信息和相关的调度器、协程、回调函数等
   */
  struct FdContext {
    using MutextType = Mutex;
    
    /**
     * @brief 事件上下文结构体
     * 
     * 存储特定事件类型的执行上下文信息
     */
    struct EventContext {
      Scheduler* scheduler{nullptr};  ///< 事件执行所属的调度器
      Fiber::sptr fiber{nullptr};     ///< 事件执行所属的协程
      std::function<void()> cb;       ///< 事件执行回调函数
    };

    /**
     * @brief 根据事件类型获取对应的上下文
     * @param event 事件类型
     * @return 对应事件的上下文引用
     */
    EventContext& getContext(Event event);
    
    /**
     * @brief 重置事件上下文
     * @param event_ctx 要重置的事件上下文
     */
    void resetContext(EventContext& event_ctx);
    
    /**
     * @brief 触发指定事件
     * @param event 要触发的事件类型
     */
    void triggerEvent(Event event);

    int fd;                    ///< 文件描述符
    EventContext read;         ///< 读事件上下文
    EventContext write;        ///< 写事件上下文
    Event events{NONE};        ///< 当前监听的事件类型
    MutexType mutex;           ///< 保护fd上下文的互斥锁
  };

 public:
  /**
   * @brief 构造函数
   * @param threads 工作线程数量，默认为1
   * @param use_caller 是否使用调用线程作为工作线程，默认为true
   * @param name 调度器名称
   */
  IOManager(size_t threads = 1, bool use_caller = true,
            const std::string& name = "");
  
  /**
   * @brief 析构函数
   * 
   * 清理资源，关闭epoll文件描述符和管道
   */
  ~IOManager();

  /**
   * @brief 添加需要监听的IO事件
   * @param fd 文件描述符
   * @param event 要监听的事件类型
   * @param cb 事件触发时的回调函数，如果为nullptr则使用当前协程
   * @return 成功返回0，失败返回-1
   */
  int addEvent(int fd, Event event,
               std::function<void()> cb = nullptr);
  
  /**
   * @brief 删除指定的事件监听
   * @param fd 文件描述符
   * @param event 要删除的事件类型
   * @return 成功返回true，失败返回false
   */
  bool removeEvent(int fd, Event event);
  
  /**
   * @brief 取消指定事件并触发回调（TODO: 待实现）
   * @param fd 文件描述符
   * @param event 要取消的事件类型
   * @return 成功返回true，失败返回false
   */
  bool cancelEvent(int fd, Event event);
  
  /**
   * @brief 取消文件描述符上的所有事件（TODO: 待实现）
   * @param fd 文件描述符
   * @return 成功返回true，失败返回false
   */
  bool cancelAll(int fd);
  
  /**
   * @brief 调整文件描述符上下文数组大小
   * @param sz 新的数组大小
   */
  void contextResize(size_t sz);
  
  /**
   * @brief 安全地调整文件描述符上下文数组大小
   * @param sz 新的数组大小
   */
  void SafeContextResize(size_t sz);

  /**
   * @brief 获取当前线程的IOManager实例
   * @return 当前线程的IOManager指针，如果不存在则返回nullptr
   */
  static IOManager* GetThis();

  /**
   * @brief 检查是否正在停止，并获取下一个定时器的超时时间
   * @param time_out 输出参数，下一个定时器的超时时间
   * @return 如果正在停止返回true，否则返回false
   */
  bool stopping(uint64_t& time_out);

 protected:
  // Scheduler虚函数重写
  /**
   * @brief 唤醒空闲线程
   * 
   * 通过管道向epoll写入数据来唤醒等待的线程
   */
  void tickle() override;
  
  /**
   * @brief 空闲状态处理
   * 
   * 主要的IO事件循环，处理epoll事件和定时器
   */
  void idle() override;
  
  /**
   * @brief 检查调度器是否正在停止
   * @return 如果正在停止返回true，否则返回false
   */
  bool stopping() override;

  // TimerManager虚函数重写
  /**
   * @brief 当定时器插入到队列前端时的回调
   * 
   * 唤醒空闲线程以处理新插入的定时器
   */
  void onTimerInsertAtFront() override;

 private:
  int m_epfd{-1};             ///< epoll文件描述符，用于IO多路复用
  int m_tickleFds[2];         ///< 管道文件描述符，用于线程间通信和唤醒

  std::atomic<size_t> m_pendingEventCount{0};  ///< 待处理的事件数量
  RWMutexType m_mutex;                          ///< 保护fd上下文数组的读写锁
  std::vector<FdContext*> m_fdContexts;        ///< 文件描述符上下文数组
};

}  // namespace East
