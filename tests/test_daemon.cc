/*
 * @Author: Xudong0722 
 * @Date: 2025-08-20 22:59:00 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-08-21 00:36:18
 */

#include "../East/include/Elog.h"
#include "../East/include/IOManager.h"
#include "../East/include/daemon.h"

static East::Logger::sptr g_logger = ELOG_ROOT();

East::Timer::sptr timer{};

int server_main(int argc, char** argv) {
  East::IOManager iom(1);
  ELOG_INFO(g_logger) << East::ProcessInfoMgr::GetInst()->to_str();

  timer = iom.addTimer(
      1000,
      []() {
        ELOG_INFO(g_logger) << "OnTimer";
        static int count{0};
        if (count++ > 30) {
          timer->cancel();
        }
      },
      true);
  return 0;
}

int main(int argc, char** argv) {
  return East::start_daemon(argc, argv, server_main, argc != 1);
}