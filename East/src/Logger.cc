/*
 * @Author: Xudong0722
 * @Date: 2025-03-04 23:57:19
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-04 23:58:22
 */

#include "Logger.h"
#include "LogAppender.h"
#include "LogFormatter.h"

namespace East
{
    Logger::Logger(const std::string &name)
        : m_name(name), m_level(LogLevel::DEBUG)
    {
        m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n")); // default formatter
    }

    void Logger::Log(LogLevel::Level level, LogEvent::sptr event)
    {
        if (level < m_level)
            return;
        auto self = shared_from_this();
        for (auto &appender : m_appenders)
        {
            if (nullptr != appender)
            {
                appender->Log(self, level, event);
            }
        }
    }

    void Logger::debug(LogEvent::sptr event)
    {
        Log(LogLevel::DEBUG, event);
    }

    void Logger::info(LogEvent::sptr event)
    {
        Log(LogLevel::INFO, event);
    }

    void Logger::warn(LogEvent::sptr event)
    {
        Log(LogLevel::WARN, event);
    }

    void Logger::error(LogEvent::sptr event)
    {
        Log(LogLevel::ERROR, event);
    }

    void Logger::fatal(LogEvent::sptr event)
    {
        Log(LogLevel::FATAL, event);
    }

    void Logger::addAppender(LogAppender::sptr appender)
    {
        if (nullptr == appender)
            return;
        if (nullptr == appender->getFormatter())
        {
            appender->setFormatter(m_formatter);
        }
        m_appenders.emplace_back(appender);
    }

    void Logger::delAppender(LogAppender::sptr appender)
    {
        for (auto cit = m_appenders.cbegin(); cit != m_appenders.cend(); ++cit)
        {
            if (*cit == appender)
            {
                m_appenders.erase(cit);
                break;
            }
        }
    }

    LoggerMgr::LoggerMgr()
    {
        m_root.reset(new Logger);
        m_root->addAppender(std::make_shared<StdoutLogAppender>());
    }

    void LoggerMgr::init()
    {
    }

    Logger::sptr LoggerMgr::getLogger(const std::string &name)
    {
        auto it = m_loggers.find(name);
        return it == m_loggers.end() ? m_root : it->second;
    }
}