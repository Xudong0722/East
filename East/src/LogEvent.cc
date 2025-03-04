/*
 * @Author: Xudong0722
 * @Date: 2025-03-04 23:43:12
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-04 23:54:59
 */
#include <cassert>
#include "LogEvent.h"
#include "Logger.h"

namespace East
{
    const char *LogLevel::toStr(Level level)
    {
        static const char *str_levels[] = {"NONE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
        assert(level >= DEBUG && level <= FATAL);
        return str_levels[level];
    }

    LogEvent::LogEvent(std::shared_ptr<Logger> logger,
                       LogLevel::Level level,
                       const char *file,
                       int32_t line,
                       int32_t elapse,
                       uint32_t thread_id,
                       uint32_t fiber_id,
                       uint64_t time)
        : m_file(file), m_line(line), m_elapse(elapse), m_threadId(thread_id), m_fiberId(fiber_id), m_time(time), m_level(level), m_logger(logger)
    {
    }

    LogEventWrap::LogEventWrap(LogEvent::sptr event)
        : m_event(event)
    {
    }

    LogEventWrap::~LogEventWrap()
    {
        if (nullptr != m_event)
        {
            m_event->getLogger()->Log(m_event->getLevel(), m_event);
        }
    }

    std::stringstream &LogEventWrap::getSStream()
    {
        return m_event->getSStream();
    }
}