//
// Created by jxq on 20-2-22.
//


#include <functional>
#include <ucontext.h>   // 协程库
#include "fiber.h"
#include "sylar.h"

namespace sylar{

    sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    static std::atomic<uint64_t> s_fiber_id {0};
    static std::atomic<uint64_t> s_fiber_count {0}; // 协程数

    static thread_local Fiber* t_fiber = nullptr;   // 当前协程
    static thread_local Fiber::ptr t_threadFiber = nullptr; // 旧的协成

    static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
            Config::Lookup<uint32_t >("fiber.stack_size", 128*1024, "fiber stack size");

    class MallocStackAlloctor {
    public:
        static void* Alloc(size_t size) {
            return malloc(size);
        }

        static void Dealloc(void* vp, size_t size) {
            return free(vp);
        }
    };

    using StackAllocator = MallocStackAlloctor;

    // 主协程
    Fiber::Fiber()
    {
        m_state = EXEC;
        SetThis(this);  // 设置到线程变量 t_fiber;

        // 该函数初始化ucp所指向的结构体ucontext_t(用来保存前执行状态上下文)，填充当前有效的上下文
        // 获取当前context
        if (getcontext(&m_ctx))
        {
            SYLAR_ASSERT2(false, "getcontext");
        }

        ++s_fiber_count;
        SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber main";
    }

    // 子协程 需要分配栈
    Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
        : m_id(++s_fiber_id),
          m_cb(cb)
    {
        ++s_fiber_count;
        m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();

        // 栈生成
        m_stack = StackAllocator::Alloc(m_stacksize);   // 申请空间
        if (getcontext(&m_ctx))
        {
            SYLAR_ASSERT2(false, "getcontext");
        }

        m_ctx.uc_link = nullptr;    // 指向当前的上下文结束时要恢复到的上下文
        // 该上下文中使用的栈
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;

        if (!use_caller) {
            makecontext(&m_ctx, &Fiber::MainFunc, 0);   // 用于将一个新函数和堆栈，绑定到指定context中
        }
        else {  // 在MainFiber上调度
            makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
        }
        SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber id = " << m_id;
    }


