//
// Created by jxq on 20-2-24.
//

#include "timer.h"
#include "util.h"

namespace sylar {

    bool Timer::Comparator::operator()(const sylar::Timer::ptr &lhs, const sylar::Timer::ptr &rhs) const {
        if(!lhs && !rhs) {
            return false;
        }
        if(!lhs) {
            return true;
        }
        if(!rhs) {
            return false;
        }
        if(lhs->m_next < rhs->m_next) {
            return true;
        }
        if(rhs->m_next < lhs->m_next) {
            return false;
        }
        return lhs.get() < rhs.get();   // 看指针地址
    }

    Timer::Timer(uint64_t ms, std::function<void()> cb, bool recurring, sylar::TimerManager *manager)
        : m_recurring(recurring),
          m_ms(ms),
          m_cb(cb),
          m_manager(manager)
    {
        m_next = sylar::GetCurrentMS() + m_ms;
    }

    Timer::Timer(uint64_t next)
        : m_next(next)
    {

    }

    bool Timer::cancel()
    {
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if (m_cb) {
            m_cb = nullptr;
            auto it = m_manager->m_timers.find(shared_from_this());
            m_manager->m_timers.erase(it);
            return true;
        }
        return false;
    }

    // 往后挪，不会往前挪
    bool Timer::refresh() {
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if (!m_cb) {
            return false;
        }

        auto it = m_manager->m_timers.find(shared_from_this()); // 找到这个定时器
        if (it == m_manager->m_timers.end()) {
            return false;
        }

        m_manager->m_timers.erase(it);
        m_next = sylar::GetCurrentMS() + m_ms;
        m_manager->m_timers.insert(shared_from_this());
        return true;
    }

    bool Timer::reset(uint64_t ms, bool from_now)
    {
        if (ms == m_ms && !from_now) {  // 时间没有变的话
            return true;
        }

        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if (!m_cb) {
            return false;
        }
        auto it = m_manager->m_timers.find(shared_from_this());
        if (it == m_manager->m_timers.end()) {
            return false;
        }
        m_manager->m_timers.erase(it);
        uint64_t start = 0;
        if (from_now) {
            start = sylar::GetCurrentMS();
        } else {
            start = m_next - m_ms;
        }

        m_ms = ms;
        m_next = start + m_ms;
        m_manager->addTimer(shared_from_this(), lock);
        return true;
    }

    TimerManager::TimerManager() {
        m_previouseTime = sylar::GetCurrentMS();
    }

    TimerManager::~TimerManager() {
    }

    Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring) {
        Timer::ptr timer(new Timer(ms, cb, recurring, this));
        RWMutexType::WriteLock lock(m_mutex);
        addTimer(timer, lock);
        return timer;   // 返回方便取消
    }

    static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb) {
        std::shared_ptr<void> tmp = weak_cond.lock();
        if(tmp) {
            cb();
        }
    }

    Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond,
                                               bool recurring) {
        return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
    }

    uint64_t TimerManager::getNextTimer()
    {
        RWMutexType::ReadLock lock(m_mutex);
        m_tickled = false;
        if (m_timers.empty()) {
            return ~0ull;
        }

        const Timer::ptr& next = *m_timers.begin();
        uint64_t now_ms = sylar::GetCurrentMS();
        if (now_ms >= next->m_next) {
            return 0;
        } else {
            return next->m_next - now_ms;   // 返回还要等待时间
        }
    }

    void TimerManager::listExpiredCb(std::vector<std::function<void()> > &cbs)
    {
        uint64_t now_ms = sylar::GetCurrentMS();
        std::vector<Timer::ptr> expired;    // 超时数组
        {
            RWMutexType::ReadLock lock(m_mutex);
            if (m_timers.empty()) {
                return;
            }
        }

        RWMutexType::WriteLock lock(m_mutex);
        if (m_timers.empty()) {
            return;
        }
        bool rollover = detectClockRollover(now_ms);    // true 表明有问题
        if(!rollover && ((*m_timers.begin())->m_next > now_ms)) {   // 没有问题
            return;
        }

        Timer::ptr now_timer(new Timer(now_ms));    // 当前时间
        auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);  // true : 有问题全部处理掉
        // 等于的
        while(it != m_timers.end() && (*it)->m_next == now_ms) {
            ++it;
        }

        expired.insert(expired.begin(), m_timers.begin(), it);
        m_timers.erase(m_timers.begin(), it);
        cbs.reserve(expired.size());

        for(auto& timer : expired) {
            cbs.push_back(timer->m_cb);
            if(timer->m_recurring) {    // 循环事件
                timer->m_next = now_ms + timer->m_ms;
                m_timers.insert(timer);
            } else {
                timer->m_cb = nullptr;
            }
        }
    }

    void TimerManager::addTimer(sylar::Timer::ptr val, sylar::RWMutex::WriteLock &lock) {
        auto it = m_timers.insert(val).first;
        bool at_front = (it == m_timers.begin()) && !m_tickled; // 是否加到最前面
        if (at_front) {
            m_tickled = true;   // 防止频繁修改
        }
        lock.unlock();

        if (at_front) {
            onTimerInsertedAtFront();
        }
    }

    bool TimerManager::detectClockRollover(uint64_t now_ms) {
        bool rollover = false;
        if(now_ms < m_previouseTime &&
           now_ms < (m_previouseTime - 60 * 60 * 1000)) {
            rollover = true;    // 表明时间有问题
        }
        m_previouseTime = now_ms;
        return rollover;
    }

    bool TimerManager::hasTimer() {
        RWMutexType::ReadLock lock(m_mutex);
        return !m_timers.empty();
    }
}