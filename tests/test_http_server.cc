/*
 * @Author: Xudong0722 
 * @Date: 2025-06-11 22:36:10 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-11 22:39:48
 */

#include "../East/include/Elog.h"
#include "../East/include/IOManager.h"
#include "../East/http/HttpServer.h"

static East::Logger::sptr g_logger = ELOG_ROOT();

void test(){
  auto server = std::make_shared<East::Http::HttpServer>();
  East::Address::sptr addr = East::Address::LookupAnyIPAddress("0.0.0.0:8020");
  while(!server->bind(addr)) {
    sleep(2);
  }
  server->start();
}

int main(){
  East::IOManager iom(2);
  iom.schedule(test);
  return 0;
}