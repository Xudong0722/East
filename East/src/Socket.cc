/*
 * @Author: Xudong0722 
 * @Date: 2025-04-24 22:55:05 
 * @Last Modified by:   Xudong0722 
 * @Last Modified time: 2025-04-24 22:55:05 
 */

#include <netinet/tcp.h>
#include "Socket.h"
#include "FdManager.h"
#include "Elog.h"
#include "Macro.h"
#include "Hook.h"

namespace East {

    static East::Logger::sptr g_logger = ELOG_NAME("system");

    Socket::Socket(int family, int type, int protocol) 
      : m_sock(-1)
      , m_family(family)
      , m_type(type)
      , m_protocol(protocol)
      , m_is_connected(false){

    }

    Socket::~Socket() {
      close();
    }
  
    int64_t Socket::getSendTimeout() const {
      auto fd = FdMgr::GetInst()->getFd(m_sock);
      if(nullptr != fd){
        return fd->getSendTimeout();
      }
      return -1;
    }

    void Socket::setSendTimeout(int64_t timeout) {
      struct timeval tv{int(timeout / 1000), int(timeout % 1000 * 1000)};
      return setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
    }
  
    int64_t Socket::getRecvTimeout() const {
      auto fd = FdMgr::GetInst()->getFd(m_sock);
      if(nullptr != fd) {
        return fd->getRecvTimeout();
      }
      return -1;
    }

    void Socket::setRecvTimeout(int64_t timeout) {
      struct timeval tv{int(timeout / 1000), int(timeout % 1000 * 1000)};
      return setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
    }
  
    bool Socket::getOption(int level, int option, void* result, size_t* len){
      int rt = getsockopt(m_sock, level, option, result, (socklen_t*)len);
      if(rt) {
        ELOG_ERROR(g_logger) << "getOption(" << level << ", " << option
                             << ", " << result << ", " << len
                             << ") err: " << errno;
      }
      return rt == 0 ? true : false;
    }
    
    bool Socket::setOption(int level, int option, const void* result, size_t len) {
      int rt = setsockopt(m_sock, level, option, result, len);
      if(rt) {
        ELOG_ERROR(g_logger) << "setOption(" << level << ", " << option
                             << ", " << result << ", " << len
                             << ") err: " << errno;
      }
      return rt == 0 ? true : false;
    }

  
    Socket::sptr Socket::accept() {
      Socket::sptr sock = std::make_shared<Socket>(m_family, m_type, m_protocol);
      int fd = ::accept(m_sock, nullptr, nullptr);
      if(fd == -1) {
        ELOG_ERROR(g_logger) << "accept(" << m_sock << ") err: " << errno;
        return nullptr;
      }
      if(sock->init(fd)) {
        return sock;
      }
      return nullptr;
    }
    
    bool Socket::init(int sock) {
      //目前只有accept会用到这个函数，用来初始化一些socket的参数
      auto fd = FdMgr::GetInst()->getFd(sock);
      if(nullptr != fd && fd->isSocket() && !fd->isClosed()) {
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
      if(!isValid()) {
        newSocket();
        if(EAST_UNLIKELY(!isValid())) {
          ELOG_ERROR(g_logger) << "bind(" << addr->toString() << ") err: " << errno;
          return false;
        }
      }

      if(EAST_UNLIKELY(addr->getFamily() != m_family)) {
        ELOG_ERROR(g_logger) << "bind(" << addr->toString() << ") err: " << errno;
        return false;   
      }

      if(::bind(m_sock, addr->getAddr(), addr->getAddrLen())) {
        ELOG_ERROR(g_logger) << "bind(" << addr->toString() << ") err: " << errno;
        return false;
      }
      getLocalAddr();
      return true;
    }
  
    bool Socket::connect(const Address::sptr addr, uint64_t timeout_ms) {
        if(!isValid()) {
            newSocket();
            if(EAST_UNLIKELY(!isValid())) {
                ELOG_ERROR(g_logger) << "connect(" << addr->toString() << ") err: " << errno;
                return false;
            }
        }

      if(EAST_UNLIKELY(addr->getFamily() != m_family)) {
        ELOG_ERROR(g_logger) << "connect(" << addr->toString() << ") err: " << errno;
        return false;   
      }
      
      if(timeout_ms == (uint64_t)-1) {
        if(::connect(m_sock, addr->getAddr(), addr->getAddrLen())) {
          ELOG_ERROR(g_logger) << "connect(" << addr->toString() << ") err: " << errno;
          close();
          return false;
        }
      }else {
        if(::connect_with_timeout(m_sock, addr->getAddr(), addr->getAddrLen(), timeout_ms)) {
          ELOG_ERROR(g_logger) << "connect(" << addr->toString() << ") err: " << errno;
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
        return true;
    }
  
    bool Socket::close() {
        return true;
    }
  
    int Socket::send(const void* buffer, size_t length, int flags) {
        return 0;
    }
  
    int Socket::sendTo(const void* buffer, size_t length, const Address::sptr to, int flags) {
        return 0;
    }
  
    int Socket::send(const iovec* buffers, size_t length, int flags) {
        return 0;
    }

    int Socket::sendTo(const iovec* buffers, size_t length, const Address::sptr to, int flags) {
        return 0;
    }
  
    int Socket::recv(void* buffer, size_t length, int flags) {
        return 0;
    }

    int Socket::recv(iovec* buffers, size_t length, int flags) {
        return 0;
    }

    int Socket::recvFrom(void* buffer, size_t length, Address::sptr from, int flags) {
        return 0;
    }

    int Socket::recvFrom(iovec* buffers, size_t length, Address::sptr from, int flags) {
        return 0;
    }
  
    Address::sptr Socket::getLocalAddr() const {
      return m_local_addr;
    }

    Address::sptr Socket::getRemoteAddr() const {
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
        return 0;
    }

    int Socket::getError() {
        return 0;
    }
  
    std::ostream& Socket::dump(std::ostream& os) const {
        return os;
    }

    int Socket::getSocket() const {
      return m_sock;
    }
  
    bool Socket::cancelRead() {
        return 0;
    }

    bool Socket::cancelWrite() {
        return 0;
    }

    bool Socket::cancelAccept() {
        return 0;
    }

    bool Socket::cancelAll() {
        return 0;
    }

    void Socket::initSocket() {
      //TODO
      int val = 1;
      setOption(SOL_SOCKET, SO_REUSEADDR, val);  //端口释放之后可以立即被复用
      if(m_type == SOCK_STREAM) {
        setOption(IPPROTO_TCP, TCP_NODELAY, val);  //禁用Nagle算法，该算法通过将小的数据包合并成更大的数据包来减少网络传输的次数，从而提高网络传输效率。
      }
    }

    void Socket::newSocket() {
      m_sock = socket(m_family, m_type, m_protocol);
      if(EAST_LIKELY(m_sock != -1)) {
        initSocket();
      }else{
        ELOG_ERROR(g_logger) << "newSocket(" << m_family << ", " << m_type
                             << ", " << m_protocol << ") err: " << errno;
      }
    }
}