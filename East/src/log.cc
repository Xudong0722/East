/*
 * @Author: Xudong0722 
 * @Date: 2025-02-25 21:52:46 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-02-26 00:13:31
 */
#include "../include/log.h"
#include <iostream>
#include <cassert>
#include <tuple>
#include <map>
#include <functional>

namespace East{

LogEvent::LogEvent(const char* file, 
    int32_t line,
    int32_t elapse,
    uint32_t thread_id,
    uint32_t fiber_id,
    uint64_t time)
    : m_file(file)
    , m_line(line)
    , m_elapse(elapse)
    , m_threadId(thread_id)
    , m_fiberId(fiber_id)
    , m_time(time) {
}

const char* LogLevel::toStr(Level level)
{
    static const char* str_levels[] = {"NONE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
    assert(level >= DEBUG && level <= FATAL);
    return str_levels[level];
}

class MessageFormatItem : public LogFormatter::FormatItem{
public:
    MessageFormatItem(const std::string& __ = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::sptr event) override{
        os << event->getContent();
    }
};
    
class LevelFormatItem : public LogFormatter::FormatItem{
public:
    LevelFormatItem(const std::string& __ = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::sptr event) override{
        os << LogLevel::toStr(level);
    }
};

class ElapseFormatItem : public LogFormatter::FormatItem{
public:
    ElapseFormatItem(const std::string& __ = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::sptr event) override{
        os << event->getElapse();
    }
};

class LoggerNameFormatItem : public LogFormatter::FormatItem{
public:
    LoggerNameFormatItem(const std::string& __ = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::sptr event) override{
        os << logger->getName();
    }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem{
public:
    ThreadIdFormatItem(const std::string& __ = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::sptr event) override{
        os << event->getThreadId();  
    }
};

class FiberIdFormatItem : public LogFormatter::FormatItem{
public:
    FiberIdFormatItem(const std::string& __ = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::sptr event) override{
        os << event->getFiberId();
    }
};


class DateTimeFormatItem : public LogFormatter::FormatItem{
public:
    DateTimeFormatItem(const std::string& format = "%Y:%m:%d %H%M%S")
        : m_format(format){

    }
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::sptr event) override{
        os << event->getTimeStamp();  
    }
private:
    std::string m_format;
};

class FilenameFormatItem : public LogFormatter::FormatItem{
public:
    FilenameFormatItem(const std::string& __ = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::sptr event) override{
        os << event->getFileName();  
    }
};

class NewLineFormatItem : public LogFormatter::FormatItem{
public:
    NewLineFormatItem(const std::string& __ = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::sptr event) override{
        os << std::endl;  
    }
};

class LineFormatItem : public LogFormatter::FormatItem{
public:
    LineFormatItem(const std::string& __ = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::sptr event) override{
        os << event->getLineNo();  
    }
};

class NormalStringFormatItem : public LogFormatter::FormatItem{
public:
    NormalStringFormatItem(const std::string& __ = "")
        : m_normal_str(__) {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::sptr event) override{
        os << m_normal_str;
    }
private:
    std::string m_normal_str;
};

LogFormatter::LogFormatter(const std::string& pattern)
    : m_pattern(pattern){
    init();
}

std::string LogFormatter::format(Logger::sptr logger, LogLevel::Level level, LogEvent::sptr event){
    if(nullptr == event || nullptr == logger) return {};
    std::stringstream ss;
    for(const auto&it : m_items){
        if(nullptr == it) continue;
        it->format(ss, logger, level, event);
    }
    return ss.str();
}

//%% , %xxx, %xxx{xxx}
void LogFormatter::init(){
    std::vector<std::tuple<std::string, std::string, int>> vec_pattern{};  //{pattern, format, type}
    std::string no_style_str;
    auto j = 0u;
    for(auto i = 0u; i < m_pattern.size(); ++i){
        if(m_pattern[i] != '%'){
            no_style_str.append(1, m_pattern[i]);
            continue;
        }
        
        //%% case
        if((i + 1) < m_pattern.size() && m_pattern[i+1] == '%'){
            no_style_str.append(1, '%');
            continue;
        }

        //%xxx or %xxx{xxx} case
        j = i + 1;
        int fmt_status{0};  //0 means no '{', 1 means find '{', 2 means both have '{' and '}'
        auto fmt_begin{0u};
        std::string fmt, pattern;
        while(j < m_pattern.size()){
            if(m_pattern[j] == ' ') break;

            if(fmt_status == 0){
                if(m_pattern[j] == '{'){
                    fmt_status = 1;
                    pattern = m_pattern.substr(i + 1, j - i - 1);
                    fmt_begin = j;
                    ++j;
                    continue;
                }
            }
            if(fmt_status == 1){
                if(m_pattern[j] == '}'){
                    fmt_status = 2;
                    fmt = m_pattern.substr(fmt_begin + 1, j - fmt_begin - 1);
                    ++j;
                    break;
                }
            }
            ++j;
        }

        if(fmt_status == 0){
            if(!no_style_str.empty()){
                vec_pattern.emplace_back(std::make_tuple(no_style_str, "", 0));
                no_style_str.clear();
            }
            pattern = m_pattern.substr(i + 1, j - i - 1);
            vec_pattern.emplace_back(std::make_tuple(pattern, "", 1));
            i = j;
        }else if(fmt_status == 1){
            std::cout << "Parse error " << m_pattern.substr(i) << '\n';
            vec_pattern.emplace_back(std::make_tuple("<error parse>", "", 0));
            break;
        }else if(fmt_status == 2) {
            if(!no_style_str.empty()){
                vec_pattern.emplace_back(std::make_tuple(no_style_str, "", 0));
                no_style_str.clear();
            }
            vec_pattern.emplace_back(std::make_tuple(pattern, fmt, 1));
            i = j;
        }
    }
    if(!no_style_str.empty()){
        vec_pattern.emplace_back(std::make_tuple(no_style_str, "", 0));
    }

    /*
    %m -- message body
    %p -- log level
    %r -- time since server startup
    %c -- log name
    %t -- thread id
    %n -- new line
    %d -- time
    %f -- file name
    %l -- line numble
    */

    static std::map<std::string, 
        std::function<FormatItem::sptr(const std::string& fmt)> > s_format_items {
      {"m", [](const std::string& fmt){ return FormatItem::sptr(new MessageFormatItem(fmt));}},
      {"p", [](const std::string& fmt){ return FormatItem::sptr(new LevelFormatItem(fmt));}},
      {"r", [](const std::string& fmt){ return FormatItem::sptr(new ElapseFormatItem(fmt));}},
      {"c", [](const std::string& fmt){ return FormatItem::sptr(new LoggerNameFormatItem(fmt));}},
      {"t", [](const std::string& fmt){ return FormatItem::sptr(new ThreadIdFormatItem(fmt));}},
      {"n", [](const std::string& fmt){ return FormatItem::sptr(new NewLineFormatItem(fmt));}},
      {"d", [](const std::string& fmt){ return FormatItem::sptr(new DateTimeFormatItem(fmt));}},
      {"f", [](const std::string& fmt){ return FormatItem::sptr(new FilenameFormatItem(fmt));}},
      {"l", [](const std::string& fmt){ return FormatItem::sptr(new LineFormatItem(fmt));}},
    };

    for(const auto& item : vec_pattern){
        if(std::get<2>(item) == 0){
            m_items.emplace_back(std::make_shared<NormalStringFormatItem>(std::get<0>(item)));
        }else{
            //find in s_format_items, if we can't find right format item, we will push back a error msg
            const auto cit = s_format_items.find(std::get<0>(item));
            if(cit == s_format_items.end()){
                m_items.emplace_back(std::make_shared<NormalStringFormatItem>("<<error_format %" + std::get<0>(item) + ">>"));
            }else{
                m_items.emplace_back(cit->second(std::get<1>(item)));
            }
        }


        std::cout << std::get<0>(item) << "--" << std::get<1>(item) << "--" << std::get<2>(item) << std::endl;
    }
}

Logger::Logger(const std::string& name)
    : m_name(name)
    , m_level(LogLevel::DEBUG) {
    m_formatter.reset(new LogFormatter("%d [%p] %f %l %m %n"));  //default formatter
}

void Logger::Log(LogLevel::Level level, LogEvent::sptr event){
    if(level < m_level) return;
    auto self  = shared_from_this();
    for(auto& appender : m_appenders){
        if(nullptr != appender){
            appender->Log(self, level, event);
        }
    }
}

void Logger::debug(LogEvent::sptr event){
    Log(LogLevel::DEBUG, event);
}

void Logger::info(LogEvent::sptr event){
    Log(LogLevel::INFO, event);
}

void Logger::warn(LogEvent::sptr event){
    Log(LogLevel::WARN, event);
}

void Logger::error(LogEvent::sptr event){
    Log(LogLevel::ERROR, event);
}

void Logger::fatal(LogEvent::sptr event){
    Log(LogLevel::FATAL, event);
}

void Logger::addAppender(LogAppender::sptr appender) {
    if(nullptr == appender) return;
    if(nullptr == appender->getFormatter()){
        appender->setFormatter(m_formatter);
    }
    m_appenders.emplace_back(appender);
}

void Logger::delAppender(LogAppender::sptr appender) {
    for(auto cit = m_appenders.cbegin(); cit != m_appenders.cend(); ++cit){
        if(*cit == appender){
            m_appenders.erase(cit);
            break;
        }
    }
}

void StdoutLogAppender::Log(Logger::sptr logger, LogLevel::Level level, LogEvent::sptr event)
{
    if(level < m_level || nullptr == m_formatter || nullptr == event) return;
    std::cout << m_formatter->format(logger, level, event);
}

FileLogAppender::FileLogAppender(const std::string &filename)
    : m_filename(filename)
{
}

void FileLogAppender::Log(Logger::sptr logger, LogLevel::Level level, LogEvent::sptr event)
{
    if(level < m_level || nullptr == m_formatter || nullptr == event) return ;
    m_filestream << m_formatter->format(logger, level, event);
}

bool FileLogAppender::reopen()
{
    if(m_filestream){
        m_filestream.close();
    }
    m_filestream.open(m_filename);
    return !!m_filestream;
}
} // namespace East
