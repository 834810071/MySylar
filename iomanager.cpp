//
// Created by jxq on 20-2-24.
//

#include "iomanager.h"
#include "log.h"
#include "macro.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <string.h>
#include <unistd.h>

namespace sylar {
    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    IOManager::FdContext::EventContext& IOManager::FdContext::getContext(sylar::IOManager::Event event)
    {
        switch (event) {
            case IOManager::READ:
                return read;
            case IOManager::WRITE:
                return write;
            default:
                SYLAR_ASSERT2(false, "getContext");
        }
        throw std::invalid_argument("getContext invalid event");
    }

    void IOManager::FdContext::resetContext(EventContext& ctx) {
        ctx.scheduler = nullptr;
        ctx.fiber.reset();
        ctx.cb = nullptr;
    }

    // 触发事件
    void IOManager::FdContext::triggerEvent(sylar::IOManager::Event event)
    {
        SYLAR_ASSERT(event & events);
        events = (Event)(events & ~event);  // 取消事件
        EventContext& ctx = getContext(event);
        if (ctx.cb) {
            ctx.scheduler->schedule(&ctx.cb);
        } else
        {
            ctx.scheduler->schedule(&ctx.fiber);
        }
        ctx.scheduler =  nullptr;
        return;
    }

    IOManager::IOManager(size_t threads, bool use_caller, const std::string &name)
        : Scheduler(threads, use_caller, name)
    {
        m_epfd = epoll_create(5000);
        SYLAR_ASSERT(m_epfd > 0);

        int rt = pipe(m_tickleFds); // 调用pipe()函数就已将管道打开
        SYLAR_ASSERT(!rt);

        epoll_event event;
        memset(&event, 0, sizeof(epoll_event));
        event.data.fd = m_tickleFds[0];
        event.events = EPOLLIN | EPOLLET;   // 输入 + 边缘触发

        rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
        SYLAR_ASSERT(!rt);

        rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
        SYLAR_ASSERT(!rt);

        contextResize(32);

        start();
    }

    IOManager::~IOManager() {
        stop();
        close(m_epfd);
        close(m_tickleFds[0]);
        close(m_tickleFds[1]);

        for (size_t i = 0; i < m_fdContexts.size(); ++i)
        {
            if (m_fdContexts[i])
            {
                delete m_fdContexts[i];
            }
        }
    }

    void IOManager::contextResize(size_t size) {
        m_fdContexts.resize(size);

        for (size_t i = 0; i < m_fdContexts.size(); ++i)
        {
            if (!m_fdContexts[i]) {
                m_fdContexts[i] = new FdContext;
                m_fdContexts[i]->fd = i;
            }
        }
    }

    int IOManager::addEvent(int fd, sylar::IOManager::Event event, std::function<void()> cb)
    {
        FdContext* fd_ctx = nullptr;
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() > fd) {
            fd_ctx = m_fdContexts[fd];
            lock.unlock();
        } else {
            lock.unlock();
            RWMutexType::WriteLock lock2(m_mutex);
            contextResize(fd * 1.5);    // 扩容
            fd_ctx = m_fdContexts[fd];
        }

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        // 事件已经加过了
        if (SYLAR_UNLIKELY(fd_ctx->events & event)) {
            SYLAR_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd
                                      << " event=" << (EPOLL_EVENTS)event
                                      << " fd_ctx.event=" << (EPOLL_EVENTS)fd_ctx->events;
            SYLAR_ASSERT(!(fd_ctx->events & event));
        }

        int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        epoll_event epevent;
        epevent.events = EPOLLET | fd_ctx->events | event;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if(rt) {
            SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                      << op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                                      << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
                                      << (EPOLL_EVENTS)fd_ctx->events;
            return -1;
        }

        ++m_pendingEventCount;  // 等待事件++
        fd_ctx->events = (Event)(fd_ctx->events | event);   // 设置事件
        FdContext::EventContext& event_ctx = fd_ctx->getContext(event); // 获取事件 是读 还是 写 还是 空
        SYLAR_ASSERT(!event_ctx.scheduler
                     && !event_ctx.fiber
                     && !event_ctx.cb);

