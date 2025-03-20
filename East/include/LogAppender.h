/*
 * @Author: Xudong0722
 * @Date: 2025-03-04 23:35:46
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-04 23:38:27
 */

#pragma once
#include <fstream>
#include <memory>
#include "LogEvent.h"

namespace East {
// Log target, std or file ...
class LogFormatter;
class LogAppender {
  friend class Logger;

 public:
  using sptr = std::shared_ptr<LogAppender>;
  virtual ~LogAppender() {}

  virtual void Log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                   LogEvent::sptr event) = 0;
  virtual std::string toYamlString() = 0;
  void setFormatter(std::shared_ptr<LogFormatter> formatter);
  std::shared_ptr<LogFormatter> getFormatter() const;

  void setLevel(LogLevel::Level level) { m_level = level; }
  LogLevel::Level getLevel() const { return m_level; }

 protected:
  bool m_has_formatter{false};
  LogLevel::Level m_level{LogLevel::DEBUG};
  std::shared_ptr<LogFormatter> m_formatter;
};

class StdoutLogAppender : public LogAppender {
 public:
  using sptr = std::shared_ptr<StdoutLogAppender>;
  void Log(std::shared_ptr<Logger> logger, LogLevel::Level level,
           LogEvent::sptr event) override;
  std::string toYamlString() override;
};

class FileLogAppender : public LogAppender {
 public:
  using sptr = std::shared_ptr<FileLogAppender>;
  FileLogAppender(const std::string& filename);
  ~FileLogAppender();
  void Log(std::shared_ptr<Logger> logger, LogLevel::Level level,
           LogEvent::sptr event) override;
  std::string toYamlString() override;

  bool reopen();  // TODO, Why add this api?
 private:
  std::string m_filename;
  std::ofstream m_filestream;
};
}  // namespace East