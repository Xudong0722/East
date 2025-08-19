/*
 * @Author: Xudong0722 
 * @Date: 2025-04-01 22:53:33 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-04-09 01:25:17
 */
#include "Scheduler.h"
#include "Elog.h"
#include "Hook.h"
#include "Macro.h"

namespace East {

/**
 * @brief 系统日志记录器，用于记录调度器的运行日志
 */
static East::Logger::sptr g_logger = ELOG_NAME("system");

/**
 * @brief 线程本地存储：当前线程的调度器指针
 * 
 * 同一个调度器下的所有线程都指向同一个调度器实例，
 * 用于在协程切换时获取当前线程的调度器上下文。
 */
static thread_local Scheduler* t_scheduler =
    nullptr;

/**
 * @brief 线程本地存储：当前线程的调度协程指针
 * 
 * 每个线程都有自己独立的调度协程，包括调用者线程。
 * 用于管理协程的调度和切换。
 */
static thread_local Fiber* t_scheduler_fiber =
    nullptr;  

/**
 * @brief 全局任务ID计数器
 * 
 * 为每个任务分配唯一的递增ID，用于调试、日志记录和任务跟踪。
 */
std::atomic<int32_t> Scheduler::s_task_id{0};

/**
 * @brief 构造函数实现
 * 
 * 初始化调度器的核心组件和状态：
 * 1. 验证线程数量参数的有效性
 * 2. 根据use_caller参数决定是否使用调用者线程
 * 3. 如果使用调用者线程，创建主协程并设置线程本地存储
 * 4. 初始化线程计数和ID管理
 * 
 * @param threads 工作线程数量（不包括调用者线程）
 * @param use_caller 是否使用调用者线程参与调度
 * @param name 调度器名称
 */
Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name)
    : m_name(name) {
  EAST_ASSERT2(threads > 0, "threads must be at least 1");

  //user_caller: 是否使用当前调用线程
  if (use_caller) {
    East::Fiber::GetThis();  //如果当前线程没有协程，会创建一个协程
    --threads;               //当前调用线程参与，所以线程数减一

    EAST_ASSERT2(GetThis() == nullptr,
                 "Scheduler has been created");  //一个线程只能有一个调度器
    t_scheduler = this;

    m_rootFiber =
        std::make_shared<Fiber>(std::bind(&Scheduler::run, this), 0, false);

    t_scheduler_fiber = m_rootFiber.get();

    East::Thread::SetName(m_name);  //why use this name?
    m_rootThreadId = East::GetThreadId();
    m_threadIds.push_back(m_rootThreadId);
  } else {
    m_rootThreadId = -1;
  }
  m_threadCount = threads;
}

/**
 * @brief 析构函数实现
 * 
 * 确保调度器已经停止，并清理线程本地存储。
 * 防止在调度器运行期间意外销毁对象。
 */
Scheduler::~Scheduler() {
  EAST_ASSERT(m_stopping);
  if (GetThis() == this) {
    t_scheduler = nullptr;
  }
}

/**
 * @brief 获取当前线程的协程调度器
 * @return 当前线程关联的调度器指针，如果没有则返回nullptr
 */
Scheduler* Scheduler::GetThis() {
  return t_scheduler;
}

/**
 * @brief 获取当前协程调度器的主协程
 * @return 主协程指针，用于协程切换
 */
Fiber* Scheduler::GetMainFiber() {
  return t_scheduler_fiber;
}

/**
 * @brief 设置当前线程的协程调度器
 * @param p 要设置的调度器指针
 */
void Scheduler::SetThis(Scheduler* p) {
  t_scheduler = p;
}

/**
 * @brief 启动调度器
 * 
 * 核心启动逻辑：
 * 1. 检查调度器是否已经启动，避免重复启动
 * 2. 设置停止标志为false，允许任务执行
 * 3. 创建指定数量的工作线程，每个线程都执行run方法
 * 4. 记录所有线程的ID，用于后续管理和调试
 * 
 * 注意：此方法需要线程安全，使用互斥锁保护共享状态。
 */
void Scheduler::start() {
  MutexType::LockGuard lock(m_mutex);

  if (!m_stopping)
    return;
  m_stopping = false;
  EAST_ASSERT2(m_threads.empty(), "m_threads is not empty");
  m_threads.resize(m_threadCount);

  for (size_t i = 0; i < m_threadCount; ++i) {
    m_threads[i].reset(new Thread(m_name + "_" + std::to_string(i),
                                  std::bind(&Scheduler::run, this)));
    m_threadIds.emplace_back(m_threads[i]->getId());
  }
}

/**
 * @brief 停止调度器
 * 
 * 优雅停止的核心逻辑：
 * 1. 设置自动停止标志，通知所有线程准备停止
 * 2. 检查根协程状态，如果已经完成则直接停止
 * 3. 设置停止标志，防止新任务继续执行
 * 4. 唤醒所有工作线程，让它们检查停止条件
 * 5. 如果根协程存在且调度器未停止，执行一次run方法
 * 6. 等待所有工作线程完成并清理资源
 * 
 * 关键设计：避免在持锁情况下调用join，防止死锁。
 */
