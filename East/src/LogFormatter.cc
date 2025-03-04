/*
 * @Author: Xudong0722
 * @Date: 2025-03-04 23:54:14
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-04 23:56:45
 */

#include <functional>

#include "LogFormatter.h"
#include "Logger.h"

namespace East
{
    class MessageFormatItem : public LogFormatter::FormatItem
    {
    public:
        MessageFormatItem(const std::string &__ = "") {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::sptr event) override
        {
            os << event->getContent();
        }
    };

    class LevelFormatItem : public LogFormatter::FormatItem
    {
    public:
        LevelFormatItem(const std::string &__ = "") {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::sptr event) override
        {
            os << LogLevel::toStr(level);
        }
    };

    class ElapseFormatItem : public LogFormatter::FormatItem
    {
    public:
        ElapseFormatItem(const std::string &__ = "") {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::sptr event) override
        {
            os << event->getElapse();
        }
    };

    class LoggerNameFormatItem : public LogFormatter::FormatItem
    {
    public:
        LoggerNameFormatItem(const std::string &__ = "") {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::sptr event) override
        {
            os << logger->getName();
        }
    };

    class ThreadIdFormatItem : public LogFormatter::FormatItem
    {
    public:
        ThreadIdFormatItem(const std::string &__ = "") {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::sptr event) override
        {
            os << event->getThreadId();
        }
    };

    class FiberIdFormatItem : public LogFormatter::FormatItem
    {
    public:
        FiberIdFormatItem(const std::string &__ = "") {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::sptr event) override
        {
            os << event->getFiberId();
        }
    };

    class DateTimeFormatItem : public LogFormatter::FormatItem
    {
    public:
        DateTimeFormatItem(const std::string &format = "%Y-%m-%d %H:%M:%S")
            : m_format(format)
        {
            if (format.empty())
            {
                m_format = "%Y-%m-%d %H:%M:%S";
            }
        }
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::sptr event) override
        {
            struct tm tm;
            time_t time = event->getTimeStamp();
            localtime_r(&time, &tm);
            char buf[64]{};
            strftime(buf, sizeof(buf), m_format.c_str(), &tm);
            os << buf;
        }

