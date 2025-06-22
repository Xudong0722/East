/*
 * @Author: Xudong0722 
 * @Date: 2025-06-22 12:41:47 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-22 12:51:58
 */

#include "../East/include/uri.h"
#include <iostream>

int main() {
  East::Uri::sptr uri = East::Uri::Create("http://admin@www.baidu.com:8080/test/中文/uri?id=100&name=East&vv=中文#frg中文");
  std::cout << "URI: " << uri->toString() << std::endl;
  auto addr = uri->createAddress();
  std::cout << "Address: " << addr->toString() << std::endl;
  return 0;
}