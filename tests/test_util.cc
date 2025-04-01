/*
 * @Author: Xudong0722 
 * @Date: 2025-03-24 18:06:14 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-04-01 22:54:13
 */

#include <cassert>
#include <iostream>
#include <memory>
#include "../East/include/Elog.h"
#include "../East/include/Macro.h"
#include "../East/include/util.h"

East::Logger::sptr g_logger = ELOG_ROOT();

void func() {
  //EAST_ASSERT(0 == 1);
}

void test() {
  //   ELOG_INFO(g_logger) << East::BacktraceToStr(100, 2, "      ");
  func();
}

class A : public std::enable_shared_from_this<A> {
 public:
  A() {}
  std::shared_ptr<A> getA() {
    auto self = shared_from_this();
    std::cout << __FUNCTION__ << " " << self.use_count() << std::endl;
    return self;
  }
};

int main() {
  //assert(0);
  //test();
  auto p = std::make_shared<A>();
  std::cout << p.use_count() << " \n";
  auto q = p->getA();
  std::cout << "after geta, use count: " << q.use_count();
  return 0;
}