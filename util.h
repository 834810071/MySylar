//
// Created by jxq on 20-2-15.
//

#ifndef MYSYLAR_UTIL_H
#define MYSYLAR_UTIL_H

#include <sys/types.h>
#include <cstdint>

namespace sylar {

    /**
     * 返回当前线程的ID
     * @return
     */
    pid_t GetThreadId();

    /**
     * @brief 返回当前协程的ID
    */
    uint32_t GetFiberId();
};


#endif //MYSYLAR_UTIL_H
