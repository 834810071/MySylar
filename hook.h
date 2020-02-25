//
// Created by jxq on 20-2-25.
//

#ifndef MYSYLAR_HOOK_H
#define MYSYLAR_HOOK_H

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

namespace sylar {
    /**
     * @brief 当前线程是否hook
     */
    bool is_hook_enable();
    /**
     * @brief 设置当前线程的hook状态
     */
    void set_hook_enable(bool flag);
}

extern "C" {

//sleep
typedef unsigned int (*sleep_fun)(unsigned int seconds);
extern sleep_fun sleep_f;

typedef int (*usleep_fun)(useconds_t usec);
extern usleep_fun usleep_f;

};

#endif //MYSYLAR_HOOK_H
