/*
 * @Author: Xudong0722
 * @Date: 2025-03-04 23:38:57
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-05 23:10:03
 */

#pragma once
#include <list>
#include <map>
#include <vector>
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
        void clearAppenders();

        std::shared_ptr<LogFormatter> getFormatter() const;
        void setFormatter(std::shared_ptr<LogFormatter> fomatter);
        void setFormatter(const std::string& pattern);

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
        /// @brief Get Logger by name, if logger is not exist, we will create a new logger
        /// @param name 
        /// @return shared_ptr to logger
        Logger::sptr getLogger(const std::string &name);

        void init();
        Logger::sptr getRoot() { return m_root; }

    private:
        std::map<std::string, Logger::sptr> m_loggers;
        Logger::sptr m_root;
    };
    using LogMgr = East::Singleton<LoggerMgr>;

    enum class LogAppenderType
    {
        NONE = 0,
        FILE = 1,
        STANDARD = 2,
    };

    //from log.yml
    struct LogAppenderConfigInfo
    {
        LogAppenderType type{LogAppenderType::NONE};
        LogLevel::Level level{LogLevel::Level::NONE};
        std::string file_path;
        std::string formatter;

        bool operator==(const LogAppenderConfigInfo& rhs) const 
        {
            return type == rhs.type
                && level == rhs.level
                && file_path == rhs.file_path
                && formatter == rhs.formatter;
        }
    };

    struct LoggerConfigInfo
    {
        std::string name;
        LogLevel::Level level{LogLevel::Level::NONE};
        std::string formatter;
        std::vector<LogAppenderConfigInfo> log_appenders;

        bool operator==(const LoggerConfigInfo& rhs) const 
        {
            return name == rhs.name
                && level == rhs.level
                && formatter == rhs.formatter
                && log_appenders == rhs.log_appenders;
        }

        bool operator<(const LoggerConfigInfo& rhs) const  
        {
            return name < rhs.name;
        }
    };

    struct LogInitiator
    {
        LogInitiator();
    };
}