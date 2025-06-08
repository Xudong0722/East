/*
 * @Author: Xudong0722 
 * @Date: 2025-04-08 23:58:36 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-04-09 01:22:01
 */

#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
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

void test_sock() {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  //fcntl(sock, F_SETFL, O_NONBLOCK);

  sockaddr_in sockad;
  memset(&sockad, 0, sizeof(sockad));
  sockad.sin_addr.s_addr = inet_addr("39.156.66.10");
  sockad.sin_family = AF_INET;
  sockad.sin_port = htons(80);
  ELOG_INFO(g_logger) << "begin connect " << sock;
  int res = connect(sock, (sockaddr*)&sockad, sizeof(sockad));
  ELOG_INFO(g_logger) << "connect result: " << res << ", errno: " << errno;
  if (res)
    return;

  const char data[] = "GET / HTTP/1.0\r\n\r\n";
  res = send(sock, data, sizeof(data), 0);

  ELOG_INFO(g_logger) << "send result: " << res << ", errno: " << errno;
  if (res <= 0)
    return;

  std::string buf{};
  buf.resize(4096);
  res = recv(sock, &buf[0], buf.size(), 0);
  ELOG_INFO(g_logger) << "recv result: " << res << ", errno: " << errno;
  if (res <= 0)
    return;
  buf.resize(res);
  ELOG_INFO(g_logger) << "recv data: " << buf;
  // close(sock);
}

int main() {
  test_sleep();
  //East::IOManager io_mgr;
  //io_mgr.schedule(test_sock);
  //test_sock();
  return 0;
}