/*
 * @Author: Xudong0722 
 * @Date: 2025-04-05 20:22:15 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-04-06 21:44:30
 */

#include "Timer.h"
#include <algorithm>
#include "util.h"

namespace East {

Timer::Timer(uint64_t period, std::function<void()> cb, bool recurring,
             TimerManager* mgr)
    : m_period(period), m_cb(cb), m_recurring(recurring), m_timer_mgr(mgr) {
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

TimerManager::TimerManager() {}
TimerManager::~TimerManager() {}

void TimerManager::addTimer(uint64_t period, std::function<void()> cb,
                            bool recurring) {
  Timer::sptr timer = std::make_shared<Timer>(period, cb, recurring, this);
  bool at_front{false};
  {
    RWMutexType::WLockGuard wlock(m_mutex);  //TODO，是否可以放在{}里？
    auto it = m_timers.insert(timer).first;
    if (it == m_timers.begin()) {
      at_front = true;
    }
  }

  if (at_front) {
    onTimerInsertAtFront();
  }
}

void TimerManager::addConditionTimer(uint64_t period, std::function<void()> cb,
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

uint64_t TimerManager::getNextTimer() {
  RWMutexType::RLockGuard rlock(m_mutex);
  if (m_timers.empty()) {
    return ~0ull;
  }

  const Timer::sptr& next = *m_timers.begin();
  const uint64_t now_ms = GetCurrentTimeInMs();
  if (now_ms >= next->m_execute_time) {
    return 0ull;
  }

  return next->m_execute_time - now_ms;
}

void TimerManager::listExpiredCb(std::vector<std::function<void()>>& cbs) {
  const uint64_t now_ms = GetCurrentTimeInMs();
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
  auto i = 0u;
  for (const auto& p : expired) {
    cbs[i] = p->m_cb;
    if (p->m_recurring) {
      p->m_execute_time += p->m_period;
      m_timers.insert(p);
    }
  }
}

}  // namespace East
