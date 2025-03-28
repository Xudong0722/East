#include "Scheduler.h"
#include "Elog.h"
#include "Macro.h"

namespace East {

static East::Logger::sptr g_logger = EAST_LOG_NAME("system");

static thread_local Scheduler* t_scheduler = nullptr;    //当前线程的调度器
static thread_local Fiber* t_scheduler_fiber = nullptr;  //当前调度器的主协程

Scheduler::Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "") {
    EAST_ASSERT2(threads > 0, "threads must be at least 1");

    //user_caller: 是否使用当前调用线程
    if(use_caller) {
        East::Fiber::GetThis();  //如果当前线程没有协程，会创建一个协程
        --threads;               //当前调用线程参与，所以线程数减一

        EAST_ASSERT2(GetThis() == nullptr, "Scheduler has been created");  //一个线程只能有一个调度器
        t_scheduler = this;

        m_rootFiber = std::make_shared<Fiber>(std::bind(&Scheduler::run, this));
        East::Thread::SetName(m_name);
        m_rootThreadId = East::GetThreadId();
        m_threadIds.push_back(m_rootThreadId);
    }else{
        m_rootThreadId = -1;
    }
    m_threadCount = threads;
}

Scheduler::~Scheduler() {
    EAST_ASSERT(m_stopping);
    if(GetThis() == this) {
        GetThis() == nullptr;
    }
}

//获取当前线程的协程调度器
Scheduler* Scheduler::GetThis() {
    return t_scheduler;
}

//获取当前协程调度器的主协程
Fiber* Scheduler::GetMainFiber() {
    return t_scheduler_fiber;
}

void Scheduler::setThis() {
    t_scheduler = this;
}

void Scheduler::start() {
    MutexType::LockGuard lock(m_mutex);

    if(!m_stopping) return ;
    m_stopping = false;
    EAST_ASSERT2(m_threads.empty(), "m_threads is not empty");
    m_threads.resize(m_threadCount);

    for(size_t i = 0; i < m_threadCount; ++i) {
        m_threads[i].reset(new Thread(m_name + "_" + std::to_string(i),
            std::bind(&Scheduler::run, this)));
        m_threadIds.emplace_back(m_threads[i]->getId());
    }
}

void Scheduler::stop() {

}

void Scheduler::run() {
    ELOG_INFO(g_logger) << "run";
    setThis();
    

    if(East::GetThreadId() != m_rootThreadId) {
        t_scheduler_fiber = Fiber::GetThis().get();  //如果不是scheduer的主线程，就把当前协程设置为主协程
    }

    Fiber::sptr idle_fiber = std::make_shared<Fiber>(std::bind(&Scheduler::idle, this));
    Fiber::sptr cb_fiber{nullptr};   //用于执行回调函数

    ExecuteTask task{};
    while(true) {
        task.reset();

        bool tickle_me = false;
        bool is_active = false;
        {
            MutexType::LockGuard lock(m_mutex);
            auto it = m_tasks.begin();
            while(it != m_tasks.end()) {
                //如果是指定线程执行，且不是当前线程，就跳过
                if(it->thread_id != -1 && it->thread_id != East::GetThreadId()) {
                    ++ it;
                    tickle_me = true;
                    continue;
                }

                EAST_ASSERT(it->fiber || it->cb);
                //如果是协程且协程已经在执行，也直接跳过
                if(it->fiber && it->fiber->getState() == Fiber::EXEC) {
                    ++ it;
                    continue;
                }

                task = *it;
                it = m_tasks.erase(it);
                ++m_activeThreadCount;
                is_active = true;
                break;
            }
            tickle_me = tickle_me || it != m_tasks.end();
        }
        if(tickle_me) {
            tickle();
        }

        //如果协程的状态可以执行，则执行
        if(task.fiber != nullptr && (task.fiber->getState() != Fiber::TERM
                            && task.fiber->getState != Fiber::EXCEPT)) {
            task.fiber->swapIn();
            --m_activeThreadCount;

            //执行完之后如果协程还是ready状态，就放回到任务队列中
            if(task.fiber->getState() == Fiber::READY) {
                schedule(task.fiber);
            }else if(task.fiber->getState() != Fiber::TERM
                        && task.fiber->getState() != Fiber::EXCEPT) {
                task.fiber->setState(Fiber::HOLD);
            }
            task.reset();
        }else if(task.cb != nullptr) {
            if(cb_fiber != nullptr) {
                cb_fiber->reset(task.cb);
            }else {
                cb_fiber = std::make_shared<Fiber>(task.cn);
            }
            task.reset();
            cb_fiber->swapIn();
            --m_activeThreadCount;
            if(cb_fiber->getState() == Fiber::READY) {
                schedule(cb_fiber);
                cb_fiber.reset();
            }else if(cb_fiber->getState() == Fiber::EXCEPT
                    || cb_fiber->getState() == Fiber::TERM) {
                cb_fiber->reset(nullptr);
            }else {
                cb_fiber->setState(Fiber::HOLD);  //meaningless?
                cb_fiber.reset();
            }
        }else {
            if(is_active) {
                --m_activeThreadCount;  //TODO:why thread count minus one?
                continue;
            }

            if(idle_fiber->getState() == Fiber::TERM) {
                ELOG_INFO(g_logger) << "idle fiber term";
                break;
            }

            ++m_idleThreadCount;
            idle_fiber->swapIn();
            --m_idleThreadCount;
            if(idle_fiber->getState() != Fiber::TERM
                && idle_fiber->getState() != Fiber::EXCEPT) {
                idle_fiber->setState(Fiber::HOLD);
            }
        }
    }
}

void Scheduler::tickle() {
    ELOG_INFO(g_logger) << "tickle";
}

void Scheduler::idle() {

}

bool Scheduler::stopping() {

}

//切换到某个线程中执行
void Scheduler::switchTo(int thread_id = -1) {

}

std::ostream& Scheduler::dump(std::ostream& os) {

}

} // namespace East
