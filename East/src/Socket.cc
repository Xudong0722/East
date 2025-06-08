/*
 * @Author: Xudong0722 
 * @Date: 2025-04-24 22:55:05 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-05-11 18:44:47
 */

#include "Socket.h"
#include <netinet/tcp.h>
#include "Elog.h"
#include "FdManager.h"
#include "Hook.h"
#include "IOManager.h"
#include "Macro.h"

namespace East {

static East::Logger::sptr g_logger = ELOG_NAME("system");

Socket::sptr Socket::CreateTCP(East::Address::sptr addr) {
  return std::make_shared<Socket>(addr->getFamily(), TCP, 0);
}

Socket::sptr Socket::CreateUDP(East::Address::sptr addr) {
  return std::make_shared<Socket>(addr->getFamily(), UDP, 0);
}

Socket::sptr Socket::CreateTCPSocket() {
  return std::make_shared<Socket>(IPv4, TCP, 0);
}

Socket::sptr Socket::CreateUDPSocket() {
  return std::make_shared<Socket>(IPv4, UDP, 0);
}

Socket::sptr Socket::CreateTCPSocket6() {
  return std::make_shared<Socket>(IPv6, TCP, 0);
}

Socket::sptr Socket::CreateUDPSocket6() {
  return std::make_shared<Socket>(IPv6, UDP, 0);
}

Socket::sptr Socket::CreateUnixTCPSocket() {
  return std::make_shared<Socket>(UNIX, TCP, 0);
}

Socket::sptr Socket::CreateUnixUDPSocket() {
  return std::make_shared<Socket>(UNIX, UDP, 0);
}

Socket::Socket(int family, int type, int protocol)
    : m_sock(-1),
      m_family(family),
      m_type(type),
      m_protocol(protocol),
      m_is_connected(false) {}

Socket::~Socket() {
  close();
}

int64_t Socket::getSendTimeout() const {
  auto fd = FdMgr::GetInst()->getFd(m_sock);
  if (nullptr != fd) {
    return fd->getSendTimeout();
  }
  return -1;
}

void Socket::setSendTimeout(int64_t timeout) {
  struct timeval tv {
    int(timeout / 1000), int(timeout % 1000 * 1000)
  };
  return setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
}

int64_t Socket::getRecvTimeout() const {
  auto fd = FdMgr::GetInst()->getFd(m_sock);
  if (nullptr != fd) {
    return fd->getRecvTimeout();
  }
  return -1;
}

void Socket::setRecvTimeout(int64_t timeout) {
  struct timeval tv {
    int(timeout / 1000), int(timeout % 1000 * 1000)
  };
  return setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
}

bool Socket::getOption(int level, int option, void* result, size_t* len) {
  int rt = getsockopt(m_sock, level, option, result, (socklen_t*)len);
  if (rt) {
    ELOG_ERROR(g_logger) << "getOption(" << level << ", " << option << ", "
                         << result << ", " << len << ") err: " << errno;
  }
  return rt == 0 ? true : false;
}

bool Socket::setOption(int level, int option, const void* result, size_t len) {
  int rt = setsockopt(m_sock, level, option, result, len);
  if (rt) {
    ELOG_ERROR(g_logger) << "setOption(" << level << ", " << option << ", "
                         << result << ", " << len << ") err: " << errno;
  }
  return rt == 0 ? true : false;
}

Socket::sptr Socket::accept() {
  Socket::sptr sock = std::make_shared<Socket>(m_family, m_type, m_protocol);
  int fd = ::accept(m_sock, nullptr, nullptr);
  if (fd == -1) {
    ELOG_DEBUG(g_logger) << "accept(" << m_sock << ") err: " << errno;
    return nullptr;
  }
  if (sock->init(fd)) {
    return sock;
  }
  return nullptr;
}

bool Socket::init(int sock) {
  //目前只有accept会用到这个函数，用来初始化一些socket的参数
  auto fd = FdMgr::GetInst()->getFd(sock);
  ELOG_INFO(g_logger) << fd;
  if (nullptr != fd && fd->isSocket() && !fd->isClosed()) {
    m_sock = sock;
    m_is_connected = true;
    initSocket();
    getLocalAddr();
    getRemoteAddr();
    return true;
  }

  ELOG_ERROR(g_logger) << "init(" << sock << ") err: " << errno;
  return false;
}

bool Socket::bind(const Address::sptr addr) {
  if (!isValid()) {
    newSocket();
    if (EAST_UNLIKELY(!isValid())) {
      ELOG_ERROR(g_logger) << "bind(" << addr->toString() << ") err: " << errno;
      return false;
    }
  }

  if (EAST_UNLIKELY(addr->getFamily() != m_family)) {
    ELOG_ERROR(g_logger) << "bind(" << addr->toString() << ") err: " << errno;
    return false;
  }

  if (::bind(m_sock, addr->getAddr(), addr->getAddrLen())) {
    ELOG_ERROR(g_logger) << "bind(" << addr->toString() << ") err: " << errno;
    return false;
  }
  getLocalAddr();
  return true;
}

bool Socket::connect(const Address::sptr addr, uint64_t timeout_ms) {
  if (!isValid()) {
    newSocket();
    if (EAST_UNLIKELY(!isValid())) {
      ELOG_ERROR(g_logger) << "connect(" << addr->toString()
                           << ") err: " << errno;
      return false;
    }
  }

  if (EAST_UNLIKELY(addr->getFamily() != m_family)) {
    ELOG_ERROR(g_logger) << "connect(" << addr->toString()
                         << ") err: " << errno;
    return false;
  }

  if (timeout_ms == (uint64_t)-1) {
    if (::connect(m_sock, addr->getAddr(), addr->getAddrLen())) {
      ELOG_ERROR(g_logger) << "connect(" << addr->toString()
                           << ") err: " << errno;
      close();
      return false;
    }
  } else {
    if (::connect_with_timeout(m_sock, addr->getAddr(), addr->getAddrLen(),
                               timeout_ms)) {
      ELOG_ERROR(g_logger) << "connect(" << addr->toString()
                           << ") err: " << errno;
      close();
      return false;
    }
  }

  m_is_connected = true;
  getLocalAddr();
  getRemoteAddr();
  return true;
}

bool Socket::reconnect(uint64_t timeout_ms) {
  return true;
}

//backlog 未完成连接队列的最大长度
bool Socket::listen(int backlog) {
  if (!isValid()) {
    ELOG_ERROR(g_logger) << "listen error sock = -1";
    return false;
  }

  if (::listen(m_sock, backlog)) {
    ELOG_ERROR(g_logger) << "listen(" << m_sock << ", " << backlog
                         << ") err: " << errno;
    return false;
  }
  return true;
}

bool Socket::close() {
  if (!m_is_connected && m_sock == -1) {
    return true;
  }
  m_is_connected = false;
  if (m_sock != -1) {
    ::close(m_sock);
    m_sock = -1;
  }
  return false;
}

int Socket::send(const void* buffer, size_t length, int flags) {
  if (m_is_connected) {
    return ::send(m_sock, buffer, length, flags);
  }
  return -1;
}

int Socket::sendTo(const void* buffer, size_t length, const Address::sptr to,
                   int flags) {
  if (m_is_connected) {
    return ::sendto(m_sock, buffer, length, flags, to->getAddr(),
                    to->getAddrLen());
  }
  return -1;
}

int Socket::send(const iovec* buffers, size_t length, int flags) {
  if (m_is_connected) {
    msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = (iovec*)buffers;
    msg.msg_iovlen = length;
    return ::sendmsg(m_sock, &msg, flags);
  }
  return -1;
}

int Socket::sendTo(const iovec* buffers, size_t length, const Address::sptr to,
                   int flags) {
  if (m_is_connected) {
    msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = (iovec*)buffers;
    msg.msg_iovlen = length;
    msg.msg_name = (void*)(to->getAddr());
    msg.msg_namelen = to->getAddrLen();
    return ::sendmsg(m_sock, &msg, flags);
  }
  return -1;
}

int Socket::recv(void* buffer, size_t length, int flags) {
  if (m_is_connected) {
    return ::recv(m_sock, buffer, length, flags);
  }
  return -1;
}

int Socket::recv(iovec* buffers, size_t length, int flags) {
  if (m_is_connected) {
    msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = (iovec*)buffers;
    msg.msg_iovlen = length;
    return ::recvmsg(m_sock, &msg, flags);
  }
  return -1;
}

int Socket::recvFrom(void* buffer, size_t length, Address::sptr from,
                     int flags) {
  if (m_is_connected) {
    socklen_t len = from->getAddrLen();
    return ::recvfrom(m_sock, buffer, length, flags,
                      const_cast<sockaddr*>(from->getAddr()), &len);
  }
  return -1;
}

int Socket::recvFrom(iovec* buffers, size_t length, Address::sptr from,
                     int flags) {
  if (m_is_connected) {
    msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = (iovec*)buffers;
    msg.msg_iovlen = length;
    msg.msg_name = const_cast<sockaddr*>(from->getAddr());
    msg.msg_namelen = from->getAddrLen();
    return ::recvmsg(m_sock, &msg, flags);
  }
  return -1;
}

Address::sptr Socket::getLocalAddr() {
  if (nullptr != m_local_addr) {
    return m_local_addr;
  }
  Address::sptr addr;
  switch (m_family) {
    case AF_INET:
      addr = std::make_shared<IPV4Address>();
      break;
    case AF_INET6:
      addr = std::make_shared<IPV6Address>();
      break;
    case AF_UNIX:
      addr = std::make_shared<UnixAddress>();
      break;
    default:
      addr = std::make_shared<UnknownAddress>(m_family);
      break;
  }
  socklen_t len = addr->getAddrLen();
  if (getsockname(m_sock, const_cast<sockaddr*>(addr->getAddr()), &len)) {
    ELOG_ERROR(g_logger) << "getsockname(" << m_sock << ") err: " << errno;
    return Address::sptr(new UnknownAddress(m_family));
  }

  if (m_family == AF_UNIX) {
    UnixAddress::sptr unix_addr = std::dynamic_pointer_cast<UnixAddress>(addr);
    unix_addr->setAddrLen(len);
  }
  m_local_addr = addr;
  return m_local_addr;
}

Address::sptr Socket::getRemoteAddr() {
  if (nullptr != m_remote_addr) {
    return m_remote_addr;
  }
  Address::sptr addr;
  switch (m_family) {
    case AF_INET:
      addr = std::make_shared<IPV4Address>();
      break;
    case AF_INET6:
      addr = std::make_shared<IPV6Address>();
      break;
    case AF_UNIX:
      addr = std::make_shared<UnixAddress>();
      break;
    default:
      addr = std::make_shared<UnknownAddress>(m_family);
      break;
  }
  socklen_t len = addr->getAddrLen();
  if (getpeername(m_sock, const_cast<sockaddr*>(addr->getAddr()), &len)) {
    ELOG_ERROR(g_logger) << "getpeername(" << m_sock << ") err: " << errno;
    return Address::sptr(new UnknownAddress(m_family));
  }

  if (m_family == AF_UNIX) {
    UnixAddress::sptr unix_addr = std::dynamic_pointer_cast<UnixAddress>(addr);
    unix_addr->setAddrLen(len);
  }
  m_remote_addr = addr;
  return m_remote_addr;
}

int Socket::getFamily() const {
  return m_family;
}

int Socket::getType() const {
  return m_type;
}

int Socket::getProtocol() const {
  return m_protocol;
}

bool Socket::isConnected() const {
  return m_is_connected;
}

bool Socket::isValid() const {
  return m_sock != -1;
}

int Socket::getError() {
  int error = 0;
  size_t len = sizeof(int);
  if (!getOption(SOL_SOCKET, SO_ERROR, &error, &len)) {
    ELOG_ERROR(g_logger) << "getError(" << m_sock << ") err: " << errno;
    return -1;
  }
  return error;
}

std::ostream& Socket::dump(std::ostream& os) const {
  os << "[Socket: sock=" << m_sock << ", family=" << m_family
     << ", type=" << m_type << ", protocol=" << m_protocol
     << ", is_connected=" << m_is_connected
     << ", local_addr=" << (m_local_addr ? m_local_addr->toString() : "")
     << ", remote_addr=" << (m_remote_addr ? m_remote_addr->toString() : "");
  return os;
}

int Socket::getSocket() const {
  return m_sock;
}

bool Socket::cancelRead() {
  return IOManager::GetThis()->cancelEvent(m_sock, IOManager::READ);
}

bool Socket::cancelWrite() {
  return IOManager::GetThis()->cancelEvent(m_sock, IOManager::WRITE);
}

bool Socket::cancelAccept() {
  return IOManager::GetThis()->cancelEvent(m_sock, IOManager::READ);
}

bool Socket::cancelAll() {
  return IOManager::GetThis()->cancelAll(m_sock);
}

void Socket::initSocket() {
  //TODO
  int val = 1;
  setOption(SOL_SOCKET, SO_REUSEADDR, val);  //端口释放之后可以立即被复用
  if (m_type == SOCK_STREAM) {
    setOption(
        IPPROTO_TCP, TCP_NODELAY,
        val);  //禁用Nagle算法，该算法通过将小的数据包合并成更大的数据包来减少网络传输的次数，从而提高网络传输效率。
  }
}

void Socket:: newSocket() {
  m_sock = socket(m_family, m_type, m_protocol);
  if (EAST_LIKELY(m_sock != -1)) {
    initSocket();
  } else {
    ELOG_ERROR(g_logger) << "newSocket(" << m_family << ", " << m_type << ", "
                         << m_protocol << ") err: " << errno;
  }
}

std::ostream& operator<<(std::ostream& os, const Socket& sock) {
  return sock.dump(os);
}
}  // namespace East