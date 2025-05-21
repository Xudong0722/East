/*
 * @Author: Xudong0722 
 * @Date: 2025-05-21 23:28:10 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-05-21 23:34:38
 */

#include "../East/http/Http.h"
#include "../East/include/Elog.h"

void test() {
  East::Http::HttpReq::sptr req = std::make_shared<East::Http::HttpReq>();
  req->setHeader("host", "www.baidu.com");
  req->setBody("hello world");

  req->dump(std::cout) << std::endl;
}

int main() {
  test();
  return 0;
}