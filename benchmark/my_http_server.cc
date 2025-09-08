#include "../East/http/HttpServer.h"
#include "../East/include/IOManager.h"
#include "../East/include/Elog.h"

East::Logger::sptr g_logger = ELOG_ROOT();

void run() {
    g_logger->setLevel(East::LogLevel::INFO);
    East::Address::sptr addr = East::Address::LookupAnyIPAddress("0.0.0.0:8020");
    if(nullptr == addr){
        ELOG_ERROR(g_logger) << "get address error";
        return ;
    }

    auto http_server = std::make_shared<East::Http::HttpServer>(true);
    while(!http_server->bind(addr)) {
        ELOG_ERROR(g_logger) << "bind " << *addr <<" fail.";
        return ;
    }
    http_server->start();
}

int main(int argc, char** argv) {
    East::IOManager iom(10);
    iom.schedule(run);
    return 0;
}