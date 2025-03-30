#include <iostream>
#include "../East/include/Elog.h"
#include "../East/include/util.h"

int main() {
  East::Logger::sptr logger(new East::Logger);
  logger->addAppender(East::LogAppender::sptr(new East::StdoutLogAppender));

  auto event = std::make_shared<East::LogEvent>(
      logger, East::LogLevel::FATAL, __FILE__, __LINE__, 0, East::GetThreadId(),
      2, time(0));
  event->getSStream() << "first log! " << std::this_thread::get_id();
  logger->Log(East::LogLevel::DEBUG, event);
  std::cout << event->getContent() << std::endl;
  std::cout << event->getLevel() << " " << logger->getLevel() << std::endl;

  // std::cout << static_cast<size_t>(pthread_self()) <<" " << std::hash<std::thread::id>{}(std::this_thread::get_id());

  East::FileLogAppender::sptr file_appender =
      std::make_shared<East::FileLogAppender>("./log.txt");
  file_appender->setLevel(East::LogLevel::ERROR);
  logger->addAppender(file_appender);
  std::cout << std::endl;

  int a{100101};
  std::string msg{"hello"};
  ELOG_DEBUG(logger) << "debug log1 "
                     << "debug 2:" << a;
  ELOG_INFO(logger) << "INFO log1";
  ELOG_WARN(logger) << "warn log2";
  ELOG_FATAL(logger) << "Fatal error!";
  ELOG_ERROR(logger) << "error msg:" << msg;

  std::cout << "---------\n";
  auto logger1 = East::LogMgr::GetInst()->getLogger("test");
  ELOG_ERROR(logger1) << "test log";

  std::cout << "-------------test fmt log\n";

  ELOG_FMT_DEBUG(logger, "%d%s%3f", 100, "stringstring", 3.1415926);
  ELOG_FMT_INFO(logger, "%d%s%3f", 100, "stringstring", 3.1415926);
  ELOG_FMT_WARN(logger, "%d%s%3f", 100, "stringstring", 3.1415926);
  ELOG_FMT_FATAL(logger, "%d%s%3f", 100, "stringstring", 3.1415926);
  ELOG_FMT_ERROR(logger, "%d%s%3f", 100, "stringstring", 3.1415926);
  return 0;
}