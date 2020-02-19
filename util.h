//
// Created by jxq on 20-2-15.
//

#ifndef MYSYLAR_UTIL_H
#define MYSYLAR_UTIL_H

#include <sys/types.h>
#include <cstdint>
#include <cxxabi.h>

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

    template<class T>
    const char* TypeToName() {
        static const char* s_name = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
    }
};


#endif //MYSYLAR_UTIL_H
