/*
 * @Author: Xudong0722 
 * @Date: 2025-06-08 21:54:47 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-08 22:16:41
 */

#include "../East/include/TcpServer.h"
#include "../East/include/Elog.h"
#include "../East/include/ByteArray.h"
#include "../East/include/Address.h"

East::Logger::sptr g_logger = ELOG_ROOT();
int type = 1;

class EchoServer: public East::TcpServer {
public:
    EchoServer(int type);
    void handleClient(East::Socket::sptr client) override;
private:
    int m_type{0};
};

EchoServer::EchoServer(int type) : m_type(type) {}

void EchoServer::handleClient(East::Socket::sptr client) {
    ELOG_INFO(g_logger) << "handleClient: " << *client;
    East::ByteArray::sptr ba = std::make_shared<East::ByteArray>();
    while(true) {
        ba->clear();
        std::vector<iovec> iovs;
        ba->getWriteableBuffers(iovs, 1024);

        int rt = client->recv(&iovs[0], iovs.size());
        if(rt == 0) {
            ELOG_INFO(g_logger) << "client close." << *client;
            return ;
        }else if(rt < 0) {
            ELOG_ERROR(g_logger) << "client error rt: " << rt 
              << " errno: " << errno << " strerrno: " << strerror(errno);
            break;
        }

        ba->setOffset(0);
        if(m_type == 1) {
            std::cout << ba->toString() << std::endl;   //text format
        }else if(m_type == 2) {
            std::cout << ba->toHexString() << std::endl;  //binary format
        }
    }
}

void run() {
    auto addr = East::Address::LookupAny("0.0.0.0:8020");
    //auto addr2 = std::make_shared<East::UnixAddress>("/tmp/unix_addr");
    ELOG_INFO(g_logger) << *addr;// << " - " << *addr2;
    std::vector<East::Address::sptr> addrs{addr};
    EchoServer::sptr echo_server = std::make_shared<EchoServer>(type);

    std::vector<East::Address::sptr> fails;
    while(!echo_server->bind(addrs, fails)) {
        sleep(2);
    }

    echo_server->start();
}

int main(int argc, char** argv) {
    if(argc < 2) {
        ELOG_INFO(g_logger) << "used as[" << argv[0] << "-t] or [" << argv[0] << " -b]";
        return 0;
    }

    if(!strcmp(argv[1], "-b")) {
        type = 2;
    }
    East::IOManager iom(1);
    iom.schedule(run);
    return 0;
}