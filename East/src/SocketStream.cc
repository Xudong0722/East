/*
 * @Author: Xudong0722 
 * @Date: 2025-06-10 21:24:55 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-10 21:57:47
 */

#include "SocketStream.h"

namespace East {

SocketStream::SocketStream(Socket::sptr sock, bool owner)
 : m_socket(sock)
 , m_owner(owner) {

}

SocketStream::~SocketStream() {
  if(m_owner && nullptr != m_socket) {
    m_socket->close();
  }
}

int SocketStream::read(void* buffer, size_t len) {
  if(!isConnected()) return -1;
  return m_socket->recv(buffer, len);
}

int SocketStream::read(ByteArray::sptr ba, size_t len) {
  if(!isConnected()) return -1;
  std::vector<iovec> iovs;
  ba->getWriteableBuffers(iovs, len);
  int rt = m_socket->recv(&iovs[0], iovs.size());
  if(rt > 0) {
    //改变偏移量，将这次读的数据累加上去
    ba->setOffset(ba->getOffset() + rt);
  }
  return rt;
}

int SocketStream::write(const void* buffer, size_t len) {
  if(!isConnected()) return -1;
  return m_socket->send(buffer, len);
}

int SocketStream::write(ByteArray::sptr ba, size_t len) {
  if(!isConnected()) return -1;
  std::vector<iovec> iovs;
  ba->getReadableBuffers(iovs, len);
  int rt = m_socket->send(&iovs[0], iovs.size()); 
  if(rt > 0) {
    //TODO, why?
    ba->setOffset(ba->getOffset() + rt);
  }
  return rt;
}

void SocketStream::close() {
  if(nullptr != m_socket) {
    m_socket->close();
  }
}

Socket::sptr SocketStream::getSocket() const {
  return m_socket;
}

bool SocketStream::isConnected() const {
  return m_socket && m_socket->isConnected();
}
} // namespace Eas 
