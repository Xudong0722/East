/*
 * @Author: Xudong0722 
 * @Date: 2025-03-24 21:45:15 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-12 22:04:16
 */

#include "Fiber.h"
#include <atomic>
#include "Config.h"
#include "Elog.h"
#include "Macro.h"
#include "Scheduler.h"

namespace East {

static Logger::sptr g_logger = ELOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id{0};
static std::atomic<uint64_t> s_fiber_count{0};

static thread_local Fiber* t_fiber{nullptr};  //当前正在执行的协程
static thread_local Fiber::sptr t_master_fiber{
    nullptr};  //当前线程中的主协程，切换到这里面，就相当于切换到了主协程中运行

static ConfigVar<uint32_t>::sptr g_fiber_stack_size = Config::Lookup<uint32_t>(
    "fiber.stack_size", 1024 * 1024, "fiber stack size");

class MallocStackAllocator {
 public:
  static void* Alloc(size_t size) { return malloc(size); }

  static void Dealloc(void* p, size_t size) { return free(p); }
};

using StackAllocator = MallocStackAllocator;  //you can use other malloc method

//主协程的构造函数, private funcion，只会在GetThis中调用
Fiber::Fiber() {
  m_state = EXEC;
  SetThis(this);

  //主协程的上下文就是当前运行的上下文
  if (getcontext(&m_ctx)) {
    EAST_ASSERT2(false, "getcontext");
  }

  ++s_fiber_count;
  ELOG_INFO(g_logger) << "Main Fiber created, id: " << m_id
                      << ", thread id: " << GetThreadId();  //main fiber id is 0
}

Fiber::Fiber(std::function<void()> cb, size_t stack_size, bool run_in_scheduler)
    : m_id(++s_fiber_id), m_cb(cb), m_run_in_scheduler(run_in_scheduler) {

  ++s_fiber_count;
  //没有指定的话，读配置
  m_stacksize = stack_size != 0 ? stack_size : g_fiber_stack_size->getValue();

  m_stack = StackAllocator::Alloc(m_stacksize);

  if (getcontext(&m_ctx)) {
    EAST_ASSERT2(false, "getcontext");
  }

  // if (!use_caller) {
  //   m_ctx.uc_link = &t_master_fiber->m_ctx;  //diff
  // } else {
  //   m_ctx.uc_link = &Scheduler::GetMainFiber()->m_ctx;  //diff
  // }

  //这个协程时需要运行我们指定的函数，我们需要申请额外的空间
  m_ctx.uc_link = nullptr;
  m_ctx.uc_stack.ss_sp = m_stack;
  m_ctx.uc_stack.ss_size = m_stacksize;

  //设置协程函数
  makecontext(&m_ctx, &Fiber::MainFunc, 0);

  //setState(INIT);
  ELOG_DEBUG(g_logger) << "Fiber created, id: " << m_id
                      << ", thread id: " << GetThreadId()
                      << ", run in scheduler: " << m_run_in_scheduler;
}

Fiber::~Fiber() {
  --s_fiber_count;
  bool is_master_fiber = false;
  if (m_stack) {
    EAST_ASSERT2(m_state == TERM || m_state == INIT || m_state == EXCEPT,
                 m_state);
    StackAllocator::Dealloc(m_stack, m_stacksize);
  } else {
    EAST_ASSERT(!m_cb);
    EAST_ASSERT(m_state == EXEC);

    is_master_fiber = true;  //no stack means main fiber
    Fiber* cur = t_fiber;
    if (cur == this) {
      SetThis(nullptr);
    }
  }

  if (is_master_fiber) {
    ELOG_DEBUG(g_logger) << "Main Fiber destroyed, id: " << m_id
                        << ", thread id: " << GetThreadId();
  } else {
    ELOG_DEBUG(g_logger) << "Fiber destroyed, id: " << m_id
                        << ", thread id: " << GetThreadId();
  }
}

//重置协程函数，并重置状态（当前状态：INIT/TERM)
void Fiber::reset(std::function<void()> cb) {
  EAST_ASSERT(m_stack);  //不能是主协程
  EAST_ASSERT(m_state == TERM || m_state == INIT);

  m_cb = cb;
  if (getcontext(&m_ctx)) {
    EAST_ASSERT2(false, "getcontext");
  }

  m_ctx.uc_link = nullptr;
  m_ctx.uc_stack.ss_sp = m_stack;
  m_ctx.uc_stack.ss_size = m_stacksize;

  makecontext(&m_ctx, &Fiber::MainFunc, 0);
  setState(INIT);
}

void Fiber::resume() {
  EAST_ASSERT2(m_state != EXEC, m_state);  //TODO
  SetThis(this);                           //该协程切换到当前协程
  setState(EXEC);

  if (!m_run_in_scheduler) {
    if (swapcontext(&t_master_fiber->m_ctx,
                    &m_ctx)) {  //old context, new context
      EAST_ASSERT2(false, "swapcontext: master fiber to cur fiber failed.");
    }
  } else {
    if (swapcontext(&Scheduler::GetMainFiber()->m_ctx,
                    &m_ctx)) {  //old context, new context
      EAST_ASSERT2(false, "swapcontext: scheduler fiber to cur fiber failed.");
    }
  }
  // ELOG_DEBUG(g_logger) << "Fiber resumed, id: " << m_id
  //             << ", thread id: " << GetThreadId() << ", state: " << m_state << ", m_run_in_caller: " << m_run_in_scheduler
  //             << ", master fiber addr: " << t_master_fiber.get() << ", cur fiber addr: " << this
  //             << ", scheduler fiber: " << Scheduler::GetMainFiber();
}

void Fiber::yield() {
  //EAST_ASSERT2(m_state == EXEC, m_state);
  ELOG_DEBUG(ELOG_ROOT()) << "m_run_int_scheduler: " << m_run_in_scheduler
                          << ", cur id: " << m_id << ", main fiber id: "
                          << Scheduler::GetMainFiber()->getId()
                          << ", master fiber id: " << t_master_fiber->getId();
  if (m_run_in_scheduler)
    SetThis(Scheduler::GetMainFiber());
  else
    SetThis(t_master_fiber.get());  //当前协程交还给主协程
  // if (getState() != TERM) {
  //   setState(READY);                //TODO， 这里应该设置成什么状态？
  // }

  if (!m_run_in_scheduler) {
    if (swapcontext(&m_ctx,
                    &t_master_fiber->m_ctx)) {  //old context, new context
      EAST_ASSERT2(false, "swapcontext: cur fiber to master fiber failed.");
    }
  } else {
    if (swapcontext(
            &m_ctx,
            &Scheduler::GetMainFiber()->m_ctx)) {  //old context, new context
      EAST_ASSERT2(false, "swapcontext: cur fiber to scheduler fiber failed.");
    }
  }
}

//设置当前线程正在运行的协程
void Fiber::SetThis(Fiber* p) {
  t_fiber = p;
}

//获取当前线程正在运行的协程，函数的副作用是如果没有协程就创建协程
Fiber::sptr Fiber::GetThis() {
  if (nullptr != t_fiber) {
    return t_fiber->shared_from_this();
  }

  //如果当前线程没有协程，则创建一个主协程
  Fiber::sptr main_fiber(new Fiber());
  EAST_ASSERT(main_fiber.get() == t_fiber);  //主协程理论也是当前协程
  t_master_fiber = main_fiber;
  return t_fiber->shared_from_this();
}

//将当前协程切换到后台，并且设置为Ready状态
void Fiber::YieldToReady() {
  Fiber::sptr cur_fiber = GetThis();
  EAST_ASSERT(cur_fiber->getState() == EXEC);  //期望当前协程是正在执行的状态
  cur_fiber->setState(READY);
  cur_fiber->yield();  //将当前协程切换出去
}

//将当前协程切换到后台，并且设置为Hold状态
void Fiber::YieldToHold() {
  Fiber::sptr cur_fiber = GetThis();
  EAST_ASSERT(cur_fiber->getState() == EXEC);
  //cur_fiber->setState(HOLD);  //TODO
  cur_fiber->yield();
}

uint64_t Fiber::TotalFibers() {
  return s_fiber_count;
}

uint64_t Fiber::GetFiberId() {
  return nullptr == t_fiber ? 0 : t_fiber->m_id;
}

void Fiber::MainFunc() {
  Fiber::sptr cur_fiber = GetThis();
  EAST_ASSERT(cur_fiber);
  try {
    cur_fiber->m_cb();
    cur_fiber->m_cb = nullptr;
    cur_fiber->setState(TERM);
  } catch (std::exception& e) {
    cur_fiber->setState(EXCEPT);
    ELOG_ERROR(g_logger) << "Fiber Except: " << e.what();
  } catch (...) {
    cur_fiber->setState(EXCEPT);
    ELOG_ERROR(g_logger) << "Fiber Except: ";
  }

  auto raw_ptr = cur_fiber.get();
  cur_fiber.reset();  //ref count minus one
  raw_ptr->yield();
}

};  // namespace East
