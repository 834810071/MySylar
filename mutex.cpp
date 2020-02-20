//
// Created by jxq on 20-2-20.
//

#include "mutex.h"
#include <error.h>
#include <stdexcept>
#include <semaphore.h>

namespace sylar
{
    Semaphore::Semaphore(uint32_t count)
    {
        // int sem_init(sem_t *sem, int pshared, unsigned int value);
        // 该函数初始化由 sem 指向的信号对象，并给它一个初始的整数值 value。
        // pshared 控制信号量的类型，值为 0 代表该信号量用于多线程间的同步，值如果大于 0 表示可以共享，用于多个相关进程间的同步
        if (sem_init(&m_semaphore, 0, count))   // 创建信号量
        {
            throw std::logic_error("sem_init error");
        }
    }

    Semaphore::~Semaphore()
    {
        sem_destroy(&m_semaphore);  // 用于对用完的信号量的清理
    }

    void Semaphore::wait()
    {
        // sem_wait 是一个阻塞的函数，测试所指定信号量的值，它的操作是原子的。
        // 若 sem value > 0，则该信号量值减去 1 并立即返回。若sem value = 0，则阻塞直到 sem value > 0，此时立即减去 1，然后返回。
        if (sem_wait(&m_semaphore))
        {
            throw std::logic_error("sem_wait error");
        }
    }

    void Semaphore::notify()
    {
        // 把指定的信号量 sem 的值加 1，唤醒正在等待该信号量的任意线程。
        if (sem_post(&m_semaphore))
        {
            throw std::logic_error("sem_post error");
        }
    }

}