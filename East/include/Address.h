/*
 * @Author: Xudong0722 
 * @Date: 2025-04-14 18:46:44 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-06 15:36:40
 */

#pragma once

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/un.h>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace East {

/**
 * @brief IP地址类的前向声明
 */
class IPAddress;

/**
 * @brief 网络地址基类
 *
 * Address类是所有网络地址类型的基类，提供了统一的地址操作接口。
 * 支持IPv4、IPv6、Unix域套接字等不同类型的地址。
 *
 * 主要功能：
 * - 地址创建和查找
 * - 地址比较和序列化
 * - 网络接口地址获取
 * - 支持多种地址族（AF_INET, AF_INET6, AF_UNIX等）
 */
class Address {
 public:
  using sptr = std::shared_ptr<Address>;

  Address() = default;
  virtual ~Address() = default;

  /**
   * @brief 根据sockaddr结构创建Address对象
   * @param addr 系统地址结构指针
   * @param addrlen 地址结构长度
   * @return 对应类型的Address智能指针
   */
  static sptr Create(const sockaddr* addr, socklen_t addrlen);

  /**
   * @brief 查找指定主机名的所有地址
   * @param result 输出参数，存储找到的地址列表
   * @param host 主机名或IP地址字符串
   * @param family 地址族，默认为AF_UNSPEC（自动检测）
   * @param type 套接字类型，默认为SOCK_STREAM
   * @param protocol 协议类型，默认为0（自动选择）
   * @return 查找成功返回true，失败返回false
   */
  static bool Lookup(std::vector<Address::sptr>& result,
                     const std::string& host, int family = AF_UNSPEC,
                     int type = SOCK_STREAM, int protocol = 0);

  /**
   * @brief 查找指定主机名的任意一个地址
   * @param host 主机名或IP地址字符串
   * @param family 地址族，默认为AF_UNSPEC（自动检测）
   * @param type 套接字类型，默认为SOCK_STREAM
   * @param protocol 协议类型，默认为0（自动选择）
   * @return 找到的地址智能指针，失败返回nullptr
   */
  static sptr LookupAny(const std::string& host, int family = AF_UNSPEC,
                        int type = SOCK_STREAM, int protocol = 0);

  /**
   * @brief 查找指定主机名的任意一个IP地址
   * @param host 主机名或IP地址字符串
   * @param family 地址族，默认为AF_UNSPEC（自动检测）
   * @param type 套接字类型，默认为SOCK_STREAM
   * @param protocol 协议类型，默认为0（自动选择）
   * @return 找到的IP地址智能指针，失败返回nullptr
   */
  static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string& host,
                                                       int family = AF_UNSPEC,
                                                       int type = SOCK_STREAM,
                                                       int protocol = 0);

  /**
   * @brief 获取所有网络接口的地址信息
   * @param result 输出参数，存储接口名和地址的映射
   * @param family 地址族，默认为AF_UNSPEC（所有类型）
   * @return 获取成功返回true，失败返回false
   */
  static bool GetInterfaceAddresses(
      std::multimap<std::string, std::pair<Address::sptr, uint32_t>>& result,
      int family = AF_UNSPEC);

  /**
   * @brief 获取指定网络接口的地址信息
   * @param result 输出参数，存储地址和前缀长度的列表
   * @param iface 接口名称，为空或"*"表示所有接口
   * @param family 地址族，默认为AF_UNSPEC（所有类型）
   * @return 获取成功返回true，失败返回false
   */
  static bool GetInterfaceAddresses(
      std::vector<std::pair<Address::sptr, uint32_t>>& result,
      const std::string& iface, int family = AF_UNSPEC);

  /**
   * @brief 获取地址族类型
   * @return 地址族常量（AF_INET, AF_INET6, AF_UNIX等）
   */
  int getFamily() const;

  /**
   * @brief 获取系统地址结构指针
   * @return sockaddr结构指针，用于系统调用
   */
  virtual const sockaddr* getAddr() const = 0;

  /**
   * @brief 获取地址结构长度
   * @return 地址结构的字节长度
   */
  virtual socklen_t getAddrLen() const = 0;

  /**
   * @brief 将地址信息输出到流中
   * @param os 输出流引用
   * @return 输出流引用
   */
  virtual std::ostream& dump(std::ostream& os) const = 0;

  /**
   * @brief 将地址转换为字符串
   * @return 地址的字符串表示
   */
  std::string toString() const;

  /**
   * @brief 地址比较操作符（小于）
   * @param rhs 右操作数
   * @return 当前地址小于右操作数返回true
   */
  bool operator<(const Address& rhs) const;

  /**
   * @brief 地址相等比较操作符
   * @param rhs 右操作数
   * @return 两个地址相等返回true
   */
  bool operator==(const Address& rhs) const;

  /**
   * @brief 地址不等比较操作符
   * @param rhs 右操作数
   * @return 两个地址不相等返回true
   */
  bool operator!=(const Address& rhs) const;
};

