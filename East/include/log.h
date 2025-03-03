/*
 * @Author: Xudong0722
 * @Date: 2025-02-24 20:45:27
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-04 00:46:51
 */

#pragma once
#include <stdint.h>
#include <string>
#include <memory>
#include <list>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include "singleton.h"

// Log Macro
#define ELOG_LEVEL(logger, level)                                                                                                         \
    if (nullptr != logger && logger->getLevel() <= level)                                                                                 \
    East::LogEventWrap(std::make_shared<East::LogEvent>(logger, level, __FILE__, __LINE__, 0, static_cast<uint32_t>(East::getThreadId()), \
                                                        static_cast<uint32_t>(East::getFiberId()), static_cast<uint64_t>(time(0))))       \
        .getSStream()

#define ELOG_DEBUG(logger) ELOG_LEVEL(logger, East::LogLevel::DEBUG)
#define ELOG_INFO(logger) ELOG_LEVEL(logger, East::LogLevel::INFO)
#define ELOG_WARN(logger) ELOG_LEVEL(logger, East::LogLevel::WARN)
#define ELOG_ERROR(logger) ELOG_LEVEL(logger, East::LogLevel::ERROR)
#define ELOG_FATAL(logger) ELOG_LEVEL(logger, East::LogLevel::FATAL)

namespace East
{

    class Logger;
    class LogLevel
    {
    public:
        enum Level
        {
            NONE = 0,
            DEBUG = 1,
            INFO = 2,
            WARN = 3,
            ERROR = 4,
            FATAL = 5,
        };
        static const char *toStr(Level level);
    };

    class LogEvent
    {
    public:
        using sptr = std::shared_ptr<LogEvent>;
        LogEvent(std::shared_ptr<Logger> logger,
                 LogLevel::Level level,
                 const char *file,
                 int32_t line,
                 int32_t elapse,
                 uint32_t thread_id,
                 uint32_t fiber_id,
                 uint64_t time);

        const char *getFileName() const { return m_file; }
        int32_t getLineNo() const { return m_line; }
        int32_t getElapse() const { return m_elapse; }
        uint32_t getThreadId() const { return m_threadId; }
        uint32_t getFiberId() const { return m_fiberId; }
        uint64_t getTimeStamp() const { return m_time; }
        std::string getContent() const { return m_ss.str(); }
        LogLevel::Level getLevel() const { return m_level; }
        std::shared_ptr<Logger> getLogger() { return m_logger; }
        std::stringstream &getSStream() { return m_ss; }

    private:
        const char *m_file{nullptr}; // 8 bytes on x64
        int32_t m_line{0};
        int32_t m_elapse{0};
        uint32_t m_threadId{0};
        uint32_t m_fiberId{0};
        uint64_t m_time{0};
        std::string m_content;
        LogLevel::Level m_level;
        std::stringstream m_ss;
        std::shared_ptr<Logger> m_logger;
    };

    class LogEventWrap
    {
    public:
        LogEventWrap(LogEvent::sptr event);
        ~LogEventWrap();

        std::stringstream &getSStream();

    private:
        LogEvent::sptr m_event;
    };

    class LogFormatter
    {
    public:
        using sptr = std::shared_ptr<LogFormatter>;
        LogFormatter(const std::string &pattern);
        std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::sptr event);

    public:
        class FormatItem
        {
        public:
            using sptr = std::shared_ptr<FormatItem>;
            virtual ~FormatItem() {}

            virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::sptr event) = 0;
        };

        void init();

    private:
        std::string m_pattern;
        std::vector<FormatItem::sptr> m_items;
    };

    // Log target, std or file ...
    class LogAppender
    {
    public:
        using sptr = std::shared_ptr<LogAppender>;
        virtual ~LogAppender() {}

        virtual void Log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::sptr event) = 0;

        void setFormatter(LogFormatter::sptr formatter) { m_formatter = formatter; }
        LogFormatter::sptr getFormatter() const { return m_formatter; }

        void setLevel(LogLevel::Level level) { m_level = level; }
        LogLevel::Level getLevel() const { return m_level; }

    protected:
        LogLevel::Level m_level{LogLevel::DEBUG};
        LogFormatter::sptr m_formatter;
    };

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

        void addAppender(LogAppender::sptr appender);
        void delAppender(LogAppender::sptr appender);

    private:
        std::string m_name;      // log name
        LogLevel::Level m_level; // log level(output when event level >= m_level)
        std::list<LogAppender::sptr> m_appenders;
        LogFormatter::sptr m_formatter;
    };

    class StdoutLogAppender : public LogAppender
    {
    public:
        using sptr = std::shared_ptr<StdoutLogAppender>;
        void Log(Logger::sptr logger, LogLevel::Level level, LogEvent::sptr event) override;
    };

    class FileLogAppender : public LogAppender
    {
    public:
        using sptr = std::shared_ptr<FileLogAppender>;
        FileLogAppender(const std::string &filename);
        ~FileLogAppender();
        void Log(Logger::sptr logger, LogLevel::Level level, LogEvent::sptr event) override;

        bool reopen(); // TODO, Why add this api?
    private:
        std::string m_filename;
        std::ofstream m_filestream;
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
} // namespace East
