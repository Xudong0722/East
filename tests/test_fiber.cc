/*
 * @Author: Xudong0722 
 * @Date: 2025-04-01 22:54:06 
 * @Last Modified by:   Xudong0722 
 * @Last Modified time: 2025-04-01 22:54:06 
 */
#include "../East/include/Elog.h"
#include "../East/include/Fiber.h"

East::Logger::sptr g_logger = ELOG_ROOT();

void func() {
  ELOG_INFO(g_logger) << "func begin";
  East::Fiber::YieldToHold();
  ELOG_INFO(g_logger) << "func end";
  East::Fiber::YieldToHold();
}

int main() {
  East::Fiber::GetThis();
  ELOG_INFO(g_logger) << "main begin";
  East::Fiber::sptr fiber(new East::Fiber(func));
  fiber->resume();
  ELOG_INFO(g_logger) << "main after swapIn";
  fiber->resume();
  ELOG_INFO(g_logger) << "main after end";
  fiber->resume();
  ELOG_INFO(g_logger) << "main after end2";
}