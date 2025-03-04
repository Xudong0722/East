/*
 * @Author: Xudong0722
 * @Date: 2025-03-04 23:38:57
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-04 23:41:52
 */

#pragma once
#include <list>
#include <map>
#include "LogEvent.h"
#include "singleton.h"
namespace East
{
    class LogAppender;
    class LogFormatter;
    class Logger : public std::enable_shared_from_this<Logger>
    {
    public:
        using sptr = std::shared_ptr<Logger>;
        Logger(const std::string &name = "root");
        void Log(LogLevel::Level level, LogEvent::sptr event);

        void debug(LogEvent::sptr event);
        void info(LogEvent::sptr event);
        void warn(LogEvent::sptr event);
        void error(LogEvent::sptr event);
        void fatal(LogEvent::sptr event);

        LogLevel::Level getLevel() const { return m_level; }
        void setLevel(LogLevel::Level level) { m_level = level; }
        const std::string &getName() const { return m_name; }

        void addAppender(std::shared_ptr<LogAppender> appender);
        void delAppender(std::shared_ptr<LogAppender> appender);

    private:
        std::string m_name;      // log name
        LogLevel::Level m_level; // log level(output when event level >= m_level)
        std::list<std::shared_ptr<LogAppender>> m_appenders;
        std::shared_ptr<LogFormatter> m_formatter;
    };

    class LoggerMgr
    {
    public:
        LoggerMgr();
        Logger::sptr getLogger(const std::string &name);

        void init();

    private:
        std::map<std::string, Logger::sptr> m_loggers;
        Logger::sptr m_root;
    };
    using LogMgr = East::Singleton<LoggerMgr>;
}