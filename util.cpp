//
// Created by jxq on 20-2-15.
//

#include "util.h"
#include <syscall.h>
#include <zconf.h>
#include <cstdint>

namespace sylar {

    pid_t GetThreadId()
    {
        return syscall(SYS_gettid); // 获取线程id唯一标识
    }

    uint32_t GetFiberId()
    {
        return 0;
    }
};