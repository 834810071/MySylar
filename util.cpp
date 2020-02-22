//
// Created by jxq on 20-2-15.
//

#include "util.h"
#include "log.h"
#include "fiber.h"
#include <syscall.h>
#include <zconf.h>
#include <cstdint>
#include <vector>
#include <string>
#include <execinfo.h>

namespace sylar {

    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    pid_t GetThreadId()
    {
        return syscall(SYS_gettid); // 获取线程id唯一标识
    }

    uint32_t GetFiberId()
    {
        return sylar::Fiber::GetFiberId();
    }

    // 反名字改编
    static std::string demangle(const char* str)
    {
        size_t size = 0;
        int status = 0;
        std::string rt;
        rt.resize(256);

        // first, try to demangle a c++ name
        if (1 == sscanf(str, "%*[^(]%*[^_]%255[^)+]", &rt[0])) {    // 从字符串读取格式化输入,如果成功，该函数返回成功匹配和赋值的个数
            char* v = abi::__cxa_demangle(&rt[0], nullptr, &size, &status); // 将函数名还原出来
            if (v)
            {
                std::string result(v);
                free(v);
                return result;
            }
        }
        // if that didn't work, try to get a regular c symbol
        if(1 == sscanf(str, "%255s", &rt[0])) {
            return rt;
        }
        //if all else fails, just return the symbol
        return str;
    }

    // 填充栈痕迹
    void Backtrace(std::vector<std::string>& bt, int size, int skip)
    {
        void** array = (void**)malloc((sizeof(void*) * size));
        size_t s = ::backtrace(array, size);    // 返回调用堆栈 //列出当前函数调用关系

        // 将从backtrace函数获取的信息转化为一个字符串数组
        char** strings = backtrace_symbols(array, s);   // 字符串结果通过该API返回，会在该函数中malloc，由我们free
        if (strings == NULL)
        {
            SYLAR_LOG_ERROR(g_logger) << "backtrace_synbols error";
            return;
        }

        for (size_t i = skip; i < s; ++i)
        {
            // TODO demangle funcion name with abi::__cxa_demangle
            // strings[i]代表某一层的调用痕迹
            bt.push_back(demangle(strings[i]));
        }
        free(strings);
        free(array);
    }

    std::string BacktraceToString(int size, int skip, const std::string& prefix)
    {
        std::vector<std::string> bt;
        Backtrace(bt, size, skip);
        std::stringstream ss;
        for (size_t i = 0; i < bt.size(); ++i)
        {
            ss << prefix << bt[i] << std::endl;
        }
        return ss.str();
    }
};