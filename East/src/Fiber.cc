/*
 * @Author: Xudong0722 
 * @Date: 2025-03-24 21:45:15 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-26 00:27:49
 */

#include "Fiber.h"
#include <atomic>
#include "Config.h"
#include "Elog.h"
#include "Macro.h"

namespace East {

static Logger::sptr g_logger = ELOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id{0};
static std::atomic<uint64_t> s_fiber_count{0};

static thread_local Fiber* t_fiber{nullptr};  //当前正在执行的协程
static thread_local Fiber::sptr t_master_fiber{nullptr};  //线程中的主协程

static ConfigVar<uint32_t>::sptr g_fiber_stack_size = Config::Lookup<uint32_t>(
    "fiber.stack_size", 1024 * 1024, "fiber stack size");

class MallocStackAllocator {
 public:
  static void* Alloc(size_t size) { return malloc(size); }

  static void Dealloc(void* p, size_t size) { return free(p); }
};

using StackAllocator = MallocStackAllocator;  //you can use other malloc method

Fiber::Fiber() {
  m_state = EXEC;
  SetThis(this);

  if (getcontext(&m_ctx)) {
    EAST_ASSERT2(false, "getcontext");
  }

  ++s_fiber_count;
  ELOG_INFO(g_logger) << "Main Fiber created, id: " << m_id;
}

Fiber::Fiber(std::function<void()> cb, size_t stack_size)
    : m_id(++s_fiber_id), m_cb(cb) {

  ++s_fiber_count;
  m_stacksize = stack_size != 0 ? stack_size : g_fiber_stack_size->getValue();

  m_stack = StackAllocator::Alloc(m_stacksize);

  if (getcontext(&m_ctx)) {
    EAST_ASSERT2(false, "getcontext");
  }

  m_ctx.uc_link = &t_master_fiber->m_ctx;  //diff
  m_ctx.uc_stack.ss_sp = m_stack;
  m_ctx.uc_stack.ss_size = m_stacksize;

  makecontext(&m_ctx, &Fiber::MainFunc, 0);

  ELOG_INFO(g_logger) << "Fiber created, id: " << m_id;
}

Fiber::~Fiber() {
  --s_fiber_count;
  if (m_stack) {
    EAST_ASSERT(m_state == TERM || m_state == INIT || m_state == EXCEPT);
    StackAllocator::Dealloc(m_stack, m_stacksize);
  } else {
    EAST_ASSERT(!m_cb);
    EAST_ASSERT(m_state == EXEC);

    Fiber* cur = t_fiber;
    if (cur == this) {
      SetThis(nullptr);
    }
  }

  ELOG_INFO(g_logger) << "~Fiber destroy: " << m_id;
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

void Fiber::swapIn() {
  EAST_ASSERT2(m_state != EXEC, m_state);
  SetThis(this);  //该协程切换到当前协程
  setState(EXEC);
  if (swapcontext(&t_master_fiber->m_ctx, &m_ctx)) {  //old context, new context
    EAST_ASSERT2(false, "swapcontext:swap in");
  }
}

void Fiber::swapOut() {
  //EAST_ASSERT2(m_state == EXEC, m_state);
  SetThis(t_master_fiber.get());  //当前协程交还给主协程

  if (swapcontext(&m_ctx, &t_master_fiber->m_ctx)) {  //old context, new context
    EAST_ASSERT2(false, "swapcontext:swap out");
  }
}

void Fiber::SetThis(Fiber* p) {
  t_fiber = p;
}

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
  cur_fiber->swapOut();  //将当前协程切换出去
}

//将当前协程切换到后台，并且设置为Hold状态
void Fiber::YieldToHold() {
  Fiber::sptr cur_fiber = GetThis();
  EAST_ASSERT(cur_fiber->getState() == EXEC);
  cur_fiber->setState(HOLD);
  cur_fiber->swapOut();
}

uint64_t Fiber::TotalFibers() {
  return s_fiber_count;
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
}

};  // namespace East
