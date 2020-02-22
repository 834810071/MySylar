//
// Created by jxq on 20-2-21.
//
// macro 宏
#ifndef MYSYLAR_MACRO_H
#define MYSYLAR_MACRO_H

#include <string>
#include <assert.h>
#include "util.h"
#include "log.h"

#if defined __GNUC__ || defined __llvm__
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率成立
#   define SYLAR_LIKELY(x)       __builtin_expect(!!(x), 1)
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率不成立
#   define SYLAR_UNLIKELY(x)     __builtin_expect(!!(x), 0)
#else
#   define SYLAR_LIKELY(x)      (x)
#   define SYLAR_UNLIKELY(x)      (x)
#endif

// 断言宏封装
#define SYLAR_ASSERT(x) \
    if (SYLAR_UNLIKELY(!(x))) { \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x \
            << "\nbacktrace:\n" \
            << sylar::BacktraceToString(100, 2, "    ");  \
    assert(x);\
    }

// 断言宏封装
#define SYLAR_ASSERT2(x, w) \
    if (SYLAR_UNLIKELY(!(x))) { \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x \
            << "\n" << w \
            << "\nbacktrace:\n" \
            << sylar::BacktraceToString(100, 2, "    ");  \
    assert(x);\
    }


#endif //MYSYLAR_MACRO_H
