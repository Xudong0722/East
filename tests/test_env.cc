/*
 * @Author: Xudong0722 
 * @Date: 2025-08-21 16:54:53 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-08-21 17:13:25
 */

#include "../East/include/env.h"
#include <iostream>

int main(int argc, char** argv){
    East::EnvMgr::GetInst()->addHelp("s", "start with the test");
    East::EnvMgr::GetInst()->addHelp("d", "run as daemon");
    East::EnvMgr::GetInst()->addHelp("p", "print help");
    bool res = East::EnvMgr::GetInst()->init(argc, argv);
    std::cout << "Parse result: " << res << std::endl;
    East::EnvMgr::GetInst()->printArgs();
    East::EnvMgr::GetInst()->removeHelp("p");
    if(East::EnvMgr::GetInst()->has("d")) {
        East::EnvMgr::GetInst()->printHelp();
    }
    return 0;
}