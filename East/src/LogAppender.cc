/*
 * @Author: Xudong0722
 * @Date: 2025-03-04 23:58:44
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-24 12:38:30
 */
#include "LogAppender.h"
#include <yaml-cpp/yaml.h>
#include <iostream>
#include "LogFormatter.h"
#include "Logger.h"

namespace East {
void LogAppender::setFormatter(std::shared_ptr<LogFormatter> formatter) {
  MutexType::LockGuard lock(m_mutex);
  m_formatter = formatter;
  if (nullptr != m_formatter)
    m_has_formatter = true;
  else
    m_has_formatter = false;
}

std::shared_ptr<LogFormatter> LogAppender::getFormatter() {
  MutexType::LockGuard lock(m_mutex);
  return m_formatter;
}

void StdoutLogAppender::Log(Logger::sptr logger, LogLevel::Level level,
                            LogEvent::sptr event) {
  if (level < m_level || nullptr == m_formatter || nullptr == event)
    return;
  MutexType::LockGuard lock(m_mutex);
  std::cout << m_formatter->format(logger, level, event);
}

std::string StdoutLogAppender::toYamlString() {
  MutexType::LockGuard lock(m_mutex);
  YAML::Node node{};
  std::stringstream ss;

  node["type"] = "StdoutLogAppender";
  if (m_level != LogLevel::NONE)
    node["level"] = LogLevel::toStr(m_level);
  if (m_has_formatter && nullptr != m_formatter)
    node["formatter"] = m_formatter->getPattern();

  ss << node;
  return ss.str();
}

FileLogAppender::FileLogAppender(const std::string& filename)
    : m_filename(filename) {
  reopen();
}

FileLogAppender::~FileLogAppender() {
  if (m_filestream) {
    m_filestream.close();
  }
}

void FileLogAppender::Log(Logger::sptr logger, LogLevel::Level level,
                          LogEvent::sptr event) {
  if (level < m_level || nullptr == m_formatter || nullptr == event)
    return;
  MutexType::LockGuard lock(m_mutex);
  m_filestream << m_formatter->format(logger, level, event);
}

bool FileLogAppender::reopen() {
  MutexType::LockGuard lock(m_mutex);
  if (m_filestream) {
    m_filestream.close();
  }
  m_filestream.open(m_filename);
  return !!m_filestream;
}

std::string FileLogAppender::toYamlString() {
  MutexType::LockGuard lock(m_mutex);
  YAML::Node node{};
  node["type"] = "FileLogAppender";
  node["file"] = m_filename;

  if (m_level != LogLevel::NONE)
    node["level"] = LogLevel::toStr(m_level);
  if (m_has_formatter && nullptr != m_formatter)
    node["formatter"] = m_formatter->getPattern();

  std::stringstream ss;
  ss << node;
  return ss.str();
}
}  // namespace East