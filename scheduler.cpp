//
// Created by jxq on 20-2-22.
//

#include "scheduler.h"
#include "fiber.h"
#include "log.h"
#include "macro.h"


namespace sylar {

    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    static thread_local Scheduler* t_scheduler = nullptr;
    static thread_local Fiber* t_scheduler_fiber = nullptr;

    Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name)
        : m_name(name)
    {
        SYLAR_ASSERT(threads > 0);  // 线程数量大于0

        if (use_caller) // 使用创建调度器的线程作为一个调用线程
        {
            // 创建主协程
            sylar::Fiber::GetThis();    // 有则返回  没有则初始化一个主协程
            --threads;

            SYLAR_ASSERT(GetThis() == nullptr);
            t_scheduler = this;

            // 创建子协程 (use_caller)
            m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
            sylar::Thread::SetName(m_name);

            t_scheduler_fiber = m_rootFiber.get();
            m_rootThread = sylar::GetThreadId();
            m_threadIds.push_back(m_rootThread);
        } else {
            m_rootThread = -1;
        }
        m_threadCount = threads;
    }

    Scheduler::~Scheduler(){
        SYLAR_ASSERT(m_stopping);
        if (GetThis() == this) {
            t_scheduler = nullptr;
        }
    }

    /**
     * @brief 返回当前协程调度器
     */
     Scheduler* Scheduler::GetThis(){
        return t_scheduler;
    }

    /**
     * @brief 返回当前协程调度器的调度协程
     */
    Fiber* Scheduler::GetMainFiber(){
        return t_scheduler_fiber;
    }

    // 启动协程调度器
    void Scheduler::start(){
        MutexType::Lock lock(m_mutex);
        if (!m_stopping)
        {
            return;
        }
        m_stopping = false;
        SYLAR_ASSERT(m_threads.empty());    // 保证线程池为空

        // 创建线程
        m_threads.resize(m_threadCount);
        for (size_t i = 0; i < m_threadCount; ++i)
        {
            m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this), m_name + "_" + std::to_string(i)));
            m_threadIds.push_back(m_threads[i]->getId());
        }
        lock.unlock();  // run函数里面有锁，所以只有这里释放了，其他线程才能run

