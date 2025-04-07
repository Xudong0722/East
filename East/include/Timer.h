/*
 * @Author: Xudong0722 
 * @Date: 2025-04-05 20:22:11 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-04-07 14:08:54
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
  Timer(uint64_t execute_time);

  bool cancel();
  bool refresh();
  bool reset(uint64_t period, bool from_now);

 private:
  bool m_recurring{false};  //是否是循环定时器
  uint64_t m_period{0};     //定时器执行周期， 单位：ms
  uint64_t m_execute_time{
      0};  //精确的执行时间, TODO这个含义可能不够准确，后续纠正
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

  //添加定时器任务
  Timer::sptr addTimer(uint64_t period, std::function<void()> cb,
                       bool recurring = false);

  //添加带有条件的定时器任务
  Timer::sptr addConditionTimer(uint64_t period, std::function<void()> cb,
                                std::weak_ptr<void> weak_cond,
                                bool recurring = false);

  //Timer的某些操作可能会调用到onTimerInsertAtFront，统一使用这个方法
  void addTimer(Timer::sptr timer, RWMutexType::WLockGuard& lock);

  //定时器队列头部的定时器执行时间与当前时间的间隔
  uint64_t getNextTimer();

  //获取所有已经超时的定时器的回调函数
  void listExpiredCb(std::vector<std::function<void()>>& cbs);

  bool hasTimer();

 protected:
  virtual void onTimerInsertAtFront() = 0;

 private:
  RWLock m_mutex;
  std::set<Timer::sptr, Timer::Cmp> m_timers;
  bool m_tickled{false};
};
}  //namespace East