    private:
        std::string m_format;
    };

    class FilenameFormatItem : public LogFormatter::FormatItem
    {
    public:
        FilenameFormatItem(const std::string &__ = "") {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::sptr event) override
        {
            os << event->getFileName();
        }
    };

    class NewLineFormatItem : public LogFormatter::FormatItem
    {
    public:
        NewLineFormatItem(const std::string &__ = "") {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::sptr event) override
        {
            os << std::endl;
        }
    };

    class LineFormatItem : public LogFormatter::FormatItem
    {
    public:
        LineFormatItem(const std::string &__ = "") {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::sptr event) override
        {
            os << event->getLineNo();
        }
    };

    class NormalStringFormatItem : public LogFormatter::FormatItem
    {
    public:
        NormalStringFormatItem(const std::string &__ = "")
            : m_normal_str(__) {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::sptr event) override
        {
            os << m_normal_str;
        }

    private:
        std::string m_normal_str;
    };

    class TabFormatItem : public LogFormatter::FormatItem
    {
    public:
        TabFormatItem(const std::string &__ = "") {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::sptr event) override
        {
            os << '\t';
        }
    };

    LogFormatter::LogFormatter(const std::string &pattern)
        : m_pattern(pattern)
    {
        init();
    }

    std::string LogFormatter::format(Logger::sptr logger, LogLevel::Level level, LogEvent::sptr event)
    {
        if (nullptr == event || nullptr == logger)
            return {};
        std::stringstream ss;
        for (const auto &it : m_items)
        {
            if (nullptr == it)
                continue;
            it->format(ss, logger, level, event);
        }
        return ss.str();
    }

    //%% , %xxx, %xxx{xxx}
    void LogFormatter::init()
    {
        std::vector<std::tuple<std::string, std::string, int>> vec_pattern{}; //{pattern, format, type}
        std::string no_style_str;
        auto j = 0u;
        for (auto i = 0u; i < m_pattern.size(); ++i)
        {
            if (m_pattern[i] != '%')
            {
                no_style_str.append(1, m_pattern[i]);
                continue;
            }

            //%% case
            if ((i + 1) < m_pattern.size() && m_pattern[i + 1] == '%')
            {
                no_style_str.append(1, '%');
                continue;
            }

            //%xxx or %xxx{xxx} case
            j = i + 1;
            int fmt_status{0}; // 0 means no '{', 1 means find '{'
            auto fmt_begin{0u};
            std::string fmt, pattern;
            while (j < m_pattern.size())
            {
                if (!fmt_status && !isalpha(m_pattern[j]) && m_pattern[j] != '{' && m_pattern[j] != '}')
                {
                    pattern = m_pattern.substr(i + 1, j - i - 1);
                    break;
                }

                if (fmt_status == 0)
                {
                    if (m_pattern[j] == '{')
                    {
                        fmt_status = 1;
                        pattern = m_pattern.substr(i + 1, j - i - 1);
                        fmt_begin = j;
                        ++j;
                        continue;
                    }
                }
                if (fmt_status == 1)
                {
                    if (m_pattern[j] == '}')
                    {
                        fmt_status = 0;
                        fmt = m_pattern.substr(fmt_begin + 1, j - fmt_begin - 1);
                        ++j;
                        break;
                    }
                }
                ++j;
                if (j == m_pattern.size())
                {
                    if (pattern.empty())
                        pattern = m_pattern.substr(i + 1);
                }
            }

            if (fmt_status == 0)
            {
                if (!no_style_str.empty())
                {
                    vec_pattern.emplace_back(std::make_tuple(no_style_str, "", 0));
                    no_style_str.clear();
                }
                vec_pattern.emplace_back(std::make_tuple(pattern, fmt, 1));
                i = j - 1;
            }
            else if (fmt_status == 1)
            {
                // std::cout << "Parse error " << m_pattern.substr(i) << '\n';
                vec_pattern.emplace_back(std::make_tuple("<error parse>", fmt, 0));
                break;
            }
        }
        if (!no_style_str.empty())
        {
            vec_pattern.emplace_back(std::make_tuple(no_style_str, "", 0));
        }

        /*
        %m -- message body
        %p -- log level
        %r -- time since server startup
        %c -- log name
        %t -- thread id
        %F -- fiber id
        %n -- new line
        %d -- time
        %f -- file name
        %l -- line numble
        %T -- tab
        */

        static std::map<std::string,
                        std::function<FormatItem::sptr(const std::string &fmt)>>
            s_format_items{
                {"m", [](const std::string &fmt)
                 { return FormatItem::sptr(new MessageFormatItem(fmt)); }},
                {"p", [](const std::string &fmt)
                 { return FormatItem::sptr(new LevelFormatItem(fmt)); }},
                {"r", [](const std::string &fmt)
                 { return FormatItem::sptr(new ElapseFormatItem(fmt)); }},
                {"c", [](const std::string &fmt)
                 { return FormatItem::sptr(new LoggerNameFormatItem(fmt)); }},
                {"t", [](const std::string &fmt)
                 { return FormatItem::sptr(new ThreadIdFormatItem(fmt)); }},
                {"F", [](const std::string &fmt)
                 { return FormatItem::sptr(new FiberIdFormatItem(fmt)); }},
                {"n", [](const std::string &fmt)
                 { return FormatItem::sptr(new NewLineFormatItem(fmt)); }},
                {"d", [](const std::string &fmt)
                 { return FormatItem::sptr(new DateTimeFormatItem(fmt)); }},
                {"f", [](const std::string &fmt)
                 { return FormatItem::sptr(new FilenameFormatItem(fmt)); }},
                {"l", [](const std::string &fmt)
                 { return FormatItem::sptr(new LineFormatItem(fmt)); }},
                {"T", [](const std::string &fmt)
                 { return FormatItem::sptr(new TabFormatItem(fmt)); }},
            };

        for (const auto &item : vec_pattern)
        {
            if (std::get<2>(item) == 0)
            {
                m_items.emplace_back(std::make_shared<NormalStringFormatItem>(std::get<0>(item)));
            }
            else
            {
                // find in s_format_items, if we can't find right format item, we will push back a error msg
                const auto cit = s_format_items.find(std::get<0>(item));
                if (cit == s_format_items.end())
                {
                    m_items.emplace_back(std::make_shared<NormalStringFormatItem>("<<error_format %" + std::get<0>(item) + ">>"));
                }
                else
                {
                    m_items.emplace_back(cit->second(std::get<1>(item)));
                }
            }

            // std::cout << std::get<0>(item) << "--" << std::get<1>(item) << "--" << std::get<2>(item) << std::endl;
        }
    }

}