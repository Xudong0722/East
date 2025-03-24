/*
 * @Author: Xudong0722 
 * @Date: 2025-03-24 21:45:18 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-24 23:01:26
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

  enum State { INIT = 0, HOLD = 1, EXEC = 2, TERM = 3, READY = 4 };

 private:
  Fiber();

 public:
  Fiber(std::function<void()> cb, size_t stack_size = 0);
  ~Fiber();

  //重置协程函数，并重置状态
  void reset(std::function<void()> cb);
  //切换到当前协程执行
  void SwapIn();
  //切换到后台执行
  void SwapOut();

 public:
  //返回当前协程
  static Fiber::sptr GetThis();
  //协程切换到后台，并且设为READY状态
  static void YieldToReady();
  //协程切换到后台，并且设置为HOLD状态
  static void YieldToHold();
  //总协程数
  static uint64_t TotalFibers();

  static MainFunc();

 private:
  uint64_t m_id{0};         //协程id
  uint32_t m_stacksize{0};  //协程栈大小
  State m_state{INIT};
  ucontext_t m_ctx;
  void* m_stack{nullptr};
  std::function<void()> m_cb;
};

}  // namespace East
