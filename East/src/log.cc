/*
 * @Author: Xudong0722 
 * @Date: 2025-02-25 21:52:46 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-02-26 00:13:31
 */
#include "log.h"
#include <iostream>
#include <cassert>
#include <tuple>

namespace East{

const char* LogLevel::toStr(Level level)
{
    static const char* str_levels[] = {"NONE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
    assert(level >= DEBUG && level <= FATAL);
    return str_levels[level];
}

LogFormatter::LogFormatter(const std::string& pattern)
    : m_pattern(pattern){
}

std::string LogFormatter::format(LogEvent::sptr event){
    if(nullptr == event) return {};
    std::stringstream ss;
    for(const auto&it : m_items){
        if(nullptr == it) continue;
        it->format(ss, event);
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
}

Logger::Logger(const std::string& name)
    : m_name(name) {
}

void Logger::Log(LogLevel::Level level, LogEvent::sptr event){
    if(level < m_level) return;
    for(auto& appender : m_appenders){
        if(nullptr != appender){
            appender->Log(level, event);
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

void StdoutLogAppender::Log(LogLevel::Level level, LogEvent::sptr event)
{
    if(level < m_level || nullptr == m_formatter || nullptr == event) return;
    std::cout << m_formatter->format(event);
}

FileLogAppender::FileLogAppender(const std::string &filename)
    : m_filename(filename)
{
}

void FileLogAppender::Log(LogLevel::Level level, LogEvent::sptr event)
{
    if(level < m_level || nullptr == m_formatter || nullptr == event) return ;
    m_filestream << m_formatter->format(event);
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
