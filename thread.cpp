//
// Created by jxq on 20-2-20.
//

#include "thread.h"
#include "log.h"

namespace sylar
{
    static thread_local Thread* t_thread = nullptr; // 在线程开始的时候被生成(allocated)，在线程结束的时候被销毁(deallocated)

    static thread_local std::string t_thread_name = "UNKNOW";

    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");  // 只能在本文件中访问

    Thread::Thread(std::function<void()> cb, const std::string& name)
        : m_cb(cb), m_name(name)
    {
        if (name.empty())
        {
            m_name = "UNKNOW";
        }
        int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);    // 创建线程
        if (rt)
        {
            SYLAR_LOG_ERROR(g_logger) << "pthread_create thread fail, rt=" << rt
                                      << " name=" << name;
            throw std::logic_error("pthread_create error");
        }
        m_semaphore.wait(); // p操作
    }

    Thread::~Thread()
    {
        if (m_thread)
        {
            pthread_detach(m_thread);   // 将该线程的状态设置为detached,则该线程运行结束后会自动释放所有资源。
        }
    }

    void Thread::join()
    {
        if (m_thread)
        {
            int rt = pthread_join(m_thread, nullptr);
            if(rt) {
                SYLAR_LOG_ERROR(g_logger) << "pthread_join thread fail, rt=" << rt
                                          << " name=" << m_name;
                throw std::logic_error("pthread_join error");
            }
            m_thread = 0;
        }
    }

    Thread* Thread::GetThis()
    {
        return t_thread;
    }

    const std::string& Thread::GetName()
    {
        return t_thread_name;
    }

    void* Thread::run(void* arg)
    {
        Thread* thread = (Thread*)arg;
        t_thread = thread;
        t_thread_name = thread->m_name;
        thread->m_id = sylar::GetThreadId();
        pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());   // 设置线程名称

        std::function<void()> cb;
        cb.swap(thread->m_cb);

        thread->m_semaphore.notify();   // v操作

        cb();
        return 0;
    }
}
