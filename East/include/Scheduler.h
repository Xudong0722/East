
#pragma once

#include <memory>
#include <vector>
#include <iostream>
#include <list>
#include <string>
#include "Fiber.h"
#include "Thread.h"

namespace East {

class Scheduler {
public:
    using sptr = std::shared_ptr<Scheduler>;
    using MutexType = Mutex;

    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");
    virtual ~Scheduler();

    const std::string& getName() const { return m_name; }

public:
    //static func
    static Scheduler* GetThis();    //获取当前线程的协程调度器
    static Fiber* GetMainFiber();   //获取当前协程调度器的主协程
public:
   void setThis();
   void start();
   void stop();
   void switchTo(int thread_id = -1);  //切换到某个线程中执行
   std::ostream& dump(std::ostream& os);

   template<class Task>
    void schedule(Task&& task, int thread_id = -1) {
        bool need_tickle = false;
         {
              MutexType::Lock lock(m_mutex);
              need_tickle = scheduleNoLock(std::forward<Task>(task), thread_id);
         }

         if(need_tickle) {
            tickle();
         }
    }
    template<class Iterator>
    void schedule(Iterator begin, Iterator end) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            while(begin != end) {
                need_tickle = scheduleNolock(&*begin, -1) || need_tickle;
                ++ begin;
            }
        }
        if(need_tickle) {
            tickle();
        }
    }
private:
    template<class Task>
    void scheduleNoLock(Task&& task, int thread_id = -1) {
        bool need_tickle = m_tasks.empty();
        ExecuteTask et(std::forward<Task>(task), thread_id);
        if(et.fiber || et.cb) {
            m_tasks.emplace_back(std::move(et));
        }
        return need_tickle;
    }
protected:
    void run();
    virtual void tickle();
    virtual void idle();
    virtual bool stopping();
private:
    //Task: Fiber or function<void()> 
    struct ExecuteTask {
        Fiber::sptr fiber;
        std::function<void()> cb;
        
        int thread_id;   //在某个线程中执行

        ExecuteTask(Fiber::sptr f, int id) : fiber(f), thread_id(id) {}
        ExecuteTask(std::function<void()> f, int id) : cb(f), thread_id(id) {}
        ExecuteTask(Fiber::sptr* f, int id) : thread_id(id) {   //外部直接交出所有权，转移到ExecuteTask中
            fiber.swap(*f);
        }
        ExecuteTask(std::function<void()>* f, int id) : thread_id(id) {
            cb.swap(*f);
        }
        ExecuteTask() : thread_id(-1) {}

        void reset() {
            fiber = nullptr;
            cb = nullptr;
            thread_id = -1;
        }
    };
private:
    MutexType m_mutex;
    std::vector<Thread::sptr> m_threads;  //线程池
    std::list<ExecuteTask> m_tasks;       //任务队列
    Fiber::sptr m_rootFiber;              //主协程
    std::string m_name;                   //调度器名称
protected:
    std::vector<int> m_threadIds;         //线程id
    size_t m_threadCount = 0;             //线程数量
    std::atomic<size_t> m_activeThreadCount = 0;  //当前活跃线程数量
    std::atomic<size_t> m_idleThreadCount = 0;    //当前空闲线程数量
    bool m_stopping = true;               //是否正在停止
    bool m_autoStop = false;              //是否自动停止
    int m_rootThreadId = 0;                //主线程id    
};

}  // namespace East
