//
// Created by jxq on 20-2-20.
//

#ifndef MYSYLAR_THREAD_H
#define MYSYLAR_THREAD_H

#include <thread>
#include <functional>
#include <memory>
#include <pthread.h>
#include "mutex.h"

// pthread_xxx
// std::thread, pthread

namespace sylar
{

    class Thread {
    public:
        typedef std::shared_ptr<Thread> ptr;
        Thread(std::function<void()> cb, const std::string& name);
        ~Thread();

        pid_t getId() const { return m_id; }
        const std::string& getName() const { return m_name; }

        void join();

        static Thread* GetThis();
        static const std::string& GetName();
        /**
         * @brief 设置当前线程名称
         * @param[in] name 线程名称
         */
        static void SetName(const std::string& name);


    private:
        Thread(const Thread&) = delete;
        Thread(const Thread&&) = delete;
        Thread& operator=(const Thread&) = delete;

        // 线程执行函数
        static void* run(void* arg);

    private:
        pid_t m_id = -1;    // 线程所在的进程id
        pthread_t m_thread; // 线程结构
        std::function<void()> m_cb; // 线程执行函数
        std::string m_name; // 线程名称
        Semaphore m_semaphore; // 信号量
    };
};

#endif //MYSYLAR_THREAD_H
