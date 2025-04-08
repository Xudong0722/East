/*
 * @Author: Xudong0722 
 * @Date: 2025-04-05 20:22:15 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-04-07 14:34:28
 */

#include "Timer.h"
#include <algorithm>
#include "Elog.h"
#include "util.h"

namespace East {

Timer::Timer(uint64_t period, std::function<void()> cb, bool recurring,
             TimerManager* mgr)
    : m_recurring(recurring), m_period(period), m_cb(cb), m_timer_mgr(mgr) {
  m_execute_time = GetCurrentTimeInMs() + m_period;
}

Timer::Timer(uint64_t execute_time) : m_execute_time(execute_time) {}

bool Timer::Cmp::operator()(const Timer::sptr& lhs,
                            const Timer::sptr& rhs) const {
  if (nullptr == lhs && nullptr == rhs)
    return false;
  if (nullptr == lhs)
    return true;
  if (nullptr == rhs)
    return false;
  if (lhs->m_execute_time == rhs->m_execute_time) {
    return lhs < rhs;
  }
  return lhs->m_execute_time < rhs->m_execute_time;
}

bool Timer::cancel() {
  TimerManager::RWMutexType::WLockGuard wlock(m_timer_mgr->m_mutex);
  if (nullptr != m_cb) {
    m_cb = nullptr;
    auto it = m_timer_mgr->m_timers.find(shared_from_this());
    m_timer_mgr->m_timers.erase(it);
    return true;
  }
  return false;
}

bool Timer::refresh() {
  TimerManager::RWMutexType::WLockGuard wlock(m_timer_mgr->m_mutex);
  if (nullptr == m_cb)
    return false;
  auto it = m_timer_mgr->m_timers.find(shared_from_this());
  if (it == m_timer_mgr->m_timers.end())
    return false;
  auto myself = shared_from_this();
  m_timer_mgr->m_timers.erase(it);
  //TODO, 先删除再插入
  m_execute_time = GetCurrentTimeInMs() + m_period;
  m_timer_mgr->m_timers.insert(myself);
  return true;
}

bool Timer::reset(uint64_t period, bool from_now) {
  if (m_period == period && !from_now)
    return true;

  TimerManager::RWMutexType::WLockGuard wlock(m_timer_mgr->m_mutex);
  if (nullptr == m_cb)
    return false;
  auto it = m_timer_mgr->m_timers.find(shared_from_this());
  auto myself =
      shared_from_this();  //add ref count before erase, make sure current timer is alive
  if (it == m_timer_mgr->m_timers.end())
    return false;
  m_timer_mgr->m_timers.erase(it);

  uint64_t start{0};
  if (from_now) {
    start = GetCurrentTimeInMs();
  } else {
    start = m_execute_time - m_period;
  }
  m_period = period;
  m_execute_time = start + m_period;
  //ELOG_INFO(ELOG_ROOT()) << "recurring: " << m_recurring << ", this: " << (void*)this << ", ref count: " << myself.use_count();  //TODO, why core dump when I add this line?, we need to add ref count before erase
  m_timer_mgr->addTimer(myself, wlock);
  return true;
}

TimerManager::TimerManager() {}
TimerManager::~TimerManager() {}

Timer::sptr TimerManager::addTimer(uint64_t period, std::function<void()> cb,
                                   bool recurring) {
  Timer::sptr timer = std::make_shared<Timer>(period, cb, recurring, this);
  RWMutexType::WLockGuard wlock(m_mutex);
  addTimer(timer, wlock);
  return timer;
}

Timer::sptr TimerManager::addConditionTimer(uint64_t period,
                                            std::function<void()> cb,
                                            std::weak_ptr<void> weak_cond,
                                            bool recurring) {
  auto func = [weak_cond, cb]() -> void {
    auto tmp = weak_cond.lock();
    if (tmp != nullptr) {
      cb();
    }
  };

  return addTimer(period, func, recurring);
}

void TimerManager::addTimer(Timer::sptr timer,
                            TimerManager::RWMutexType::WLockGuard& lock) {
  auto it = m_timers.insert(timer).first;
  bool at_front = (it == m_timers.begin()) && !m_tickled;
  if (at_front) {
    m_tickled = true;
  }
  lock.unlock();
  if (at_front) {
    onTimerInsertAtFront();
  }
}

uint64_t TimerManager::getNextTimer() {
  RWMutexType::RLockGuard rlock(m_mutex);
  if (m_timers.empty()) {
    return ~0ull;
  }
  m_tickled = false;
  const Timer::sptr& next = *m_timers.begin();
  const uint64_t now_ms = GetCurrentTimeInMs();
  if (now_ms >= next->m_execute_time) {
    return 0ull;
  }

  return next->m_execute_time - now_ms;
}

void TimerManager::listExpiredCb(std::vector<std::function<void()>>& cbs) {
  {
    RWLock::RLockGuard rlock(m_mutex);
    if (m_timers.empty()) {
      return;
    }
  }

  RWLock::WLockGuard wlock(m_mutex);
  if (m_timers.empty()) {
    return;
  }

  const uint64_t now_ms = GetCurrentTimeInMs();

  if (now_ms < (*m_timers.begin())->m_execute_time) {
    //当前没有过期的定时器
    return;
  }

  Timer::sptr timer_for_now = std::make_shared<Timer>(now_ms);
  auto it = std::lower_bound(begin(m_timers), end(m_timers), timer_for_now);
  //lower_bound是找到第一个不小于timer_for_now的元素，我们还得加上等于timer_for_now的

  while (it != m_timers.end() && (*it)->m_execute_time == now_ms) {
    ++it;
  }

  cbs.clear();
  std::vector<Timer::sptr> expired(m_timers.begin(), it);
  cbs.resize(expired.size());
  m_timers.erase(m_timers.begin(), it);
  // ELOG_INFO(ELOG_ROOT()) << "------------------"
  //                        << "timer size, before: " << m_timers.size()
  //                        << ", expired size: " << expired.size();

  auto i = 0u;
  for (const auto& p : expired) {
    cbs[i] = p->m_cb;
    if (p->m_recurring) {
      // auto old_execute_time = p->m_execute_time;
      p->m_execute_time = now_ms + p->m_period;
      // ELOG_INFO(ELOG_ROOT())
      //     << "recurring timer, old execute time: " << old_execute_time
      //     << ", new execute time: " << p->m_execute_time
      //     << " , period: " << p->m_period << ", now: " << now_ms;
      m_timers.insert(p);
    } else {
      p->m_cb = nullptr;  //清空回调函数
    }
  }
  // ELOG_INFO(ELOG_ROOT()) << "------------------"
  //                        << "timer size, after: " << m_timers.size()
  //                        << ", expired size: " << expired.size();
}

bool TimerManager::hasTimer() {
  RWMutexType::RLockGuard rlock(m_mutex);
  return !m_timers.empty();
}
}  // namespace East
