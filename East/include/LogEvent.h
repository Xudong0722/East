/*
 * @Author: Xudong0722
 * @Date: 2025-03-04 23:25:56
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-21 10:56:43
 */

#pragma once
#include <cstdarg>
#include <memory>
#include <sstream>
#include <string>

namespace East {
class Logger;
class LogLevel {
 public:
  enum Level {
    NONE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5,
  };
  static const char* toStr(Level level);
  static Level fromStr(const std::string& str);
};

class LogEvent {
 public:
  using sptr = std::shared_ptr<LogEvent>;
  LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
           const char* file, int32_t line, int32_t elapse, uint32_t thread_id,
           uint32_t fiber_id, uint64_t time);

  const char* getFileName() const { return m_file; }
  int32_t getLineNo() const { return m_line; }
  int32_t getElapse() const { return m_elapse; }
  uint32_t getThreadId() const { return m_threadId; }
  uint32_t getFiberId() const { return m_fiberId; }
  uint64_t getTimeStamp() const { return m_time; }
  std::string getContent() const { return m_ss.str(); }
  LogLevel::Level getLevel() const { return m_level; }
  std::shared_ptr<Logger> getLogger() { return m_logger; }
  std::stringstream& getSStream() { return m_ss; }

  void format(const char* fmt, ...);
  void format(const char* fmt, va_list al);

 private:
  const char* m_file{nullptr};  // 8 bytes on x64
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

class LogEventWrap {
 public:
  LogEventWrap(LogEvent::sptr event);
  ~LogEventWrap();

  std::stringstream& getSStream();
  LogEvent::sptr getEvent();

 private:
  LogEvent::sptr m_event;
};
}  // namespace East