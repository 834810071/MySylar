//
// Created by jxq on 20-2-14.
//

#ifndef MYSYLAR_LOG_H
#define MYSYLAR_LOG_H

#include <string>
#include <stdint.h>
#include <memory>
#include <list>
#include <fstream>
#include <vector>
#include <sstream>
#include <map>
#include "util.h"
#include "singleton.h"

// 使用流式方式将日志级别level的日志写入到logger
// 如果当前logger的日志级别小于参数level，那么重新创建一个LogEvent，并用LogEventWrap包装
#define SYLAR_LOG_LEVEL(logger, level)\
    if (logger->getLevel() <= level) \
        sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger, level, \
                __FILE__, __LINE__, 0, sylar::GetThreadId(),\
                sylar::GetFiberId(), time(0)))).getSS()

/**
 * @brief 使用流式方式将日志级别debug的日志写入到logger
 */
#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::DEBUG)

/**
 * @brief 使用流式方式将日志级别info的日志写入到logger
 */
#define SYLAR_LOG_INFO(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::INFO)

/**
 * @brief 使用流式方式将日志级别warn的日志写入到logger
 */
#define SYLAR_LOG_WARN(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::WARN)

/**
 * @brief 使用流式方式将日志级别error的日志写入到logger
 */
#define SYLAR_LOG_ERROR(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::ERROR)

/**
 * @brief 使用流式方式将日志级别fatal的日志写入到logger
 */
#define SYLAR_LOG_FATAL(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::FATAL)

/**
 * @brief 使用格式化方式将日志级别level的日志写入到logger
 */
#define SYLAR_LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if(logger->getLevel() <= level) \
        sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger, level, \
                        __FILE__, __LINE__, 0, sylar::GetThreadId(),\
                sylar::GetFiberId(), time(0), sylar::Thread::GetName()))).getEvent()->format(fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别debug的日志写入到logger
 */
#define SYLAR_LOG_FMT_DEBUG(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::DEBUG, fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别info的日志写入到logger
 */
#define SYLAR_LOG_FMT_INFO(logger, fmt, ...)  SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::INFO, fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别warn的日志写入到logger
 */
#define SYLAR_LOG_FMT_WARN(logger, fmt, ...)  SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::WARN, fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别error的日志写入到logger
 */
#define SYLAR_LOG_FMT_ERROR(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::ERROR, fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别fatal的日志写入到logger
 */
#define SYLAR_LOG_FMT_FATAL(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::FATAL, fmt, __VA_ARGS__)

// 获取主日志器
#define SYLAR_LOG_ROOT() sylar::LoggerMgr::GetInstance()->getRoot()

// 获取name的日志器
#define SYLAR_LOG_NAME(name) sylar::LoggerMgr::GetInstance()->getLogger(name)

namespace sylar {
    class Logger;

    // 日志级别
    class LogLevel {
    public:
        /**
         * 日志级别枚举
         */
        enum Level {
            UNKNOW = 0,
            DEBUG = 1,
            INFO = 2,
            WARN = 3,
            ERROR = 4,
            FATAL = 5,
        };

        // 将日志级别转换成文本输出
        static const char* ToString(LogLevel::Level level);

        // 将文本转化成日志级别
        static LogLevel::Level FromString(const std::string& str);

    };

    // 日志事件的封装
    class LogEvent {
    public:
        typedef std::shared_ptr<LogEvent> ptr;
        /**
         * @brief 构造函数
         * @param[in] logger 日志器
         * @param[in] level 日志级别
         * @param[in] file 文件名
         * @param[in] line 文件行号
         * @param[in] elapse 程序启动依赖的耗时(毫秒)
         * @param[in] thread_id 线程id
         * @param[in] fiber_id 协程id
         * @param[in] time 日志事件(秒)
         * @param[in] thread_name 线程名称
         */
        LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level
                ,const char* file, int32_t line, uint32_t elapse
                ,uint32_t thread_id, uint32_t fiber_id, uint64_t time
                ,const std::string& thread_name);

        LogEvent(const char* file, int32_t line, uint32_t elapse
                ,uint32_t thread_id, uint32_t fiber_id, uint64_t time);

        LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level
                ,const char* file, int32_t line, uint32_t elapse
                ,uint32_t thread_id, uint32_t fiber_id, uint64_t time);

        // ~LogEvent();

        const char* getFile() const { return m_file; }
        int32_t getLine() const { return m_line; }
        uint32_t getElapse() const { return m_elapse; }
        uint32_t getThreadId() const { return m_threadId; }
        uint32_t getFiberId() const { return m_fiberId; }
        uint64_t getTime() const { return m_time; }
        const std::string getContent() const { return m_ss.str(); }
        std::stringstream& getSS() { return m_ss;}  // 返回日志内容字符串流
        const std::string getThreadName() { return m_threadName; }
        LogLevel::Level  getLevel() const { return m_level; }
        std::shared_ptr<Logger> getLogger() { return m_logger; }

        // 格式化写入日志内容
        void format(const char* fmt, ...);

        // 格式化写入日志内容
        void format(const char* fmt, va_list al);
    private:
        const char* m_file = nullptr;   // 文件名
        int32_t m_line = 0;             // 行号
        uint32_t m_elapse = 0;          // 程序启动开始到现在的毫秒数
        uint32_t m_threadId = 0;        // 线程id
        uint32_t m_fiberId = 0;         // 协成id;
        uint64_t m_time = 0;            // 时间戳
        std::string m_threadName;       // 线程名称
        //std::string m_content;
        std::stringstream m_ss;         // 日志内容留
        std::shared_ptr<Logger> m_logger;  // 日志器
        LogLevel::Level m_level;        // 日志等级
    };

    // 日志事件包装器
    class LogEventWrap {
    public:
        LogEventWrap(LogEvent::ptr e);
        // 析构的时候进行日志输出
        ~LogEventWrap();

        LogEvent::ptr getEvent() const { return m_event; }
        std::stringstream& getSS();

    private:
        LogEvent::ptr m_event;  // 日志事件
    };

    // 日志格式器
    class LogFormatter {
    public:
        typedef std::shared_ptr<LogFormatter> ptr;
        LogFormatter(const std::string& pattern);

        // 将LogEvent格式化为字符串
        std::string format(std::shared_ptr<Logger> loger, LogLevel::Level level, LogEvent::ptr event);

    public:
        // 具体日志内容项格式化
        class FormatItem {
        public:
            typedef std::shared_ptr<FormatItem> ptr;
            FormatItem(const std::string& fmt = "") {};
            virtual ~FormatItem() {}
            // 格式化日志到流
            virtual void format(std::ostream& os, std::shared_ptr<Logger> loger, LogLevel::Level level, LogEvent::ptr event) = 0;
        };

        void init();    // 初始化, 进行日志格式解析

        /**
         * @brief 是否有错误
         */
        bool isError() const { return m_error;}

        std::string getPattern() { return m_pattern; }
    private:
        std::string m_pattern;              // 日志格式模版
        std::vector<FormatItem::ptr> m_items;    // 日志格式解析后格式
        bool m_error = false;               // 是否有错误
    };

    // 日志输出目标
    class LogAppender {
    friend class Logger;
    public:
        typedef std::shared_ptr<LogAppender> ptr;
        virtual ~LogAppender() {}

        // 将日志输出到对应的落地点
        virtual void log(std::shared_ptr<Logger> loger, LogLevel::Level level, LogEvent::ptr event) = 0;   // 纯虚函数

        void setFormatter(LogFormatter::ptr val) {
            m_formatter = val;
            if (m_formatter)
            {
                m_hasFormatter = true;
            }
            else
            {
                m_hasFormatter = false;
            }
        }
        LogFormatter::ptr getFormatter() const {
            return m_formatter;
        }

        LogLevel::Level getLevel() const { return m_level; }
        void setLevel(LogLevel::Level val) { m_level = val; }

        /**
         * @brief 将日志输出目标的配置转成YAML String
         */
        virtual std::string toYamlString() = 0;
    protected:
        LogLevel::Level m_level = LogLevel::DEBUG;
        bool m_hasFormatter = false;    // 是否有自己的日志格式器
        LogFormatter::ptr m_formatter;  // 日志格式器
    };

    // 日志器