/**
 * @brief IP地址基类
 *
 * IPAddress类继承自Address，专门用于处理IP地址（IPv4和IPv6）。
 * 提供了IP地址特有的操作，如端口管理、子网计算等。
 */
class IPAddress : public Address {
 public:
  using sptr = std::shared_ptr<IPAddress>;

  /**
   * @brief 创建IP地址对象
   * @param address IP地址字符串，为nullptr时使用默认地址
   * @param port 端口号，默认为0
   * @return IP地址智能指针，失败返回nullptr
   */
  static sptr Create(const char* address = nullptr, uint16_t port = 0);

  /**
   * @brief 获取广播地址
   * @param prefix_len 子网前缀长度
   * @return 广播地址的智能指针
   */
  virtual IPAddress::sptr getBroadcastAddr(uint32_t prefix_len) const = 0;

  /**
   * @brief 获取网络地址
   * @param prefix_len 子网前缀长度
   * @return 网络地址的智能指针
   */
  virtual IPAddress::sptr getNetworkAddr(uint32_t prefix_len) const = 0;

  /**
   * @brief 获取子网掩码
   * @param prefix_len 子网前缀长度
   * @return 子网掩码的智能指针
   */
  virtual IPAddress::sptr getSubnetMask(uint32_t prefix_len) const = 0;

  /**
   * @brief 获取端口号
   * @return 端口号
   */
  virtual uint16_t getPort() const = 0;

  /**
   * @brief 设置端口号
   * @param port 新的端口号
   */
  virtual void setPort(uint16_t port) = 0;
};

/**
 * @brief IPv4地址类
 *
 * IPV4Address类继承自IPAddress，专门处理IPv4地址。
 * 支持IPv4地址的创建、解析、子网计算等操作。
 */
class IPV4Address : public IPAddress {
 public:
  using sptr = std::shared_ptr<IPV4Address>;

  /**
   * @brief 从sockaddr_in结构构造IPv4地址
   * @param addr sockaddr_in结构引用
   */
  IPV4Address(const sockaddr_in& addr);

  /**
   * @brief 从IP地址和端口构造IPv4地址
   * @param address IPv4地址（网络字节序），默认为INADDR_ANY
   * @param port 端口号，默认为0
   */
  IPV4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

  /**
   * @brief 创建IPv4地址对象
   * @param address IPv4地址字符串，为nullptr时使用默认地址
   * @param port 端口号，默认为0
   * @return IPv4地址智能指针，失败返回nullptr
   */
  static sptr Create(const char* address = nullptr, uint16_t port = 0);

  // Address接口实现
  const sockaddr* getAddr() const override;
  socklen_t getAddrLen() const override;
  std::ostream& dump(std::ostream& os) const override;

  // IPAddress接口实现
  IPAddress::sptr getBroadcastAddr(uint32_t prefix_len) const override;
  IPAddress::sptr getNetworkAddr(uint32_t prefix_len) const override;
  IPAddress::sptr getSubnetMask(uint32_t prefix_len) const override;
  uint16_t getPort() const override;
  void setPort(uint16_t port) override;

