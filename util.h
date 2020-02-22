//
// Created by jxq on 20-2-15.
//

#ifndef MYSYLAR_UTIL_H
#define MYSYLAR_UTIL_H

#include <sys/types.h>
#include <cstdint>
#include <cxxabi.h>
#include <vector>
#include <string>

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
        return s_name;
    }

    // https://blog.csdn.net/widon1104/article/details/51476247
    /**
     * @brief 获取当前的调用栈
     * @param[out] bt 保存调用栈
     * @param[in] size 最多返回层数
     * @param[in] skip 跳过栈顶的层数
     */
    void Backtrace(std::vector<std::string>& bt, int size = 64, int skip = 1);

    /**
     * @brief 获取当前栈信息的字符串
     * @param[in] size 栈的最大层数
     * @param[in] skip 跳过栈顶的层数
     * @param[in] prefix 栈信息前输出的内容
     */
    std::string BacktraceToString(int size = 64, int skip = 2, const std::string& prefix = "");
};


#endif //MYSYLAR_UTIL_H
