/*
 * @Author: Xudong0722 
 * @Date: 2025-04-05 20:22:15 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-04-06 16:54:47
 */

#include "Timer.h"
#include "util.h"

namespace East {

Timer::Timer(uint64_t period, std::function<void()> cb, bool recurring,
             TimerManager* mgr)
    : m_period(period), m_cb(cb), m_recurring(recurring), m_timer_mgr(mgr) {
  m_execute_time = GetCurrentTimeInMs() + m_period;
}

bool Timer::Cmp::operator()(const Timer::sptr& lhs,
                            const Timer::sptr& rhs) const {
  if (nullptr == lhs && nullptr == rhs)
    return false;
  if (nullptr == lhs)
    return true;
  if (nullptr == rhs)
    return false;
  return lhs->m_execute_time < rhs->m_execute_time;
}

TimerManager::TimerManager(){} 
TimerManager::~TimerManager() {}

void TimerManager::addTimer(uint64_t period, std::function<void()> cb,
                            bool recurring) {
  Timer::sptr timer = std::make_shared<Timer>(period, cb, recurring, this);
  bool at_front{false};
  {
    RWMutexType::WLockGuard lock(m_mutex);
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
                                     bool recurring) {}
}  // namespace East
