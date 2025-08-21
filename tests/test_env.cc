/*
 * @Author: Xudong0722 
 * @Date: 2025-08-21 16:54:53 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-08-21 21:19:50
 */

#include <iostream>
#include "../East/include/env.h"

int main(int argc, char** argv) {
  East::EnvMgr::GetInst()->addHelp("s", "start with the test");
  East::EnvMgr::GetInst()->addHelp("d", "run as daemon");
  East::EnvMgr::GetInst()->addHelp("p", "print help");
  bool res = East::EnvMgr::GetInst()->init(argc, argv);
  std::cout << "Parse result: " << res << std::endl;
  East::EnvMgr::GetInst()->printArgs();
  East::EnvMgr::GetInst()->removeHelp("p");
  if (East::EnvMgr::GetInst()->has("d")) {
    East::EnvMgr::GetInst()->printHelp();
  }

  std::cout << East::EnvMgr::GetInst()->getExe() << '\n';
  std::cout << East::EnvMgr::GetInst()->getCwd() << '\n';

  std::cout << "getenv, path: " << East::EnvMgr::GetInst()->getEnv("PATH", "xxx") << '\n';
  return 0;
}