void Scheduler::stop() {
  ELOG_DEBUG(g_logger) << "enter Scheduler::stop";
  m_autoStop = true;

  if (m_rootFiber != nullptr && m_threadCount == 0 &&
      (m_rootFiber->getState() == Fiber::TERM ||
       m_rootFiber->getState() == Fiber::INIT)) {
    ELOG_DEBUG(g_logger) << this << " stopped.";
    m_stopping = true;

    if (stopping()) {
      return;
    }
  }

  if (m_rootThreadId != -1) {
    EAST_ASSERT2(GetThis() == this, "GetThis() != this");  //
  } else {
    EAST_ASSERT2(GetThis() != this, "GetThis() == this");
  }

  m_stopping = true;  //避免其他任务继续执行
  for (size_t i = 0; i < m_threadCount; ++i) {
    tickle();
  }

  if (m_rootFiber) {
    if (!stopping()) {
      ELOG_DEBUG(g_logger)
          << "Scheduler not stopping, root fiber will be called before stop";
      m_rootFiber
          ->resume();  //确保调度器stop之前会执行一次run方法，如果放在start里面的话，如果没有任务就空跑一次fiber了
    }
  }

  //不要在持锁的情况下调用join，否则可能会死锁
  std::vector<Thread::sptr> tmp_threads;
  {
    MutexType::LockGuard lock(m_mutex);
    tmp_threads.swap(m_threads);
  }

  for (auto& i : tmp_threads) {
    i->join();
  }
}

/**
 * @brief 调度器主运行循环
 * 
 * 这是每个工作线程的核心执行逻辑，包含以下关键步骤：
 * 
 * 1. **初始化阶段**：
 *    - 设置当前线程的调度器上下文
 *    - 启用hook功能，支持协程友好的系统调用
 *    - 设置调度协程指针
 *    - 创建空闲协程和回调协程
 * 
 * 2. **任务调度循环**：
 *    - 从任务队列中取出可执行的任务
 *    - 处理线程亲和性（指定线程执行）
 *    - 跳过已在执行或无效的协程
 *    - 更新活跃线程计数
 * 
 * 3. **任务执行**：
 *    - 协程任务：检查状态并执行，处理完成后根据状态决定是否重新调度
 *    - 函数任务：使用专用协程执行，支持重用协程对象
 *    - 空闲处理：当没有任务时执行空闲协程
 * 
 * 4. **状态管理**：
 *    - 协程状态转换：READY -> EXEC -> HOLD/TERM/EXCEPT
 *    - 线程计数管理：活跃线程和空闲线程的统计
 *    - 优雅退出：检查停止条件并退出循环
 * 
 * 关键设计特点：
 * - 支持协程和函数两种任务类型
 * - 智能的任务重新调度机制
 * - 避免协程对象重复创建
 * - 线程安全的队列操作
 */
