/*
 * @Author: Xudong0722 
 * @Date: 2025-04-24 22:55:09 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-04-25 00:03:38
 */

#include <memory>
#include "Address.h"
#include "Noncopyable.h"

namespace East {
class Socket: public std::enable_shared_from_this<Socket>, noncopyable{
public:
  using sptr = std::shared_ptr<Socket>;
  using wptr = std::weak_ptr<Socket>;

  Socket(int family, int type, int protocol);
  ~Socket();

  int64_t getSendTimeout() const;
  void setSendTimeout(int64_t timeout);

  int64_t getRecvTimeout() const;
  void setRecvTimeout(int64_t timeout);

  bool getOption(int level, int option, void* result, size_t* len);
  template<class T>
  bool getOption(int level, int option, T& result) {
    size_t len = sizeof(T);
    return getOption(level, option, &result, &len);
  }
  
  bool setOption(int level, int option, const void* result, size_t len);
  template<class T>
  void setOption(int level, int option, const T& val) {
    setOption(level, option, &val, sizeof(T));
  }

  virtual Socket::sptr accept();

  virtual bool bind(const Address::sptr addr);

  virtual bool connect(const Address::sptr addr, uint64_t timeout_ms = -1);

  virtual bool reconnect(uint64_t timeout_ms = -1);

  //backlog 未完成连接队列的最大长度
  virtual bool listen(int backlog = SOMAXCONN);

  virtual bool close();

  /// @brief 发送数据
  /// @param buffer 待发送数据
  /// @param length 待发送数据的长度
  /// @param flags 标志字
  /// @return 
  ///    retval > 0 发送成功对应大小的数据
  ///    retval = 0 socket关闭
  ///    retval < 0 socket出错
  virtual int send(const void* buffer, size_t length, int flags = 0);

  /// @brief 发送数据
  /// @param buffer 待发送数据
  /// @param length 待发送数据的长度
  /// @param to 目标地址
  /// @param flags 标志字
  /// @return 
  ///    retval > 0 发送成功对应大小的数据
  ///    retval = 0 socket关闭
  ///    retval < 0 socket出错
  virtual int sendTo(const void* buffer, size_t length, const Address::sptr to, int flags = 0);


  virtual int sendTo(const iovec* buffers, size_t length, const Address::sptr to, int flags = 0);
private:
};

} // namespace East