class Logger : public std::enable_shared_from_this<Logger>{
    friend class LoggerManager;
    public:
        typedef std::shared_ptr<Logger> ptr;

        Logger(const std::string& name = "root");
        // 写入日志，指定日志级别
        void log(LogLevel::Level level, LogEvent::ptr event);

        void debug(LogEvent::ptr event);
        void info(LogEvent::ptr event);
        void warn(LogEvent::ptr event);
        void error(LogEvent::ptr event);
        void fatal(LogEvent::ptr event);

        void addAppender(LogAppender::ptr appender);
        void delAppender(LogAppender::ptr appender);
        void clearAppenders();
        LogLevel::Level getLevel() const
        {
            return m_level;
        }
        void setLevel(LogLevel::Level val) {
            m_level = val;
        }

        const std::string& getName() const {
            return m_name;
        }

        void setFormatter(LogFormatter::ptr val);

        void setFormatter(const std::string& val);

        LogFormatter::ptr getFormatter();

        /**
         * @brief 将日志器的配置转成YAML String
         */
        std::string toYamlString();

        /**
         * @brief 返回主日志器
         */
        Logger::ptr getRoot() const { return m_root;}

    private:
        std::string m_name;                         // 日志名称
        LogLevel::Level m_level;                    // 日志级别
        std::list<LogAppender::ptr> m_appenders;    // Appender集合
        LogFormatter::ptr m_formatter;              // 日志格式器
        Logger::ptr m_root;                         // 主日志器
    };

    // 输出到控制台的Appender
    class StdoutLogAppender : public LogAppender {
    public:
        typedef std::shared_ptr<StdoutLogAppender> ptr;
        void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;
        std::string toYamlString() override;
    };

    // 定义输出到文件Appender
    class FileLogAppender : public LogAppender {
    public:
        typedef std::shared_ptr<FileLogAppender> ptr;
        FileLogAppender(const std::string& filename);
        void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;
        std::string toYamlString() override;

        // 重新打开日志文件
        bool reopen();
    private:
        std::string m_filename;     // 文件路径
        std::ofstream m_filestream;  // 文件流
    };

    // 日志管理类
    class LoggerManager
    {
    public:
        LoggerManager();

        Logger::ptr getLogger(const std::string& name);
        // 初始化
        void init();

        /**
         * @brief 返回主日志器
         */
        Logger::ptr getRoot() const { return m_root;}

        /**
         * @brief 将所有的日志器配置转成YAML String
         */
        std::string toYamlString();
    private:
        std::map<std::string, Logger::ptr> m_loggers;   // 日志器容器
        Logger::ptr m_root; // 主日志器 构造时进行初始化
    };

    // 日志管理类单例模式
    typedef sylar::Singleton<LoggerManager> LoggerMgr;

};



#endif //MYSYLAR_LOG_H
