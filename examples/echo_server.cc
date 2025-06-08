/*
 * @Author: Xudong0722 
 * @Date: 2025-06-08 21:54:47 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-08 21:55:10
 */

#include "../East/include/TcpServer.h"
#include "../East/include/Elog.h"

East::Logger::sptr g_logger = ELOG_ROOT();

void run() {
    auto addr = East::Address::LookupAny("0.0.0.0:8033");
    //auto addr2 = std::make_shared<East::UnixAddress>("/tmp/unix_addr");
    ELOG_INFO(g_logger) << *addr;// << " - " << *addr2;
    std::vector<East::Address::sptr> addrs{addr};
    East::TcpServer::sptr tcp_server = std::make_shared<East::TcpServer>();

    std::vector<East::Address::sptr> fails;
    while(!tcp_server->bind(addrs, fails)) {
        sleep(2);
    }

    tcp_server->start();
}

int main() {
    East::IOManager iom(1);
    iom.schedule(run);
    return 0;
}