/*
 * @Author: Xudong0722 
 * @Date: 2025-06-18 23:50:05 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-19 01:21:24
 */

#include "../East/http/HttpConnection.h"
#include "../East/include/Elog.h"
#include "../East/include/IOManager.h"
#include "../East/include/util.h"

static East::Logger::sptr g_logger = ELOG_ROOT();

void run() {
  East::Address::sptr addr =
      East::Address::LookupAnyIPAddress("www.baidu.com:80");
  if (!addr) {
    ELOG_INFO(g_logger) << "get addr error";
    return;
  }

  East::Socket::sptr sock = East::Socket::CreateTCP(addr);
  bool rt = sock->connect(addr);
  if (!rt) {
    ELOG_INFO(g_logger) << "connect " << *addr << " failed.";
    return;
  }

  auto conn = std::make_shared<East::Http::HttpConnection>(sock);
  auto req = std::make_shared<East::Http::HttpReq>();
  ELOG_INFO(g_logger) << "req: " << *req;
  req->setHeader("host", "www.baidu.com");
  //req->setPath("/test/");
  conn->sendRequest(req);
  auto rsp = conn->recvResponse();
  if (!rsp) {
    ELOG_INFO(g_logger) << "recv response error";
    return;
  }

  ELOG_INFO(g_logger) << "rsp: " << *rsp;
}

void run2() {
  East::Http::HttpResult::sptr res = East::Http::HttpConnection::DoGet(
      "http://www.baidu.com:80", {}, {}, 3000);

  if (res->result != East::Enum2Utype(East::Http::HttpResult::ErrorCode::OK)) {
    ELOG_INFO(g_logger) << "DoGet error: " << res->error;
    return;
  }
  ELOG_INFO(g_logger) << "DoGet response: " << *res->resp;
}

int main() {
  East::IOManager iom(2);
  //iom.schedule(run);
  iom.schedule(run2);

  return 0;
}