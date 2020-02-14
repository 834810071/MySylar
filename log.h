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

namespace sylar {

    // 日志事件
    class LogEvent {
    public:
        typedef std::shared_ptr<LogEvent> ptr;
        LogEvent();
    private:
        const char* m_file = nullptr;   // 文件名
        int32_t m_line = 0;             // 行号
        uint32_t m_elapse = 0;          // 程序启动开始到现在的毫秒数
        uint32_t m_threadId = 0;        // 线程id
        uint32_t m_fiberId = 0;         // 协成id;
        uint64_t m_time = 0;            // 时间戳
        std::string m_countent;
    };

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
    };

    // 日志格式器
    class LogFormatter {
    public:
        typedef std::shared_ptr<LogFormatter> ptr;
        LogFormatter(const std::string& pattern);

        std::string format(LogEvent::ptr event);

    private:
        // 日志内容项格式化
        class FormatItem {
        public:
            typedef std::shared_ptr<FormatItem> ptr;
            virtual ~FormatItem() {}
            // 格式化日志到流
            virtual void format(std::ostream& os, LogEvent::ptr event) = 0;
        };

        void init();
    private:
        std::string m_pattern;              // 日志格式模版
        std::vector<FormatItem> m_items;    // 日志格式解析后格式
    };

    // 日志输出目标
    class LogAppender {
    public:
        typedef std::shared_ptr<LogAppender> ptr;
        virtual ~LogAppender() {}

        virtual void log(LogLevel::Level level, LogEvent::ptr event) = 0;   // 纯虚函数

        void setFormatter(LogFormatter::ptr val) {
            m_formatter = val;
        }
        LogFormatter::ptr getFormatter() const {
            return m_formatter;
        }
    protected:
        LogLevel::Level m_level;
        LogFormatter::ptr m_formatter;
    };

    // 日志器
    class Logger {
    public:
        typedef std::shared_ptr<Logger> ptr;

        Logger(const std::string& name = "root");
        void log(LogLevel::Level level, LogEvent::ptr event);

        void debug(LogEvent::ptr event);
        void info(LogEvent::ptr event);
        void warn(LogEvent::ptr event);
        void error(LogEvent::ptr event);
        void fatal(LogEvent::ptr event);

        void addAppender(LogAppender::ptr appender);
        void delAppender(LogAppender::ptr appender);
        LogLevel::Level getLevel() const
        {
            return m_level;
        }
        void setLevel(LogLevel::Level val) {
            m_level = val;
        }

    private:
        std::string m_name;                         // 日志名称
        LogLevel::Level m_level;                    // 日志级别
        std::list<LogAppender::ptr> m_appenders;     // Appender集合
    };

    // 输出到控制台的Appender
    class StdoutLogAppender : public LogAppender {
    public:
        typedef std::shared_ptr<StdoutLogAppender> ptr;
        void log(LogLevel::Level level, LogEvent::ptr event) override;
    };

    // 定义输出到文件Appender
    class FileLogAppender : public LogAppender {
    public:
        typedef std::shared_ptr<FileLogAppender> ptr;
        FileLogAppender(const std::string& filename);
        void log(LogLevel::Level level, LogEvent::ptr event) override;

        // 重新打开日志文件
        bool reopen();
    private:
        std::string m_filename;     // 文件路径
        std::ofstream m_filestream;  // 文件流
    };
};


#endif //MYSYLAR_LOG_H
