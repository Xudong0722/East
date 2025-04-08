/*
 * @Author: Xudong0722 
 * @Date: 2025-04-08 23:58:36 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-04-09 01:22:01
 */

#include "../East/include/Elog.h"
#include "../East/include/Hook.h"
#include "../East/include/IOManager.h"
using namespace East;

static East::Logger::sptr g_logger = ELOG_ROOT();

void test_sleep() {
    //TODO
  ELOG_INFO(g_logger) << "test sleep";
  IOManager io_mgr(1);
  io_mgr.schedule([]() {
    ELOG_INFO(g_logger) << "sleep 3s start";
    sleep(3);
    ELOG_INFO(g_logger) << "sleep 3s end";
  });

  io_mgr.schedule([]() {
    ELOG_INFO(g_logger) << "sleep 4s start";
    sleep(4);
    ELOG_INFO(g_logger) << "sleep 4s end";
  });

  ELOG_INFO(g_logger) << "test sleep end";
//   sleep(10);
}

int main() {
  test_sleep();
  return 0;
}