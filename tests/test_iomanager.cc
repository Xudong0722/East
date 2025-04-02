/*
 * @Author: Xudong0722 
 * @Date: 2025-04-02 17:38:22 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-04-02 22:27:57
 */

#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
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
  if(!connect(sock, (sockaddr*)&sockad, sizeof(sockad))){
    
  }else if(errno == EINPROGRESS) {
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
  East::IOManager io_mgr;
  io_mgr.schedule(&test_fiber);
}

int main() {
  test_iomgr();
}