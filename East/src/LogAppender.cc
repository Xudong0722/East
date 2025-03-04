/*
 * @Author: Xudong0722
 * @Date: 2025-03-04 23:58:44
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-04 23:59:43
 */
#include <iostream>
#include "LogAppender.h"
#include "Logger.h"
#include "LogFormatter.h"

namespace East
{
    void StdoutLogAppender::Log(Logger::sptr logger, LogLevel::Level level, LogEvent::sptr event)
    {
        if (level < m_level || nullptr == m_formatter || nullptr == event)
            return;
        std::cout << m_formatter->format(logger, level, event);
    }

    FileLogAppender::FileLogAppender(const std::string &filename)
        : m_filename(filename)
    {
        reopen();
    }

    FileLogAppender::~FileLogAppender()
    {
        if (m_filestream)
        {
            m_filestream.close();
        }
    }

    void FileLogAppender::Log(Logger::sptr logger, LogLevel::Level level, LogEvent::sptr event)
    {
        if (level < m_level || nullptr == m_formatter || nullptr == event)
            return;
        m_filestream << m_formatter->format(logger, level, event);
    }

    bool FileLogAppender::reopen()
    {
        if (m_filestream)
        {
            m_filestream.close();
        }
        m_filestream.open(m_filename);
        return !!m_filestream;
    }
}