//
// Created by jxq on 20-2-21.
//

#include "sylar.h"
#include <assert.h>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_assert() {
    SYLAR_LOG_INFO(g_logger) << sylar::BacktraceToString(10);
    //SYLAR_ASSERT(0 == 0);
    SYLAR_ASSERT2(0 == 1, "abcdef xx");
}

int main() {
    test_assert();

    return 0;
}