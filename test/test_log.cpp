//
// Created by jxq on 20-2-15.
//

#include <iostream>
#include "../log.h"

int main() {
    sylar::Logger::ptr logger(new sylar::Logger);   // 创建logger, 初始化格式器,日志级别默认是DEBUG模式
    logger->addAppender(sylar::LogAppender::ptr(new sylar::StdoutLogAppender)); // 添加日志输出地,日志级别默认是DEBUG模式

    sylar::FileLogAppender::ptr file_appender(new sylar::FileLogAppender("./log.txt")); // 添加日志输出地,日志级别默认是DEBUG模式,并打开文件输出流
    sylar::LogFormatter::ptr fmt(new sylar::LogFormatter("%d%T%p%T%m%n"));  // 创建日志格式器, 构造函数中进行解析
    file_appender->setFormatter(fmt);   // 日出输出设置格式
    file_appender->setLevel(sylar::LogLevel::ERROR);    // 设置日志级别为ERROR

    logger->addAppender(file_appender); // 添加日志格式器

    sylar::LogEvent::ptr event(new sylar::LogEvent(__FILE__, __LINE__, 0, 1, 2, time(0)));  // 日志事件封装
    event->getSS() << "hello sylar log";    // 日志内容输入到流里面

    logger->log(sylar::LogLevel::DEBUG, event); // 输出日志

    SYLAR_LOG_INFO(logger) << "test macro"; // LogEventWrap析构的时候进行输出
    SYLAR_LOG_ERROR(logger) << "test macro error";
    std::cout << "hello sylar log" << std::endl;

    auto l = sylar::LoggerMgr::GetInstance()->getLogger("xx");
    SYLAR_LOG_INFO(l) << "xxx";


    return 0;
}