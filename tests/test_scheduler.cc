#include "../East/include/Scheduler.h"
#include "../East/include/Elog.h"

East::Logger::sptr g_logger = ELOG_ROOT();

void test_fiber() {
    ELOG_INFO(g_logger) << "test fiber";
}

int main() {
    ELOG_INFO(g_logger) << "main start";
    East::Scheduler scheduler(2, true, "test_scheduler");
    scheduler.start();
    scheduler.schedule(&test_fiber);
    scheduler.stop();
    ELOG_INFO(g_logger) << "main end";
    return 0;     
}