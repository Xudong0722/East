/*
 * @Author: Xudong0722 
 * @Date: 2025-04-05 20:22:11 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-04-06 16:53:35
 */

#include <functional>
#include <memory>
#include <set>
#include "Mutex.h"

namespace East {
class TimerManager;
class Timer : public std::enable_shared_from_this<Timer> {
  friend class TimerManager;

 public:
  using sptr = std::shared_ptr<Timer>;
  Timer(uint64_t period, std::function<void()> cb, bool recurring,
        TimerManager* manager);

 private:
  bool m_recurring{false};             //是否是循环定时器
  uint64_t m_period{0};                //定时器执行周期， 单位：ms
  uint64_t m_execute_time{0};          //精确的执行时间
  std::function<void()> m_cb;          //定时器超时时回调函数
  TimerManager* m_timer_mgr{nullptr};  //当前timer所属的管理器
 private:
  // bool operator<(const Timer& rhs) const;
  struct Cmp {
    bool operator()(const sptr& lhs, const sptr& rhs) const;
  };
};

class TimerManager {
  friend class Timer;

 public:
  using sptr = std::shared_ptr<TimerManager>;
  using RWMutexType = RWLock;

  TimerManager();
  virtual ~TimerManager();

  void addTimer(uint64_t period, std::function<void()> cb,
                bool recurring = false);
  void addConditionTimer(uint64_t period, std::function<void()> cb,
                         std::weak_ptr<void> weak_cond, bool recurring = false);

 protected:
  virtual void onTimerInsertAtFront() = 0;

 private:
  RWLock m_mutex;
  std::set<Timer::sptr, Timer::Cmp> m_timers;
};
}  //namespace East
