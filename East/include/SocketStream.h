/*
 * @Author: Xudong0722 
 * @Date: 2025-06-10 21:21:57 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-10 21:30:56
 */

#pragma once

#include "Stream.h"
#include "Socket.h"

namespace East {
    
class SocketStream: public Stream{
public:
  using sptr = std::shared_ptr<SocketStream>;
  SocketStream(Socket::sptr sock, bool owner = true);
  ~SocketStream();

  virtual int read(void* buffer, size_t len) override;
  virtual int read(ByteArray::sptr ba, size_t len) override;
  virtual int write(const void* buffer, size_t len) override;
  virtual int write(ByteArray::sptr ba, size_t len) override;
  virtual void close() override;

  Socket::sptr getSocket() const;
  bool isConnected() const;
private:
  Socket::sptr m_socket;
  bool m_owner{true};
};
} // namespace East