        event_ctx.scheduler = Scheduler::GetThis();
        if(cb) {
            event_ctx.cb.swap(cb);
        } else {
            event_ctx.fiber = Fiber::GetThis();
            SYLAR_ASSERT2(event_ctx.fiber->getState() == Fiber::EXEC, "state=" << event_ctx.fiber->getState()); // ??
        }
        return 0;
    }

    bool IOManager::delEvent(int fd, sylar::IOManager::Event event)
    {
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() <= fd){
            return false;
        }
        FdContext* fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if(SYLAR_UNLIKELY(!(fd_ctx->events & event))) { // 没有这个事件
            return false;
        }

        Event new_events = (Event)(fd_ctx->events & ~event);    // 新的事件作废
        int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = EPOLLET | new_events;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if(rt) {
            SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                      << op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                                      << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }

        --m_pendingEventCount;
        fd_ctx->events = new_events;
        FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
        fd_ctx->resetContext(event_ctx);    // 清空
        return true;
    }

    bool IOManager::cancelEvent(int fd, sylar::IOManager::Event event)
    {
        RWMutexType::ReadLock lock(m_mutex);
        if((int)m_fdContexts.size() <= fd) {
            return false;
        }
        FdContext* fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if(SYLAR_UNLIKELY(!(fd_ctx->events & event))) {
            return false;
        }

        Event new_events = (Event)(fd_ctx->events & event);
        int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = EPOLLET | new_events;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if(rt) {
            SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                      << op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                                      << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }

        fd_ctx->triggerEvent(event);
        --m_pendingEventCount;
        return true;
    }

    bool IOManager::cancelAll(int fd)
    {
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() < fd) {
            return false;
        }
        FdContext* fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if (!fd_ctx->events)
        {
            return false;
        }

        int op = EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = 0;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt) {
            SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                      << op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                                      << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }

        if (fd_ctx->events & READ) {
            fd_ctx->triggerEvent(READ);
            --m_pendingEventCount;
        }
        if(fd_ctx->events & WRITE) {
            fd_ctx->triggerEvent(WRITE);
            --m_pendingEventCount;
        }

        SYLAR_ASSERT(fd_ctx->events == 0);
        return true;
    }

    IOManager* IOManager::GetThis()
    {
        return dynamic_cast<IOManager*>(Scheduler::GetThis());
    }

    void IOManager::tickle()
    {
        if (!hasIdleThreads()) {   // 如果没有空闲事件，因为发送需要闲置线程来处理
            return ;
        }
        int rt = write(m_tickleFds[1], "T", 1); // 通过写数据来触发
        SYLAR_ASSERT(rt == 1);
    }

    bool IOManager::stopping(uint64_t& timeout)
    {
        timeout = getNextTimer(); // 获取下次执行的定时器任务
        return  timeout == ~0ull    // 无穷大
               && m_pendingEventCount == 0  // 剩余需要处理的消息为0
               && Scheduler::stopping();
    }

    bool IOManager::stopping() {
        uint64_t timeout = 0;
        return stopping(timeout);
    }

    void IOManager::idle()
    {
        SYLAR_LOG_DEBUG(g_logger) << "idle";
        const uint64_t MAX_EVENTS = 256;
        epoll_event* events = new epoll_event[MAX_EVENTS]();
        std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr) {   // 析构方法
            delete[] ptr;
        });

        while (true) {
            uint64_t next_timeout = 0;
            if(SYLAR_UNLIKELY(stopping(next_timeout))) {    // 如果结束了
                SYLAR_LOG_INFO(g_logger) << "name=" << getName()
                                         << " idle stopping exit";
                break;
            }

            int rt = 0;
            do {
                static const int MAX_TIMEOUT = 3000;    // 超时时间
                if(next_timeout != ~0ull) {
                    next_timeout = (int)next_timeout > MAX_TIMEOUT
                                   ? MAX_TIMEOUT : next_timeout;
                } else {
                    next_timeout = MAX_TIMEOUT;
                }
                rt = epoll_wait(m_epfd, events, MAX_EVENTS, (int)next_timeout);
                if(rt < 0 && errno == EINTR) {  // 如果没有协程需要执行， 则循环
                } else {
                    break;  // 有事件返回
                }
            } while (true);

            std::vector<std::function<void()> > cbs;
            listExpiredCb(cbs); // 获取超时任务
            if (!cbs.empty()) {
                schedule(cbs.begin(), cbs.end());   // 函数添加到调度器中
                cbs.clear();
            }

            for (int i = 0; i < rt; ++i) {
                epoll_event& event = events[i];
                if (event.data.fd == m_tickleFds[0]) {  // 唤醒
                    uint8_t dummy[256];
                    while (read(m_tickleFds[0], dummy, sizeof(dummy)) > 0) {

                    }
                    continue;
                }

                FdContext* fd_ctx = (FdContext*)event.data.ptr;
                FdContext::MutexType::Lock lock(fd_ctx->mutex);
                if (event.events & (EPOLLERR | EPOLLHUP)) { // 错误 或者 中断挂起
                    event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
                }
                int real_events = NONE;
                if (event.events & EPOLLIN) {
                    real_events |= READ;
                }
                if(event.events & EPOLLOUT) {
                    real_events |= WRITE;
                }

                if((fd_ctx->events & real_events) == NONE) {    // 没有交集 --> 没有事件
                    continue;
                }

                int left_events = (fd_ctx->events & ~real_events);  // 剩余事件
                int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
                event.events = EPOLLET | left_events;

                int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
                if (rt2) {
                    SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                              << op << ", " << fd_ctx->fd << ", " << (EPOLL_EVENTS)event.events << "):"
                                              << rt2 << " (" << errno << ") (" << strerror(errno) << ")";
                    continue;
                }

                // 触发事件
                if(real_events & READ) {
                    fd_ctx->triggerEvent(READ);
                    --m_pendingEventCount;
                }
                if(real_events & WRITE) {
                    fd_ctx->triggerEvent(WRITE);
                    --m_pendingEventCount;
                }
            }

            // 让出执行时间cpu
            Fiber::ptr cur = Fiber::GetThis();  // idle
            auto raw_ptr = cur.get();
            cur.reset();

            raw_ptr->swapOut();
        }
    }

    void IOManager::onTimerInsertedAtFront() {
        tickle();
    }

}