//        if(m_rootFiber) {
//            //m_rootFiber->swapIn();
//            m_rootFiber->call();
//            SYLAR_LOG_INFO(g_logger) << "call out " << m_rootFiber->getState();
//        }
    }

    // 停止协程调度器
    void Scheduler::stop(){
        m_autoStop = true;
        if(m_rootFiber
           && m_threadCount == 0
           && (m_rootFiber->getState() == Fiber::TERM
               || m_rootFiber->getState() == Fiber::INIT)) {
            SYLAR_LOG_INFO(g_logger) << this << " stopped";
            m_stopping = true;

            if(stopping()) {
                return;
            }
        }

        //bool exit_on_this_fiber = false;
        if(m_rootThread != -1) {    // 调度器线程
            SYLAR_ASSERT(GetThis() == this);
        } else {
            SYLAR_ASSERT(GetThis() != this);
        }

        m_stopping = true;
        for(size_t i = 0; i < m_threadCount; ++i) {
            tickle();     // 类似于信号量，唤醒
        }

        if(m_rootFiber) {
            tickle();
        }

        if(m_rootFiber) {
            //while(!stopping()) {
            //    if(m_rootFiber->getState() == Fiber::TERM
            //            || m_rootFiber->getState() == Fiber::EXCEPT) {
            //        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
            //        SYLAR_LOG_INFO(g_logger) << " root fiber is term, reset";
            //        t_fiber = m_rootFiber.get();
            //    }
            //    m_rootFiber->call();
            //}
            if(!stopping()) {   // 如果主协程没有停止，则调用切换到主协程执行
                m_rootFiber->call();
            }
        }

        std::vector<Thread::ptr> thrs;
        {
            MutexType::Lock lock(m_mutex);
            thrs.swap(m_threads);
        }

        for(auto& i : thrs) {
            i->join();
        }
        //if(exit_on_this_fiber) {
        //}
    }

    void Scheduler::setThis() {
        t_scheduler = this;
    }

    void Scheduler::run() {
        SYLAR_LOG_DEBUG(g_logger) << m_name << " run";
        //set_hook_enable(true);
        setThis();  // 1. 设置当前调度器
        // 如果线程id不等于主线程id, 则创建主协程
        if (sylar::GetThreadId() != m_rootThread) {
            t_scheduler_fiber = Fiber::GetThis().get();
        }

        Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
        Fiber::ptr cb_fiber;

        FiberAndThread ft;
        // 协程调度循环
        while (true) {
            ft.reset();
            bool tickle_me = false;
            bool is_active = false;
            // 从队列中取出一个应该要执行的消息任务
            {
                MutexType::Lock lock(m_mutex);
                auto it = m_fibers.begin();
                while (it != m_fibers.end())
                {
                    // 设置的id不等于当前线程id,不做该任务
                    if (it->thread != -1 && it->thread != sylar::GetThreadId()) {
                        ++it;
                        tickle_me = true;   // 要通知其他线程
                        continue;
                    }

                    SYLAR_ASSERT(it->fiber || it->cb);  // 有协程 或者 回调函数, 任务非空
                    if (it->fiber && it->fiber->getState() == Fiber::EXEC) {    // 如果是协程，并且状态为执行完毕，则下一个
                        ++it;
                        continue;
                    }

                    ft = *it;
                    m_fibers.erase(it++);
                    ++m_activeThreadCount;
                    is_active = true;
                    break;
                }
                tickle_me |= it != m_fibers.end();
            }
            if (tickle_me) {
                tickle();
            }

            if (ft.fiber && (ft.fiber->getState() != Fiber::TERM
                         && ft.fiber->getState() != Fiber::EXCEPT)) {   // 调度器
                ft.fiber->swapIn(); // 执行
                --m_activeThreadCount;

                if (ft.fiber->getState() == Fiber::READY) {
                    schedule(ft.fiber); // 再次执行
                }
                else if (ft.fiber->getState() != Fiber::TERM
                    && ft.fiber->getState() != Fiber::EXCEPT) {
                    ft.fiber->m_state = Fiber::HOLD;
                }
                ft.reset();
            } else if (ft.cb) { // 回调函数
                if (cb_fiber) {
                    cb_fiber->reset(ft.cb);
                }
                else {
                    cb_fiber.reset(new Fiber(ft.cb));   // 创建一个协程任务
                }
                ft.reset();
                cb_fiber->swapIn();
                --m_activeThreadCount;
                if (cb_fiber->getState() == Fiber::READY) {
                    schedule(cb_fiber); // 再次执行
                    cb_fiber.reset();
                } else if (cb_fiber->getState() != Fiber::TERM
                           || cb_fiber->getState() != Fiber::EXCEPT) {
                    cb_fiber->reset(nullptr);
                } else {
                    cb_fiber->m_state = Fiber::HOLD;
                    cb_fiber.reset();
                }
            } else {    // 没有任务做
                if(is_active) {
                    --m_activeThreadCount;
                    continue;
                }
                if(idle_fiber->getState() == Fiber::TERM) {
                    SYLAR_LOG_INFO(g_logger) << "idle fiber term";
                    break;
                }

                ++m_idleThreadCount;
                idle_fiber->swapIn();
                --m_idleThreadCount;
                if(idle_fiber->getState() != Fiber::TERM
                   && idle_fiber->getState() != Fiber::EXCEPT) {
                    idle_fiber->m_state = Fiber::HOLD;
                }
            }
        }
    }

    void Scheduler::tickle() {
        SYLAR_LOG_INFO(g_logger) << "tickle";
    }

    bool Scheduler::stopping() {
        MutexType::Lock lock(m_mutex);
        return m_autoStop && m_stopping
               && m_fibers.empty() && m_activeThreadCount == 0;
    }

    void Scheduler::idle() {
        SYLAR_LOG_INFO(g_logger) << "idle";
        while(!stopping()) {
            sylar::Fiber::YieldToHold();
        }
    }


}