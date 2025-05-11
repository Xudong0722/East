/*
 * @Author: Xudong0722 
 * @Date: 2025-05-11 18:26:13 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-05-11 18:44:57
 */

#include "../East/include/Elog.h"
#include "../East/include/IOManager.h"
#include "../East/include/Socket.h"

static East::Logger::sptr g_logger = ELOG_NAME("root");
void test_socket() {
  East::IPAddress::sptr addr =
      East::Address::LookupAnyIPAddress("www.baidu.com");
  if (nullptr == addr) {
    ELOG_ERROR(g_logger) << "LookupAnyIPAddress failed";
    return;
  } else {
    ELOG_INFO(g_logger) << "LookupAnyIPAddress: " << addr->toString();
  }

  East::Socket::sptr sock = East::Socket::CreateTCP(addr);
  if (nullptr == sock)
    return;

  addr->setPort(80);
  if (!sock->connect(addr)) {
    ELOG_ERROR(g_logger) << "connect failed";
    return;
  } else {
    ELOG_INFO(g_logger) << "connect " << addr->toString() << " success";
  }

  const char buf[] = "GET / HTTP/1.0\r\n\r\n";
  int res = sock->send(buf, sizeof(buf));
  if (res <= 0) {
    ELOG_ERROR(g_logger) << "send failed";
    return;
  }

  std::string rcv_buf;
  rcv_buf.resize(4096);
  res = sock->recv(&rcv_buf[0], rcv_buf.size());
  if (res <= 0) {
    ELOG_ERROR(g_logger) << "recv failed";
    return;
  }
  rcv_buf.resize(res);
  ELOG_INFO(g_logger) << "recv: " << rcv_buf;
}

int main() {
  East::IOManager iom;
  iom.schedule(test_socket);
  iom.stop();
  ELOG_INFO(g_logger) << "test_socket finished";
  return 0;
}