void Scheduler::run() {
  ELOG_DEBUG(g_logger) << "run";
  SetThis(this);
  set_hook_enable(true);  //设置当前线程需要hook, 我们自己的scheduler需要hook
  if (East::GetThreadId() != m_rootThreadId) {
    t_scheduler_fiber =
        Fiber::GetThis()
            .get();  //如果不是scheduer的主线程，就把当前协程设置为主协程
  }

  //ELOG_INFO(g_logger) << "idle fiber will be created here";
  //创建一个idle协程，专门用于处理空闲状态
  Fiber::sptr idle_fiber =
      std::make_shared<Fiber>(std::bind(&Scheduler::idle, this));
  Fiber::sptr cb_fiber{nullptr};  //用于执行回调函数

  ExecuteTask task{};
  while (true) {
    task.reset();

    bool tickle_me = false;
    bool is_active = false;
    {
      MutexType::LockGuard lock(m_mutex);
      auto it = m_tasks.begin();
      while (it != m_tasks.end()) {
        //如果是指定线程执行，且不是当前线程，就跳过
        if (it->thread_id != -1 && it->thread_id != East::GetThreadId()) {
          ++it;
          tickle_me = true;
          continue;
        }

        EAST_ASSERT(it->fiber || it->cb);
        //如果是协程且协程已经在执行，也直接跳过
        if (it->fiber && it->fiber->getState() == Fiber::EXEC) {
          ++it;
          continue;
        }

        task = *it;
        it = m_tasks.erase(it);
        ++m_activeThreadCount;
        is_active = true;
        break;
      }
      tickle_me = tickle_me || it != m_tasks.end();
    }
    if (tickle_me) {
      tickle();
    }
    if (task.isValidTask()) {
      ELOG_DEBUG(g_logger) << "Hanlded task in queue, id: " << task.getTaskId()
                           << ", task type: " << task.getTaskType()
                           << ", task queue size: " << m_tasks.size();
      if (task.getTaskType() == ExecuteTask::FIBER) {
        ELOG_DEBUG(g_logger) << "fiber state: " << task.fiber->getState();
      }
    }

    //如果协程的状态可以执行，则执行                          //TODO: 这里的状态判断有点问题, 如果是hold状态，现在不一定能执行，因为可能有定时器
    if (task.getTaskType() == ExecuteTask::FIBER &&
        (task.fiber->getState() != Fiber::TERM &&
         task.fiber->getState() != Fiber::EXCEPT)) {
      task.fiber->resume();
      --m_activeThreadCount;

      if (task.fiber->getState() ==
          Fiber::READY) {  //执行完之后如果协程还是ready状态，就放回到任务队列中
        ELOG_DEBUG(g_logger) << "After resume, task fiber is on ready state, "
                                "put back to queue. Task id: "
                             << task.getTaskId();
        schedule(task.fiber);
      } else if (task.fiber->getState() != Fiber::TERM &&
                 task.fiber->getState() != Fiber::EXCEPT) {
        task.fiber->setState(Fiber::HOLD);
      }
      task.reset();
    } else if (task.getTaskType() == ExecuteTask::FUNCTION) {
      if (cb_fiber != nullptr) {
        cb_fiber->reset(task.cb);
      } else {
        cb_fiber = std::make_shared<Fiber>(task.cb);
      }
      task.reset();
      cb_fiber->resume();
      --m_activeThreadCount;
      if (cb_fiber->getState() == Fiber::READY) {
        schedule(cb_fiber);
        cb_fiber.reset();
      } else if (cb_fiber->getState() == Fiber::EXCEPT ||
                 cb_fiber->getState() == Fiber::TERM) {
        cb_fiber->reset(nullptr);
      } else {
        cb_fiber->setState(
            Fiber::
                HOLD);  //bug trace: https://www.yuque.com/kuaiquxiedaima/ov0nhi/xvssyw7z02mgkd17#D5TXn
        cb_fiber.reset();
      }
    } else {
      if (is_active) {
        --m_activeThreadCount;
        continue;
      }

      if (idle_fiber->getState() == Fiber::TERM) {
        ELOG_DEBUG(g_logger) << "idle fiber term";
        break;
      }

      ++m_idleThreadCount;
      idle_fiber->resume();
      --m_idleThreadCount;
      if (idle_fiber->getState() != Fiber::TERM &&
          idle_fiber->getState() != Fiber::EXCEPT) {
        idle_fiber->setState(Fiber::HOLD);  //TODO
      }
    }
  }
}

/**
 * @brief 检查是否有空闲线程
 * @return 有空闲线程返回true，否则返回false
 * 
 * 通过检查空闲线程计数器来判断当前是否有线程处于空闲状态。
 * 这个信息可以用于负载均衡和性能监控。
 */
bool Scheduler::hasIdleThreads() {
  return m_idleThreadCount > 0;
}

/**
 * @brief 唤醒其他线程的虚函数
 * 
 * 默认实现只是记录日志。子类可以重写此方法实现特定的唤醒机制，
 * 比如使用条件变量、信号量或管道等机制来唤醒等待的线程。
 * 
 * 当有新任务添加时，调度器会调用此方法来通知可能处于空闲状态的线程。
 */
void Scheduler::tickle() {
  ELOG_DEBUG(g_logger) << "tickle";
}

/**
 * @brief 空闲处理协程的虚函数
 * 
 * 当线程没有可执行任务时，会执行此协程。默认实现：
 * 1. 在循环中检查调度器是否应该停止
 * 2. 如果不应该停止，则让出执行权给其他协程
 * 3. 使用YieldToHold让协程进入HOLD状态，等待被重新调度
 * 
 * 子类可以重写此方法实现特定的空闲处理逻辑，比如：
 * - 执行后台清理任务
 * - 进行性能统计
 * - 处理定时器事件
 */
void Scheduler::idle() {
  ELOG_DEBUG(g_logger) << "idle";
  while (!stopping()) {
    East::Fiber::YieldToHold();
  }
}

/**
 * @brief 检查调度器是否应该停止的虚函数
 * @return 应该停止返回true，否则返回false
 * 
 * 停止条件检查逻辑：
 * 1. 自动停止标志已设置（m_autoStop = true）
 * 2. 停止标志已设置（m_stopping = true）
 * 3. 任务队列为空（m_tasks.empty()）
 * 4. 没有活跃线程（m_activeThreadCount == 0）
 * 
 * 只有同时满足以上四个条件，调度器才会真正停止。
 * 这种设计确保了所有任务都能被正确处理完成。
 */
bool Scheduler::stopping() {
  MutexType::LockGuard lock(m_mutex);
  return m_autoStop && m_stopping && m_tasks.empty() &&
         m_activeThreadCount == 0;
}

//切换到某个线程中执行
// void Scheduler::switchTo(int thread_id = -1) {

// }

// std::ostream& Scheduler::dump(std::ostream& os) {

// }

}  // namespace East
