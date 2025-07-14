/*
 * @Author: Xudong0722 
 * @Date: 2025-03-24 21:45:18 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-26 00:26:58
 */

#pragma once
#include <ucontext.h>
#include <functional>
#include <memory>

namespace East {
class Fiber
    : public std::enable_shared_from_this<Fiber> {  //only create on heap
 public:
  using sptr = std::shared_ptr<Fiber>;

  enum State { INIT = 0, HOLD = 1, EXEC = 2, TERM = 3, READY = 4, EXCEPT = 5 };

 private:
  Fiber();

 public:
  /// @brief Create a fiber with a function
  /// @param cb
  /// @param stack_size, if 0, use default stack size
  /// @param run_in_scheduler, please set false if you are not in scheduler
  Fiber(std::function<void()> cb, size_t stack_size = 0,
        bool run_in_scheduler = true);
  ~Fiber();

  //重置协程函数，并重置状态
  void reset(std::function<void()> cb);
  //切换到当前协程执行
  void resume();
  //切换到后台执行
  void yield();

  //   void swapIn();
  //   void swapOut();

  State getState() const { return m_state; }
  void setState(State state) { m_state = state; }
  uint64_t getId() const { return m_id; }

 public:
  //设置当前协程
  static void SetThis(Fiber*);
  //返回当前协程
  static Fiber::sptr GetThis();
  //协程切换到后台，并且设为READY状态
  static void YieldToReady();
  //协程切换到后台，并且设置为HOLD状态
  static void YieldToHold();
  //总协程数
  static uint64_t TotalFibers();
  //获取当前协程id
  static uint64_t GetFiberId();

  static void MainFunc();

 private:
  uint64_t m_id{0};                //协程id
  uint32_t m_stacksize{0};         //协程栈大小
  State m_state{INIT};             //协程的状态
  ucontext_t m_ctx;                //协程存储的上下文
  void* m_stack{nullptr};          //协程额外的栈空间
  std::function<void()> m_cb;      //协程的入口函数
  bool m_run_in_scheduler{false};  //协程是否运行在调度器中
};

}  // namespace East
