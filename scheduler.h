//
// Created by jxq on 20-2-22.
//

#ifndef MYSYLAR_SCHEDULER_H
#define MYSYLAR_SCHEDULER_H

#include "fiber.h"
#include "thread.h"
#include <list>
#include <memory>
#include <vector>
#include <iostream>

namespace sylar {
    /**
     * @brief 协程调度器
     * @details 封装的是N-M的协程调度器
     *          内部有一个线程池,支持协程在线程池里面切换
     */
    class Scheduler {
    public:
        typedef std::shared_ptr<Scheduler> ptr;
        typedef Mutex MutexType;

        /**
         * @brief 构造函数
         * @param[in] threads 线程数量
         * @param[in] use_caller 是否使用当前调用线程
         * @param[in] name 协程调度器名称
         */
        Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");

        ~Scheduler();

        const std::string& getName() const { return m_name; }

        /**
         * @brief 返回当前协程调度器
         */
        static Scheduler* GetThis();

        /**
         * @brief 返回当前协程调度器的调度协程
         */
        static Fiber* GetMainFiber();

        void start();   // 启动协程调度器

        void stop();    // 停止协程调度器

        /**
         * @brief 调度协程
         * @param[in] fc 协程或函数
         * @param[in] thread 协程执行的线程id,-1标识任意线程
         */
        template <class FiberOrCb>
        void schedule(FiberOrCb fc, int thread = -1)
        {
            bool need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                need_tickle = scheduleNoLock(fc, thread);
            }
            if (need_tickle) {
                tickle();
            }
        }

        /**
         * @brief 批量调度协程
         * @param[in] begin 协程数组的开始
         * @param[in] end 协程数组的结束
         */
        template <class InputIterator>
        void schedule(InputIterator begin, InputIterator end) {
            bool need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                while (begin != end)
                {
                    need_tickle = sheduleNoLock(&*begin, -1) || need_tickle;
                    ++begin;
                }
                if (need_tickle)
                {
                    tickle();
                }
            }
        }

    protected:
        /**
         * @brief 通知协程调度器有任务了
         */
        virtual void tickle();
        /**
         * @brief 协程调度函数
         */
        void run();
        /**
         * @brief 返回时否可以停止
         */
        virtual bool stopping();

        /**
         * @brief 协程无任务可调度时执行idle协程
         */
        virtual void idle();

        /**
         * @brief 设置当前的协程调度器
         */
        void setThis();

        /**
         * @brief 是否有空闲线程
         */
        bool hasIdleThreads() { return m_idleThreadCount > 0;}

    private:
        /**
         * @brief 协程调度启动(无锁)
         */
        template <class FiberOrCb>
        bool scheduleNoLock(FiberOrCb fc, int thread) {
            bool need_tickle = m_fibers.empty();
            FiberAndThread ft(fc, thread);
            if (ft.fiber || ft.cb)
            {
                m_fibers.push_back(ft);
            }
            return need_tickle;
        }

    private:
        /**
         * @brief 协程/函数/线程组
         */
         struct FiberAndThread {
            // 协程
            Fiber::ptr fiber;
            // 协程执行函数
            std::function<void ()> cb;
            // 线程id
            int thread;

            FiberAndThread(Fiber::ptr f, int thr)
                : fiber(f), thread(thr)
            {

            }
            /**
             * @brief 构造函数
             * @param[in] f 协程指针
             * @param[in] thr 线程id
             * @post *f = nullptr
             */
             FiberAndThread(Fiber::ptr* f, int thr)
             :thread(thr) {
                     fiber.swap(*f);
             }

             /**
              * @brief 构造函数
              * @param[in] f 协程执行函数
              * @param[in] thr 线程id
              */
             FiberAndThread(std::function<void()> f, int thr)
             :cb(f), thread(thr) {
             }

             /**
              * @brief 构造函数
              * @param[in] f 协程执行函数指针
              * @param[in] thr 线程id
              * @post *f = nullptr
              */
             FiberAndThread(std::function<void()>* f, int thr)
             :thread(thr) {
                     cb.swap(*f);
             }

             /**
              * @brief 无参构造函数
              */
             FiberAndThread()
                     :thread(-1) {
             }

             /**
              * @brief 重置数据
              */
             void reset() {
                 fiber = nullptr;
                 cb = nullptr;
                 thread = -1;
             }

         };
    private:
        MutexType m_mutex;
        std::vector<Thread::ptr> m_threads; // 线程池
        std::list<FiberAndThread>   m_fibers;   // 待执行的协程队列
        Fiber::ptr m_rootFiber;     // use_caller为true时有效，调度协程
        std::string m_name; // 协程调度器名称
    protected:
        std::vector<int> m_threadIds;   // 协程下的线程id数组
        size_t m_threadCount = 0;   // 线程数量
        std::atomic<size_t> m_activeThreadCount = {0};  // 工作线程数量
        std::atomic<size_t> m_idleThreadCount = {0};    // 空闲线程数量
        bool m_stopping = true; // 是否正在停止
        bool m_autoStop = false;    // 是否自动停止
        int m_rootThread = 0;   // 主线程id(use_caller)
    };
};


#endif //MYSYLAR_SCHEDULER_H
