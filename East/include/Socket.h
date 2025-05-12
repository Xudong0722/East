/*
 * @Author: Xudong0722 
 * @Date: 2025-04-24 22:55:09 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-05-12 22:42:05
 */

#pragma once
#include <memory>
#include "Address.h"
#include "Noncopyable.h"

namespace East {
class Socket : public std::enable_shared_from_this<Socket>, noncopyable {
 public:
  using sptr = std::shared_ptr<Socket>;
  using wptr = std::weak_ptr<Socket>;

  enum FAMILY {
    IPv4 = AF_INET,
    IPv6 = AF_INET6,
    UNIX = AF_UNIX,
  };

  enum TYPE {
    TCP = SOCK_STREAM,
    UDP = SOCK_DGRAM,
    RAW = SOCK_RAW,
  };

  static Socket::sptr CreateTCP(East::Address::sptr addr);
  static Socket::sptr CreateUDP(East::Address::sptr addr);
  static Socket::sptr CreateTCPSocket();
  static Socket::sptr CreateUDPSocket();
  static Socket::sptr CreateTCPSocket6();
  static Socket::sptr CreateUDPSocket6();
  static Socket::sptr CreateUnixTCPSocket();
  static Socket::sptr CreateUnixUDPSocket();

  Socket(int family, int type, int protocol);
  ~Socket();

  int64_t getSendTimeout() const;
  void setSendTimeout(int64_t timeout);

  int64_t getRecvTimeout() const;
  void setRecvTimeout(int64_t timeout);

  bool getOption(int level, int option, void* result, size_t* len);
  template <class T>
  bool getOption(int level, int option, T& result) {
    size_t len = sizeof(T);
    return getOption(level, option, &result, &len);
  }

  bool setOption(int level, int option, const void* result, size_t len);
  template <class T>
  void setOption(int level, int option, const T& val) {
    setOption(level, option, &val, sizeof(T));
  }

  virtual Socket::sptr accept();

  bool init(int sock);

  bool bind(const Address::sptr addr);

  bool connect(const Address::sptr addr, uint64_t timeout_ms = -1);

  bool reconnect(uint64_t timeout_ms = -1);

  //backlog 未完成连接队列的最大长度
  bool listen(int backlog = SOMAXCONN);

  bool close();

  /// @brief 发送数据
  /// @param buffer 待发送数据
  /// @param length 待发送数据的长度
  /// @param flags 标志字
  /// @return
  ///    retval > 0 发送成功对应大小的数据
  ///    retval = 0 socket关闭
  ///    retval < 0 socket出错
  int send(const void* buffer, size_t length, int flags = 0);

  /// @brief 发送数据
  /// @param buffer 待发送数据
  /// @param length 待发送数据的长度
  /// @param to 目标地址
  /// @param flags 标志字
  /// @return
  ///    retval > 0 发送成功对应大小的数据
  ///    retval = 0 socket关闭
  ///    retval < 0 socket出错
  int sendTo(const void* buffer, size_t length, const Address::sptr to,
             int flags = 0);

  int send(const iovec* buffers, size_t length, int flags = 0);
  int sendTo(const iovec* buffers, size_t length, const Address::sptr to,
             int flags = 0);

  int recv(void* buffer, size_t length, int flags = 0);
  int recv(iovec* buffers, size_t length, int flags = 0);
  int recvFrom(void* buffer, size_t length, Address::sptr from, int flags = 0);
  int recvFrom(iovec* buffers, size_t length, Address::sptr from,
               int flags = 0);

  Address::sptr getLocalAddr();
  Address::sptr getRemoteAddr();

  int getFamily() const;
  int getType() const;
  int getProtocol() const;

  bool isConnected() const;
  bool isValid() const;
  int getError();

  std::ostream& dump(std::ostream& os) const;
  int getSocket() const;

  bool cancelRead();
  bool cancelWrite();
  bool cancelAccept();
  bool cancelAll();

 private:
  void initSocket();
  void newSocket();

 private:
  int m_sock{0};
  int m_family{0};
  int m_type{0};
  int m_protocol{0};
  bool m_is_connected{false};
  Address::sptr m_local_addr{nullptr};
  Address::sptr m_remote_addr{nullptr};
};

}  // namespace East
