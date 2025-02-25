/*
 * @Author: Xudong0722 
 * @Date: 2025-02-24 20:45:27 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-02-25 23:38:19
 */

#pragma once
#include <stdint.h>
#include <string>
#include <memory>
#include <list>
#include <fstream>
#include <sstream>
#include <vector>

namespace East
{

class LogEvent {
public:
    using sptr = std::shared_ptr<LogEvent>;
    LogEvent();

    const char* getFileName() const { return m_file; }
    int32_t getLineNo() const { return m_line; }
    int32_t getElapse() const { return m_elapse; }
    uint32_t getThreadId() const { return m_threadId; }
    uint32_t getFiberId() const { return m_fiberId; }
    uint64_t getTimeStamp() const { return m_time; }
    const std::string& getContent() const { return m_content; }
    
private:
    const char* m_file{nullptr};  //8 bytes on x64
    int32_t m_line{0};
    int32_t m_elapse{0};
    uint32_t m_threadId{0};
    uint32_t m_fiberId{0};
    uint64_t m_time{0};
    std::string m_content;
};

class LogLevel{
public:
    enum Level{
        NONE = 0,
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5,
    };
    static const char* toStr(Level level);
};

class LogFormatter{
public:
    using sptr = std::shared_ptr<LogFormatter>;
    LogFormatter(const std::string& pattern);
    std::string format(LogEvent::sptr event);

private:
    class FormatItem{
    public:
        using sptr = std::shared_ptr<FormatItem>;
        virtual ~FormatItem() {}
        
        virtual void format(std::ostream& os, LogEvent::sptr event);
    };

    void init();
private:
    std::string m_pattern;
    std::vector<FormatItem::sptr> m_items;
};

//Log target, std or file ...
class LogAppender{
public:
    using sptr = std::shared_ptr<LogAppender>;
    virtual ~LogAppender();

    virtual void Log(LogLevel::Level level, LogEvent::sptr event) = 0;
    
    void setFormatter(LogFormatter::sptr formatter) { m_formatter = formatter; }
    LogFormatter::sptr getFormatter() const { return m_formatter; }
protected:
    LogLevel m_level;
    LogFormatter::sptr m_formatter;
};

class Logger{
public:
    Logger(const std::string& name = "root");
    void Log(LogLevel::Level level, LogEvent::sptr event);

    void debug(LogEvent::sptr event);
    void info(LogEvent::sptr event);
    void warn(LogEvent::sptr event);
    void error(LogEvent::sptr event);
    void fatal(LogEvent::sptr event);

    LogLevel::Level getLevel() const { return m_level; }
    void setLevel(LogLevel::Level level) { m_level = level; }
    void addAppender(LogAppender::sptr appender);
    void delAppender(LogAppender::sptr appender);
private:
    std::string m_name;             //log name
    LogLevel::Level m_level{LogLevel::NONE};        //log level(output when event level >= m_level)
    std::list<LogAppender::sptr> m_appenders;
};

class StdoutLogAppender: public LogAppender{
public:
    using sptr = std::shared_ptr<StdoutLogAppender>;
    void Log(LogLevel::Level level, LogEvent::sptr event) override;
};

class FileLogAppender: public LogAppender{
public:
    using sptr = std::shared_ptr<FileLogAppender>;
    FileLogAppender(const std::string& filename);

    void Log(LogLevel::Level level, LogEvent::sptr event) override;

    bool reopen(); //TODO, Why add this api?
private:
    std::string m_filename;
    std::ofstream m_filestream;
};
} // namespace East