    Fiber::~Fiber()
    {
        --s_fiber_count;
        if (m_stack)
        {
            SYLAR_ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);
            StackAllocator::Dealloc(m_stack, m_stacksize);
        } else {
            // 主协程
            SYLAR_ASSERT(!m_cb);
            SYLAR_ASSERT(m_state == EXEC);

            Fiber* cur = t_fiber;
            if (cur == this) {
                SetThis(this);
            }
        }
        SYLAR_LOG_DEBUG(g_logger) << "Fiber::~Fiber id=" << m_id
                                  << " total=" << s_fiber_count;
    }

    /**
     * @brief 重置协程执行函数,并设置状态
     * @pre getState() 为 INIT, TERM, EXCEPT
     * @post getState() = INIT
     */
    void Fiber::reset(std::function<void()> cb)
    {
        SYLAR_ASSERT(m_stack);
        SYLAR_ASSERT(m_state == TERM
            || m_state == EXCEPT
            || m_state == INIT);
        m_cb = cb;
        if (getcontext(&m_ctx)) // 获取上下文 重置
        {
            SYLAR_ASSERT2(false, "getcontext");
        }
        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;

        makecontext(&m_ctx, &Fiber::MainFunc, 0);
        m_state = INIT;
    }

    /**
    * @brief 将当前协程切换到运行状态
    * @pre getState() != EXEC
    * @post getState() = EXEC
    */
    void Fiber::swapIn(){
        SetThis(this);
        SYLAR_ASSERT(m_state != EXEC);
        m_state = EXEC;
        // 函数保存当前的上下文到oucp所指向的数据结构，并且设置到ucp所指向的上下文
        // 执行绑定的函数 MainFunc
        if (swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)) {
            SYLAR_ASSERT2(false, "swapcontext");
        }
        // SYLAR_LOG_INFO(g_logger) << t_threadFiber->getId();
    }

    // 切换到后台执行
    void Fiber::swapOut(){
        SetThis(Scheduler::GetMainFiber());
        //SetThis(t_threadFiber.get());
        if (swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
            SYLAR_ASSERT2(false, "swapcontext");
        }
    }

    void Fiber::call() {
        SetThis(this);
        m_state = EXEC;
        if (swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
            SYLAR_ASSERT2(false, "swapcontext");
        }
    }

    void Fiber::back()
    {
        SetThis(t_threadFiber.get());
        if (swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
            SYLAR_ASSERT2(false, "swapcontext");
        }
    }

    uint64_t Fiber::GetFiberId() {
        if(t_fiber) {
            return t_fiber->getId();
        }
        return 0;
    }

    // 设置当前线程的运行协程
    void Fiber::SetThis(Fiber* f)
    {
        t_fiber = f;
    }

    // 返回当前协程
    Fiber::ptr Fiber::GetThis(){
        if (t_fiber)
        {
            return t_fiber->shared_from_this();
        }
        Fiber::ptr main_fiber(new Fiber);
        SYLAR_ASSERT(t_fiber == main_fiber.get());
        t_threadFiber = main_fiber;
        return t_fiber->shared_from_this();
    }

    // 协程切换到后台,并且设置为Ready状态
    void Fiber::YieldToReady(){
        Fiber::ptr cur = GetThis();
        SYLAR_ASSERT(cur->m_state == EXEC);
        cur->m_state = READY;
        cur->swapOut();
    }

    // 协称切换到后台，并且设置为Hold状态
    void Fiber::YieldToHold(){
        Fiber::ptr cur = GetThis();
        SYLAR_ASSERT(cur->m_state == EXEC);
        cur->m_state = HOLD;
        cur->swapOut();
    }

    // 总协程数
    uint64_t Fiber::TotalFibers(){
        return s_fiber_count;
    }

    // 协程执行函数 执行完返回到线程主协程
    void Fiber::MainFunc(){
        Fiber::ptr cur = GetThis();
        SYLAR_ASSERT(cur);
        try{
            cur->m_cb();
            cur->m_cb = nullptr;
            cur->m_state = TERM;
        }
        catch (std::exception& ex)
        {
            cur->m_state = EXCEPT;
            SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
                << " fiber_id = " << cur->getId()
                << std::endl
                << sylar::BacktraceToString();
        } catch (...) {
            cur->m_state = EXCEPT;
            SYLAR_LOG_ERROR(g_logger) << "Fiber Except"
                                      << " fiber_id=" << cur->getId()
                                      << std::endl
                                      << sylar::BacktraceToString();
        }

        // 去掉会存在内存泄露问题
        auto raw_ptr = cur.get();   // 子协程裸指针
        cur.reset();
        raw_ptr->swapOut(); // 切换到主协程执行

        SYLAR_ASSERT2(false, "never reach fiber_id = " + std::to_string(raw_ptr->getId()));
    }

    void Fiber::CallerMainFunc()
    {
        Fiber::ptr cur = GetThis();
        SYLAR_ASSERT(cur);
        try {
            cur->m_cb();
            cur->m_cb = nullptr;
            cur->m_state = TERM;
        } catch (std::exception& ex) {
            cur->m_state = EXCEPT;
            SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
                                      << " fiber_id=" << cur->getId()
                                      << std::endl
                                      << sylar::BacktraceToString();
        } catch (...) {
            cur->m_state = EXCEPT;
            SYLAR_LOG_ERROR(g_logger) << "Fiber Except"
                                      << " fiber_id=" << cur->getId()
                                      << std::endl
                                      << sylar::BacktraceToString();
        }

        auto raw_ptr = cur.get();
        cur.reset();
        raw_ptr->back();
        SYLAR_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
    }


}