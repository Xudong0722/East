/*
 * @Author: Xudong0722 
 * @Date: 2025-03-24 18:06:14 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-24 21:27:32
 */

#include <cassert>
#include "../East/include/Elog.h"
#include "../East/include/util.h"
#include "../East/include/Macro.h"

East::Logger::sptr g_logger = ELOG_ROOT();

void func() {
  EAST_ASSERT(0 == 1);
}

void test() {
//   ELOG_INFO(g_logger) << East::BacktraceToStr(100, 2, "      ");
  func();
}

int main() {
  //assert(0);
  test();
  return 0;
}