//
// Created by jxq on 20-2-20.
//

#include <iostream>
#include "sylar.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

sylar::Mutex s_mutex;
//sylar::RWMutex s_mutex;

static int count = 0;

void func1()
{
    SYLAR_LOG_INFO(g_logger) << "name: " << sylar::Thread::GetName()
                             << " this.name: " << sylar::Thread::GetThis()->getName()
                             << " id: " << sylar::GetThreadId()
                             << " this.id " << sylar::Thread::GetThis()->getId();

    for(int i = 0; i < 100000; ++i) {
        //sylar::RWMutex::ReadLock lock(s_mutex);
        sylar::Mutex::Lock lock(s_mutex);
        ++count;
    }
}

void func2()
{
    while (true)
    {
        SYLAR_LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    }
}

void func3()
{
    while (true)
    {
        SYLAR_LOG_INFO(g_logger) << "++++++++++++++++++++++++++++++++++++++++";
    }
}

int main()
{
    SYLAR_LOG_INFO(g_logger) << "thread test begin";
    YAML::Node root = YAML::LoadFile("/home/jxq/CLionProjects/MySylar/bin/conf/log2.yml");
    sylar::Config::LoadFromYaml(root);
    std::vector<sylar::Thread::ptr> thrs;
    for (int i = 0; i < 5; ++i)
    {
        sylar::Thread::ptr thr(new sylar::Thread(&func3, "name_" + std::to_string(i * 2)));
        sylar::Thread::ptr thr1(new sylar::Thread(&func2, "name_" + std::to_string(i * 2 + 1)));
        thrs.push_back(thr);
        thrs.push_back(thr1);
    }

    for (size_t i = 0; i < thrs.size(); ++i)
    {
        thrs[i]->join();
    }

    SYLAR_LOG_INFO(g_logger) << "thread test end";
    SYLAR_LOG_INFO(g_logger) << "count=" << count;
    return 0;
}