/*
 * @Author: Xudong0722 
 * @Date: 2025-05-29 21:58:07 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-05-30 00:31:38
 */

#include "TcpServer.h"
#include "Config.h"
#include "Elog.h"

namespace East {

static East::ConfigVar<uint64_t>::sptr g_tcp_server_read_timeout =
    East::Config::Lookup("tcp_server.read_timeout", uint64_t(60 * 1000 * 2),
                         "tcp server read timeout");

static East::Logger::sptr g_logger = ELOG_NAME("system");

TcpServer::TcpServer(East::IOManager* worker, East::IOManager* accept_worker)
    : m_worker(worker),
      m_acceptWorker(accept_worker),
      m_readTimeout(g_tcp_server_read_timeout->getValue()),
      m_name("East/1.0.0"),
      m_isStop(true) {}

TcpServer::~TcpServer() {
  for (auto& i : m_socks) {
    i->close();
  }
  m_socks.clear();
}

bool TcpServer::bind(East::Address::sptr addr) {
  std::vector<Address::sptr> addrs{addr};
  std::vector<Address::sptr> fails;
  return bind(addrs, fails);
}

bool TcpServer::bind(const std::vector<Address::sptr>& addrs,
                     std::vector<Address::sptr>& fails) {
  for (auto& addr : addrs) {
    Socket::sptr sock = Socket::CreateTCP(addr);
    if (!sock->bind(addr)) {
      ELOG_ERROR(g_logger) << "bind fail errno: " << errno
                           << " strerror: " << strerror(errno) << " addr:[ "
                           << addr->toString() << "]";
      fails.emplace_back(addr);
      continue;
    }
    if (!sock->listen()) {
      ELOG_ERROR(g_logger) << "listen fail errno: " << errno
                           << " strerror: " << strerror(errno) << " addr:[ "
                           << addr->toString() << "]";
      fails.emplace_back(addr);
      continue;
    }
    m_socks.emplace_back(sock);
  }

  if (!fails.empty()) {
    m_socks.clear();
    return false;
  }

  for (auto& i : m_socks) {
    ELOG_INFO(g_logger) << "Server bind success: " << i;
  }
  return true;
}

bool TcpServer::start() {
  if (!m_isStop) {
    return true;
  }
  m_isStop = false;
  for (auto& sock : m_socks) {
    m_acceptWorker->schedule(
        std::bind(&TcpServer::startAccept, shared_from_this(), sock));
  }
  return true;
}

void TcpServer::stop() {
  m_isStop = true;
  auto self = shared_from_this();  //增加一个引用计数
  m_acceptWorker->schedule([this, self]() {
    for (auto& sock : m_socks) {
      sock->cancelAll();
      sock->close();
    }
    m_socks.clear();
  });
}

void TcpServer::handleClient(Socket::sptr sock) {
  ELOG_INFO(g_logger) << "handleClient: " << sock;
}

void TcpServer::startAccept(Socket::sptr sock) {
  while (!m_isStop) {
    Socket::sptr client = sock->accept();
    if (client) {
      client->setRecvTimeout(m_readTimeout); 
      m_worker->schedule(
          std::bind(&TcpServer::handleClient, shared_from_this(), client));
    } else {
      ELOG_ERROR(g_logger) << "Accept error: " << errno
                           << " strerrno: " << strerror(errno);
    }
  }
}
}  //namespace East