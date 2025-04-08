/*
 * @Author: Xudong0722 
 * @Date: 2025-04-02 17:38:22 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-04-02 23:54:16
 */

#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "../East/include/Elog.h"
#include "../East/include/IOManager.h"

East::Logger::sptr g_logger = ELOG_ROOT();

int sock = 0;
void test_fiber() {
  ELOG_INFO(g_logger) << "test fiber";

  sock = socket(AF_INET, SOCK_STREAM, 0);
  fcntl(sock, F_SETFL, O_NONBLOCK);

  sockaddr_in sockad;
  memset(&sockad, 0, sizeof(sockad));
  sockad.sin_addr.s_addr = inet_addr("110.242.68.66");
  sockad.sin_family = AF_INET;
  sockad.sin_port = htons(80);
  ELOG_INFO(g_logger) << "sock fd: " << sock;
  if (!connect(sock, (sockaddr*)&sockad, sizeof(sockad))) {

  } else if (errno == EINPROGRESS) {
    East::IOManager::GetThis()->addEvent(sock, East::IOManager::READ, []() {
      ELOG_INFO(g_logger) << "read callback";
    });

    East::IOManager::GetThis()->addEvent(sock, East::IOManager::WRITE, []() {
      ELOG_INFO(g_logger) << "write callback";
      East::IOManager::GetThis()->cancelEvent(sock, East::IOManager::READ);
      close(sock);
      //shutdown(sock, SHUT_WR);
    });
  }
}

void test_iomgr() {
  East::IOManager io_mgr(2, true, "test_iomgr");
  io_mgr.schedule(&test_fiber);
}

void test_timer() {
  East::IOManager io_mgr(2);
  East::Timer::sptr timer = io_mgr.addTimer(
      1000,
      [&timer]() {
        static int i = 0;
        ELOG_INFO(g_logger) << "timer callback , i = " << i;
        if (++i == 5) {
          //timer->cancel();  //test pass
          //timer->reset(2000, true); //test pass
          //timer->refresh();  //test pass
          timer->reset(2000, true);
        }
      },
      true);

  East::Timer::sptr timer2 = io_mgr.addTimer(
      2000,
      [&timer2]() {
        static int j = 0;
        ELOG_INFO(g_logger) << "timer2 callback , j = " << j;
        if (++j == 5) {
          //timer2->cancel();  //test pass
          //timer2->reset(2000, true); //test pass
          //timer2->refresh();  //test pass
          timer2->reset(1000, true);
        }
      },
      true);
  // io_mgr.start();
}

int main() {
  //test_iomgr();
  test_timer();
}