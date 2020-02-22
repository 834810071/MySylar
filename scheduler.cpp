//
// Created by jxq on 20-2-22.
//

#include "scheduler.h"
#include "fiber.h"


namespace sylar {

    Scheduler::Scheduler(size_t threads, bool use_caller, std::string& name){

    }

    Scheduler::~Scheduler(){

    }

    /**
     * @brief 返回当前协程调度器
     */
     Scheduler* Scheduler::GetThis(){

    }

    /**
     * @brief 返回当前协程调度器的调度协程
     */
    Fiber* Scheduler::GetMainFiber(){

    }

    // 启动协程调度器
    void Scheduler::start(){

    }

    // 停止协程调度器
    void Scheduler::stop(){

    }
}