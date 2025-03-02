#include "../East/include/log.h"
#include <iostream>

int main()
{   
    East::Logger::sptr logger(new East::Logger);
    logger->addAppender(East::LogAppender::sptr(new East::StdoutLogAppender));

    auto event = std::make_shared<East::LogEvent>(__FILE__, __LINE__, 0, 1, 2, time(0));
    event->getsstream() << "first log!";
    logger->Log(East::LogLevel::DEBUG, event);
    return 0;
}