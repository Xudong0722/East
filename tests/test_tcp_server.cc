/*
 * @Author: Xudong0722 
 * @Date: 2025-06-06 15:01:50 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-06 15:28:26
 */

#include "../East/include/TcpServer.h"
#include "../East/include/IOManager.h"
#include "../East/include/Elog.h"

East::Logger::sptr g_logger = ELOG_ROOT();

void run() {
    auto addr = East::Address::LookupAny("0.0.0.0");
    ELOG_INFO(g_logger) << *addr;

}

int main() {
    East::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
