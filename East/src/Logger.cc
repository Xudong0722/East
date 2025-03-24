/*
 * @Author: Xudong0722
 * @Date: 2025-03-04 23:57:19
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-24 12:37:58
 */

#include "Logger.h"
#include <iostream>
#include "Config.h"
#include "LogAppender.h"
#include "LogFormatter.h"

namespace East {
Logger::Logger(const std::string& name)
    : m_name(name), m_level(LogLevel::DEBUG) {
  m_formatter.reset(new LogFormatter(
      "%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));  // default formatter
}

void Logger::Log(LogLevel::Level level, LogEvent::sptr event) {
  if (level < m_level)
    return;
  auto self = shared_from_this();
  MutexType::LockGuard lock(m_mutex);
  if (!m_appenders.empty()) {
    for (auto& appender : m_appenders) {
      if (nullptr != appender) {
        appender->Log(self, level, event);
      }
    }
  } else {
    if (nullptr != m_root)
      m_root->Log(level, event);
  }
}

void Logger::debug(LogEvent::sptr event) {
  Log(LogLevel::DEBUG, event);
}

void Logger::info(LogEvent::sptr event) {
  Log(LogLevel::INFO, event);
}

void Logger::warn(LogEvent::sptr event) {
  Log(LogLevel::WARN, event);
}

void Logger::error(LogEvent::sptr event) {
  Log(LogLevel::ERROR, event);
}

void Logger::fatal(LogEvent::sptr event) {
  Log(LogLevel::FATAL, event);
}

void Logger::addAppender(LogAppender::sptr appender) {
  MutexType::LockGuard lock(m_mutex);
  if (nullptr == appender)
    return;
  if (nullptr == appender->getFormatter()) {
    MutexType::LockGuard alock(appender->m_mutex);  //TODO
    // appender->setFormatter(m_formatter);dead lock with if(xxx)
    appender->m_formatter = m_formatter;
  }
  m_appenders.emplace_back(appender);
}

void Logger::delAppender(LogAppender::sptr appender) {
  MutexType::LockGuard lock(m_mutex);
  for (auto cit = m_appenders.cbegin(); cit != m_appenders.cend(); ++cit) {
    if (*cit == appender) {
      m_appenders.erase(cit);
      break;
    }
  }
}

void Logger::clearAppenders() {
  MutexType::LockGuard lock(m_mutex);
  m_appenders.clear();
}

std::shared_ptr<LogFormatter> Logger::getFormatter() {
  MutexType::LockGuard lock(m_mutex);
  return m_formatter;
}

void Logger::setFormatter(std::shared_ptr<LogFormatter> formatter) {
  MutexType::LockGuard lock(m_mutex);
  if (nullptr == formatter || formatter->hasError())
    return;
  m_formatter = formatter;
  for (auto& apd : m_appenders) {
    MutexType::LockGuard alock(apd->m_mutex);
    if (nullptr == apd)
      continue;
    //如果appender没有formatter，再设置进去
    if (!apd->m_has_formatter)
      apd->m_formatter = m_formatter;  //这并不是appender自己的，所以直接修改
  }
}

void Logger::setFormatter(const std::string& pattern) {
  std::shared_ptr<LogFormatter> new_formatter =
      std::make_shared<LogFormatter>(pattern);
  if (nullptr == new_formatter || new_formatter->hasError())
    return;
  setFormatter(new_formatter);
}

std::string Logger::toYamlString() {
  MutexType::LockGuard lock(m_mutex);
  YAML::Node node{};

  node["name"] = m_name;
  if (m_level != LogLevel::NONE)
    node["level"] = LogLevel::toStr(m_level);

  if (nullptr != m_formatter)
    node["formatter"] = m_formatter->getPattern();

  for (const auto& ap : m_appenders)
    node["appenders"].push_back(YAML::Load(ap->toYamlString()));

  std::stringstream ss;
  ss << node;
  return ss.str();
}

LoggerMgr::LoggerMgr() {
  m_root.reset(new Logger);
  m_root->addAppender(std::make_shared<StdoutLogAppender>());

  m_loggers.emplace(m_root->getName(), m_root);
  init();
}

void LoggerMgr::init() {}

std::string LoggerMgr::toYamlString() {
  MutexType::LockGuard lock(m_mutex);
  YAML::Node node{};

  for (const auto& [_, logger] : m_loggers) {
    if (nullptr == logger)
      continue;
    node.push_back(YAML::Load(logger->toYamlString()));
  }

  std::stringstream ss;
  ss << node;
  return ss.str();
}

Logger::sptr LoggerMgr::getLogger(const std::string& name) {
  MutexType::LockGuard lock(m_mutex);
  auto it = m_loggers.find(name);
  if (it == m_loggers.end()) {
    Logger::sptr new_logger = std::make_shared<Logger>(name);
    new_logger->setRoot(m_root);
    m_loggers.emplace(name, new_logger);
    return new_logger;
  }
  return it->second;
}

// support LoggerConfigInfo -> string
template <>
class LexicalCast<LoggerConfigInfo, std::string> {
 public:
  std::string operator()(const LoggerConfigInfo& log_config) {
    YAML::Node node{};
    node["name"] = log_config.name;
    if (log_config.level >= LogLevel::DEBUG &&
        log_config.level <= LogLevel::FATAL)
      node["level"] = LogLevel::toStr(log_config.level);
    if (!log_config.formatter.empty())
      node["formatter"] = log_config.formatter;
    for (const auto& ap : log_config.log_appenders) {
      YAML::Node ap_node{};
      if (ap.type == LogAppenderType::FILE) {
        ap_node["type"] = "FileLogAppender";
        if (!ap.file_path.empty())
          ap_node["path"] = ap.file_path;
      } else if (ap.type == LogAppenderType::STANDARD)
        ap_node["type"] = "StdoutLogAppender";

      if (!ap.formatter.empty())
        ap_node["formatter"] = ap.formatter;
      node["appenders"].push_back(ap_node);
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

// support string -> LoggerConfigInfo
template <>
class LexicalCast<std::string, LoggerConfigInfo> {
 public:
  LoggerConfigInfo operator()(const std::string& str) {
    LoggerConfigInfo logger_config{};
    YAML::Node node = YAML::Load(str);
    if (!node["name"].IsDefined()) {
      std::cout << "Log config error: name is null, " << node << std::endl;
      return logger_config;
    }
    logger_config.name = node["name"].as<std::string>();
    logger_config.level = LogLevel::fromStr(
        node["level"].IsDefined() ? node["level"].as<std::string>() : "");
    logger_config.formatter = node["formatter"].IsDefined()
                                  ? node["formatter"].as<std::string>()
                                  : "";

    if (node["appenders"].IsDefined()) {
      for (size_t i = 0; i < node["appenders"].size(); ++i) {
        auto appender = node["appenders"][i];
        if (!appender["type"].IsDefined()) {
          std::cout << "Log appender error: type is null, " << appender
                    << std::endl;
          continue;
        }
        LogAppenderConfigInfo log_appender{};
        std::string type = appender["type"].as<std::string>();
        if (type == "FileLogAppender") {
          log_appender.type = LogAppenderType::FILE;
          if (!appender["file"].IsDefined()) {
            std::cout << "Log appender error: file is null, " << appender
                      << std::endl;
            continue;
          }
          log_appender.file_path = appender["file"].as<std::string>();
          if (appender["formatter"].IsDefined()) {
            log_appender.formatter = appender["formatter"].as<std::string>();
          }
        } else if (type == "StdoutLogAppender") {
          log_appender.type = LogAppenderType::STANDARD;
        } else {
          std::cout << "Log appender error: type undefined, " << appender
                    << std::endl;
          continue;
        }
        logger_config.log_appenders.emplace_back(log_appender);
      }
    }
    return logger_config;
  }
};

ConfigVar<std::set<LoggerConfigInfo>>::sptr g_log_config =
    Config::Lookup("logs", std::set<LoggerConfigInfo>{}, "log config");

LogInitiator::LogInitiator() {
  g_log_config->addListener(
      LOG_INITIATOR_CALLBACK_ID, [](const std::set<LoggerConfigInfo>& old_v,
                                    const std::set<LoggerConfigInfo>& new_v) {
        ELOG_INFO(ELOG_ROOT())
            << "on log config changed, old size: " << old_v.size()
            << ", new size: " << new_v.size();

        //1.add 2.del 3.modify
        for (const auto& item : new_v) {
          auto it = old_v.find(item);
          std::shared_ptr<Logger> logger{};
          if (it == old_v.end()) {
            //add event
            logger = ELOG_NAME(item.name);
          } else {
            if (!(*it == item)) {
              //modify event
              logger = ELOG_NAME(item.name);
            } else {
              continue;
            }
          }

          logger->setLevel(item.level);
          if (!item.formatter.empty())
            logger->setFormatter(item.formatter);

          logger->clearAppenders();
          for (const auto& ap : item.log_appenders) {
            std::shared_ptr<LogAppender> log_appender{};
            if (ap.type == LogAppenderType::FILE) {
              log_appender.reset(new FileLogAppender(ap.file_path));
            } else if (ap.type == LogAppenderType::STANDARD) {
              log_appender.reset(new StdoutLogAppender());
            }

            log_appender->setLevel(ap.level);
            if (!ap.formatter.empty()) {
              std::shared_ptr<LogFormatter> appender_formatter =
                  std::make_shared<LogFormatter>(ap.formatter);
              if (nullptr != appender_formatter &&
                  !appender_formatter->hasError())
                log_appender->setFormatter(appender_formatter);
            }
            logger->addAppender(log_appender);
          }
        }

        for (const auto& item : old_v) {
          auto it = new_v.find(item);
          if (it == new_v.end())  // delete event
          {
            auto logger = ELOG_NAME(item.name);
            logger->clearAppenders();
            logger->setLevel(LogLevel::Level::NONE);
          }
        }
      });
}

static LogInitiator g_log_initiator;  // init before exe main func
}  // namespace East