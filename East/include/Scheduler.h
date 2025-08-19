/*
 * @Author: Xudong0722 
 * @Date: 2025-04-01 22:53:37 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-04-08 23:56:29
 */

#pragma once

#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <typeinfo>
#include <vector>
#include "Elog.h"  //debug
#include "Fiber.h"
#include "Thread.h"

namespace East {

/**
 * @brief 协程调度器类，负责管理和调度协程任务
 * 
 * Scheduler是一个核心的协程调度组件，支持多线程协程调度。
 * 它维护一个线程池和任务队列，能够智能地将协程任务分配到合适的线程上执行。
 * 支持两种工作模式：
 * 1. 使用调用者线程参与调度（use_caller = true）
 * 2. 创建独立的线程池进行调度（use_caller = false）
 */
class Scheduler {
 public:
  using sptr = std::shared_ptr<Scheduler>;
  using MutexType = Mutex;

  /**
   * @brief 构造函数
   * @param threads 工作线程数量（不包括调用者线程）
   * @param use_caller 是否使用调用者线程参与调度
   * @param name 调度器名称，用于调试和日志
   */
  Scheduler(size_t threads = 1, bool use_caller = true,
            const std::string& name = "");
  
  /**
   * @brief 虚析构函数，确保正确清理资源
   */
  virtual ~Scheduler();

  /**
   * @brief 获取调度器名称
   * @return 调度器名称的常量引用
   */
  const std::string& getName() const { return m_name; }

 public:
  /**
   * @brief 获取当前线程的协程调度器
   * @return 当前线程关联的调度器指针，如果没有则返回nullptr
   */
  static Scheduler* GetThis();
  
  /**
   * @brief 设置当前线程的协程调度器
   * @param p 要设置的调度器指针
   */
  static void SetThis(Scheduler*);
  
  /**
   * @brief 获取当前协程调度器的主协程
   * @return 主协程指针，用于协程切换
   */
  static Fiber* GetMainFiber();

  /**
   * @brief 启动调度器
   * 
   * 创建指定数量的工作线程，开始协程调度。
   * 如果use_caller为true，调用者线程也会参与调度。
   */
  void start();
  
  /**
   * @brief 停止调度器
   * 
   * 优雅地停止所有工作线程，等待所有任务完成。
   * 确保资源正确释放，避免内存泄漏。
   */
  void stop();

  /**
   * @brief 调度协程任务（模板方法）
   * @param task 要调度的协程任务，支持右值引用
   * @param thread_id 指定执行线程ID，-1表示任意线程
   * 
   * 将协程任务添加到调度队列中，调度器会自动选择合适的线程执行。
   * 支持协程对象和函数对象的调度。
   */
  template <class Task>
  void schedule(Task&& task, int thread_id = -1) {
    bool need_tickle = false;
    {
      MutexType::LockGuard lock(m_mutex);
      need_tickle = scheduleNoLock(std::forward<Task>(task), thread_id);
    }

    if (need_tickle) {
      tickle();
    }
  }

  /**
   * @brief 批量调度任务（模板方法）
   * @param begin 任务迭代器起始位置
   * @param end 任务迭代器结束位置
   * 
   * 批量添加多个任务到调度队列，提高批量操作的效率。
   */
  template <class Iterator>
  void schedule(Iterator begin, Iterator end) {
    bool need_tickle = false;
    {
      MutexType::LockGuard lock(m_mutex);
      while (begin != end) {
        need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;
        ++begin;
      }
    }
    if (need_tickle) {
      tickle();
    }
  }

 protected:
  /**
   * @brief 调度器主运行循环
   * 
   * 每个工作线程都会执行此方法，负责从任务队列中取出任务并执行。
   * 包含任务调度、协程状态管理和空闲处理逻辑。
   */
  void run();
  
  /**
   * @brief 检查是否有空闲线程
   * @return 有空闲线程返回true，否则返回false
   */
  bool hasIdleThreads();
  
  /**
   * @brief 唤醒其他线程的虚函数
   * 
   * 当有新任务添加时，用于唤醒可能处于空闲状态的线程。
   * 子类可以重写此方法实现特定的唤醒机制。
   */
  virtual void tickle();
  
  /**
   * @brief 空闲处理协程的虚函数
   * 
   * 当线程没有可执行任务时，会执行此协程。
   * 子类可以重写此方法实现特定的空闲处理逻辑。
   */
  virtual void idle();
  
  /**
   * @brief 检查调度器是否应该停止的虚函数
   * @return 应该停止返回true，否则返回false
   */
  virtual bool stopping();