 private:
  sockaddr_in m_addr;  ///< IPv4地址结构
};

/**
 * @brief IPv6地址类
 *
 * IPV6Address类继承自IPAddress，专门处理IPv6地址。
 * 支持IPv6地址的创建、解析、子网计算等操作。
 */
class IPV6Address : public IPAddress {
 public:
  using sptr = std::shared_ptr<IPV6Address>;

  /**
   * @brief 默认构造函数，创建未初始化的IPv6地址
   */
  IPV6Address();

  /**
   * @brief 从sockaddr_in6结构构造IPv6地址
   * @param address sockaddr_in6结构引用
   */
  IPV6Address(const sockaddr_in6& address);

  /**
   * @brief 从IP地址字符串和端口构造IPv6地址
   * @param address IPv6地址字符串
   * @param port 端口号，默认为0
   */
  IPV6Address(const char* address, uint16_t port = 0);

  /**
   * @brief 创建IPv6地址对象
   * @param address IPv6地址字符串，为nullptr时使用默认地址
   * @param port 端口号，默认为0
   * @return IPv6地址智能指针，失败返回nullptr
   */
  static sptr Create(const char* address = nullptr, uint32_t port = 0);

  // Address接口实现
  const sockaddr* getAddr() const override;
  socklen_t getAddrLen() const override;
  std::ostream& dump(std::ostream& os) const override;

  // IPAddress接口实现
  IPAddress::sptr getBroadcastAddr(uint32_t prefix_len) const override;
  IPAddress::sptr getNetworkAddr(uint32_t prefix_len) const override;
  IPAddress::sptr getSubnetMask(uint32_t prefix_len) const override;
  uint16_t getPort() const override;
  void setPort(uint16_t port) override;

 private:
  sockaddr_in6 m_addr;  ///< IPv6地址结构
};

/**
 * @brief Unix域套接字地址类
 *
 * UnixAddress类继承自Address，专门处理Unix域套接字地址。
 * 支持文件路径和抽象套接字地址。
 */
class UnixAddress : public Address {
 public:
  using sptr = std::shared_ptr<UnixAddress>;

  /**
   * @brief 默认构造函数，创建抽象套接字地址
   */
  UnixAddress();

  /**
   * @brief 从文件路径构造Unix域套接字地址
   * @param path 文件路径
   */
  UnixAddress(const std::string& path);

  // Address接口实现
  const sockaddr* getAddr() const override;
  socklen_t getAddrLen() const override;

  /**
   * @brief 设置地址长度
   * @param len 新的地址长度
   */
  void setAddrLen(uint32_t len);

  std::ostream& dump(std::ostream& os) const override;

 private:
  sockaddr_un m_addr;  ///< Unix域套接字地址结构
  socklen_t m_length;  ///< 地址长度
};

/**
 * @brief 未知类型地址类
 *
 * UnknownAddress类继承自Address，用于处理不支持的地址类型。
 * 主要用于错误处理和调试。
 */
class UnknownAddress : public Address {
 public:
  using sptr = std::shared_ptr<UnknownAddress>;

  /**
   * @brief 从地址族构造未知地址
   * @param family 地址族常量
   */
  UnknownAddress(int family);

  /**
   * @brief 从sockaddr结构构造未知地址
   * @param addr sockaddr结构引用
   */
  UnknownAddress(const sockaddr& addr);

  // Address接口实现
  const sockaddr* getAddr() const override;
  socklen_t getAddrLen() const override;
  std::ostream& dump(std::ostream& os) const override;

 private:
  sockaddr m_addr;  ///< 通用地址结构
};

/**
 * @brief 地址输出流操作符
 * @param os 输出流引用
 * @param addr 地址对象引用
 * @return 输出流引用
 */
std::ostream& operator<<(std::ostream& os, const Address& addr);

}  // namespace East
