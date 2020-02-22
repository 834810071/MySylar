//
// Created by jxq on 20-2-20.
//

#ifndef MYSYLAR_MUTEX_H
#define MYSYLAR_MUTEX_H

#include <semaphore.h>
#include <unistd.h>
#include <cstdint>
#include <pthread.h>
#include <atomic>

namespace sylar
{
    // 信号量
    // 信号量的使用主要是用来保护共享资源，使得资源在一个时刻只有一个进程（线程）所拥有。
    // 为正的时候表示空闲        等待（即P(信号变量))和发送（即V(信号变量))信息操作
    // https://blog.csdn.net/qq_19923217/article/details/82902442
    class Semaphore {
    public:
        // 构造量
        Semaphore(uint32_t count = 0);    // 信号量的大小

        ~Semaphore();

        void wait();    // 获取信号量

        void notify();  // 释放信号量

    private:
        Semaphore(const Semaphore&) = delete;
        Semaphore(const Semaphore&&) = delete;
        Semaphore& operator=(const Semaphore&) = delete;

    private:
        sem_t m_semaphore;
    };

    // 局部锁的模版实现
    template <class T>
    struct ScopedLockImpl {
    public:
        ScopedLockImpl(T& mutex)
            : m_mutex(mutex)
        {
            m_mutex.lock();
            m_locked = true;
        }

        ~ScopedLockImpl()
        {
            unlock();
        }

        // 加锁
        void lock() {
            if (!m_locked)
            {
                m_mutex.lock();
                m_locked = true;
            }
        }

        // 解锁
        void unlock() {
            if (m_locked)
            {
                m_mutex.unlock();
                m_locked = false;
            }
        }
    private:
        T& m_mutex;
        bool m_locked;  // 是否上锁
    };

    // 局部读锁模版实现
    template <class T>
    struct ReadScopedLockImpl {
    public:
        /**
     * @brief 构造函数
     * @param[in] mutex 读写锁
     */
        ReadScopedLockImpl(T& mutex)
                :m_mutex(mutex) {
            m_mutex.rdlock();
            m_locked = true;
        }

        /**
         * @brief 析构函数,自动释放锁
         */
        ~ReadScopedLockImpl() {
            unlock();
        }

        /**
         * @brief 上读锁
         */
        void lock() {
            if(!m_locked) {
                m_mutex.rdlock();
                m_locked = true;
            }
        }

        /**
         * @brief 释放锁
         */
        void unlock() {
            if(m_locked) {
                m_mutex.unlock();
                m_locked = false;
            }
        }

    private:
        T& m_mutex;
        bool m_locked;
    };

    /**
     * @brief 局部写锁模板实现
     */
    template<class T>
    struct WriteScopedLockImpl {
    public:
        /**
         * @brief 构造函数
         * @param[in] mutex 读写锁
         */
        WriteScopedLockImpl(T& mutex)
                :m_mutex(mutex) {
            m_mutex.wrlock();
            m_locked = true;
        }

        /**
         * @brief 析构函数
         */
        ~WriteScopedLockImpl() {
            unlock();
        }

        /**
         * @brief 上写锁
         */
        void lock() {
            if(!m_locked) {
                m_mutex.wrlock();
                m_locked = true;
            }
        }

        /**
         * @brief 解锁
         */
        void unlock() {
            if(m_locked) {
                m_mutex.unlock();
                m_locked = false;
            }
        }
    private:
        /// Mutex
        T& m_mutex;
        /// 是否已上锁
        bool m_locked;
    };

    // 互斥量
    class Mutex {
    public:
        typedef ScopedLockImpl<Mutex> Lock;

        Mutex() {
            pthread_mutex_init(&m_mutex, nullptr);
        }

        ~Mutex()
        {
            pthread_mutex_destroy(&m_mutex);
        }

        void lock()
        {
            pthread_mutex_lock(&m_mutex);
        }

        void unlock() {
            pthread_mutex_unlock(&m_mutex);
        }

    private:
        pthread_mutex_t m_mutex;
    };

    /**
     * @brief 空锁(用于调试)
     */
    class NullMutex {
    public:
        /// 局部锁
        typedef ScopedLockImpl<NullMutex> Lock;

        /**
         * @brief 构造函数
         */
        NullMutex() {}

        /**
         * @brief 析构函数
         */
        ~NullMutex() {}

        /**
         * @brief 加锁
         */
        void lock() {}

        /**
         * @brief 解锁
         */
        void unlock() {}
    };

    // 读写互斥量
    // https://www.cnblogs.com/x_wukong/p/5671537.html
    class RWMutex {
    public:

        /// 局部读锁
        typedef ReadScopedLockImpl<RWMutex> ReadLock;

        /// 局部写锁
        typedef WriteScopedLockImpl<RWMutex> WriteLock;

        /**
         * @brief 构造函数
         */
        RWMutex() {
            pthread_rwlock_init(&m_lock, nullptr);
        }

        /**
         * @brief 析构函数
         */
        ~RWMutex() {
            pthread_rwlock_destroy(&m_lock);
        }

        /**
         * @brief 上读锁
         */
        void rdlock() {
            pthread_rwlock_rdlock(&m_lock);
        }

        /**
         * @brief 上写锁
         */
        void wrlock() {
            pthread_rwlock_wrlock(&m_lock);
        }

        /**
         * @brief 解锁
         */
        void unlock() {
            pthread_rwlock_unlock(&m_lock);
        }
    private:
        /// 读写锁
        pthread_rwlock_t m_lock;
    };

    /**
     * @brief 空读写锁(用于调试)
     */
    class NullRWMutex {
    public:
        /// 局部读锁
        typedef ReadScopedLockImpl<NullMutex> ReadLock;
        /// 局部写锁
        typedef WriteScopedLockImpl<NullMutex> WriteLock;

        /**
         * @brief 构造函数
         */
        NullRWMutex() {}
        /**
         * @brief 析构函数
         */
        ~NullRWMutex() {}

        /**
         * @brief 上读锁
         */
        void rdlock() {}

        /**
         * @brief 上写锁
         */
        void wrlock() {}
        /**
         * @brief 解锁
         */
        void unlock() {}
    };

    // 自旋锁  高效
    // 当线程A想要获取一把自旋锁而该锁又被其它线程锁持有时，线程A会在一个循环中自旋以检测锁是不是已经可用了
    class Spinlock {
    public:
        typedef ScopedLockImpl<Spinlock> Lock;

        Spinlock() {
            pthread_spin_init(&m_mutex, 0);
        }

        ~Spinlock() {
            pthread_spin_destroy(&m_mutex);
        }

        void lock() {
            pthread_spin_lock(&m_mutex);
        }

        void unlock() {
            pthread_spin_unlock(&m_mutex);
        }
    private:
        pthread_spinlock_t m_mutex;
    };

    // 原子锁
    class CASLock {
    public:
        typedef ScopedLockImpl<CASLock> Lock;

        CASLock() {
            m_mutex.clear();    // 清除 std::atomic_flag 对象的标志位，即设置 atomic_flag 的值为 false
        }

        ~CASLock() {

        }

        void lock() {
            // 检查std::atomic_flag标志,并返回 std::atomic_flag 的旧值
            while (std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acquire));
        }

        void unlock() {
            // 原子地将flag设置成false（函数）
            std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
        }

    private:
        // 原子状态
        volatile std::atomic_flag m_mutex;
    };
};


#endif //MYSYLAR_MUTEX_H