 private:
  /**
   * @brief 无锁的任务调度（模板方法）
   * @param task 要调度的任务
   * @param thread_id 指定执行线程ID
   * @return 是否需要唤醒其他线程
   * 
   * 内部使用的无锁任务添加方法，调用者需要确保线程安全。
   */
  template <class Task>
  bool scheduleNoLock(Task&& task, int thread_id = -1) {
    bool need_tickle = m_tasks.empty();
    ExecuteTask et(std::forward<Task>(task), thread_id);
    if (et.fiber || et.cb) {
      m_tasks.emplace_back(std::move(et));
      ELOG_INFO(ELOG_NAME("system"))
          << "Add new task, type: " << typeid(decltype(task)).name()
          << ", task id: " << et.getTaskId() << ", thread id: " << thread_id;
    }
    return need_tickle;
  }

  /**
   * @brief 全局任务ID计数器
   * 
   * 为每个任务分配唯一的ID，用于调试和跟踪。
   */
  static std::atomic<int32_t> s_task_id;

  /**
   * @brief 执行任务结构体
   * 
   * 封装了协程任务和函数任务，支持两种类型的任务执行。
   * 包含任务类型识别、状态管理和调试信息。
   */
  struct ExecuteTask {
    Fiber::sptr fiber;           ///< 协程任务指针
    std::function<void()> cb;    ///< 函数任务回调

    int thread_id;               ///< 指定执行线程ID，-1表示任意线程
    int task_id;                 ///< 任务唯一标识符，用于调试

    /**
     * @brief 协程任务构造函数
     * @param f 协程智能指针
     * @param id 指定线程ID
     */
    ExecuteTask(Fiber::sptr f, int id)
        : fiber(f), thread_id(id), task_id(++s_task_id) {}
    
    /**
     * @brief 函数任务构造函数
     * @param f 函数回调
     * @param id 指定线程ID
     */
    ExecuteTask(std::function<void()> f, int id)
        : cb(f), thread_id(id), task_id(++s_task_id) {}
    
    /**
     * @brief 协程任务移动构造函数
     * @param f 协程指针的指针，构造后原指针会被置空
     * @param id 指定线程ID
     */
    ExecuteTask(Fiber::sptr* f, int id)
        : thread_id(id),
          task_id(++s_task_id) {  //外部直接交出所有权，转移到ExecuteTask中
      fiber.swap(*f);
    }
    
    /**
     * @brief 函数任务移动构造函数
     * @param f 函数回调的指针，构造后原指针会被置空
     * @param id 指定线程ID
     */
    ExecuteTask(std::function<void()>* f, int id)
        : thread_id(id), task_id(++s_task_id) {
      cb.swap(*f);
    }
    
    /**
     * @brief 默认构造函数
     */
    ExecuteTask() : thread_id(-1), task_id(++s_task_id) {}

    /**
     * @brief 获取任务ID
     * @return 任务唯一标识符
     */
    int getTaskId() const { return task_id; }

    /**
     * @brief 重置任务状态
     * 
     * 清空所有任务相关数据，准备重新使用。
     */
    void reset() {
      fiber = nullptr;
      cb = nullptr;
      thread_id = -1;
    }

    /**
     * @brief 任务类型枚举
     */
    enum TaskType {
      NONE = -1,      ///< 无效任务
      FIBER = 0,      ///< 协程任务
      FUNCTION = 1,   ///< 函数任务
    };

    /**
     * @brief 获取任务类型
     * @return 任务类型枚举值
     */
    TaskType getTaskType() const {
      if (nullptr != fiber) {
        return FIBER;
      } else if (nullptr != cb) {
        return FUNCTION;
      }
      return NONE;
    }

    /**
     * @brief 检查任务是否有效
     * @return 任务有效返回true，否则返回false
     */
    bool isValidTask() const {
      if (getTaskType() == NONE)
        return false;
      return true;
    }
  };

 private:
  MutexType m_mutex;                    ///< 保护共享数据的互斥锁
  std::vector<Thread::sptr> m_threads;  ///< 工作线程池
  std::list<ExecuteTask> m_tasks;       ///< 待执行任务队列
  Fiber::sptr m_rootFiber;              ///< 主协程，用于调度管理
  std::string m_name;                   ///< 调度器名称

 protected:
  std::vector<int> m_threadIds;                 ///< 所有线程ID列表
  size_t m_threadCount = 0;                     ///< 工作线程数量
  std::atomic<size_t> m_activeThreadCount = 0;  ///< 当前活跃线程数量
  std::atomic<size_t> m_idleThreadCount = 0;    ///< 当前空闲线程数量
  bool m_stopping = true;                       ///< 调度器停止标志
  bool m_autoStop = false;                      ///< 自动停止标志
  int m_rootThreadId = 0;                       ///< 主线程ID
};

}  // namespace East
