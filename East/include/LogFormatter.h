/*
 * @Author: Xudong0722
 * @Date: 2025-03-04 23:34:04
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-04 23:35:19
 */

#pragma once
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "LogEvent.h"

namespace East {
class LogFormatter {
 public:
  using sptr = std::shared_ptr<LogFormatter>;
  LogFormatter(const std::string& pattern);
  std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level,
                     LogEvent::sptr event);

 public:
  class FormatItem {
   public:
    using sptr = std::shared_ptr<FormatItem>;
    virtual ~FormatItem() {}

    virtual void format(std::ostream& os, std::shared_ptr<Logger> logger,
                        LogLevel::Level level, LogEvent::sptr event) = 0;
  };

  void init();
  bool hasError();
  std::string getPattern();

 private:
  bool m_has_error;
  std::string m_pattern;
  std::vector<FormatItem::sptr> m_items;
};
}  // namespace East