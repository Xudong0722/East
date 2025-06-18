/*
 * @Author: Xudong0722 
 * @Date: 2025-06-18 23:50:05 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-19 00:16:37
 */

#include "../East/include/Elog.h"
#include "../East/include/IOManager.h"
#include "../East/http/HttpConnection.h"

static East::Logger::sptr g_logger = ELOG_ROOT();

void run() {
  East::Address::sptr addr = East::Address::LookupAnyIPAddress("www.baidu.com:80");
  if(!addr) {
    ELOG_INFO(g_logger) << "get addr error";
    return ;
  }

  East::Socket::sptr sock = East::Socket::CreateTCP(addr);
  bool rt = sock->connect(addr);
  if(!rt) {
    ELOG_INFO(g_logger) << "connect " << *addr << " failed.";
    return ;
  }

  auto conn = std::make_shared<East::Http::HttpConnection>(sock);
  auto req = std::make_shared<East::Http::HttpReq>();
  ELOG_INFO(g_logger) << "req: " << *req;
  req->setHeader("host", "www.baidu.com");
  conn->sendRequest(req);
  auto rsp = conn->recvResponse();
  if(!rsp) {
    ELOG_INFO(g_logger) << "recv response error";
    return ;
  }

  ELOG_INFO(g_logger) << "rsp: " << *rsp;
}

int main() {
  East::IOManager iom(2);
  iom.schedule(run);
  return 0;
}