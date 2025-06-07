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
class IPAddress;
class Address {
 public:
  using sptr = std::shared_ptr<Address>;
  Address() = default;
  virtual ~Address() = default;

  static sptr Create(const sockaddr* addr, socklen_t addrlen);

  static bool Lookup(std::vector<Address::sptr>& result,
                     const std::string& host, int family = AF_UNSPEC,
                     int type = SOCK_STREAM, int protocol = 0);
  static sptr LookupAny(const std::string& host, int family = AF_UNSPEC,
                        int type = SOCK_STREAM, int protocol = 0);
  static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string& host,
                                                       int family = AF_UNSPEC,
                                                       int type = SOCK_STREAM,
                                                       int protocol = 0);

  static bool GetInterfaceAddresses(
      std::multimap<std::string, std::pair<Address::sptr, uint32_t>>& result,
      int family = AF_UNSPEC);
  static bool GetInterfaceAddresses(
      std::vector<std::pair<Address::sptr, uint32_t>>& result,
      const std::string& iface, int family = AF_UNSPEC);

  int getFamily() const;

  virtual const sockaddr* getAddr() const = 0;
  virtual socklen_t getAddrLen() const = 0;

  virtual std::ostream& dump(std::ostream& os) const = 0;
  std::string toString() const;

  bool operator<(const Address& rhs) const;
  bool operator==(const Address& rhs) const;
  bool operator!=(const Address& rhs) const;
};

class IPAddress : public Address {
 public:
  using sptr = std::shared_ptr<IPAddress>;

  static sptr Create(const char* address = nullptr, uint16_t port = 0);
  virtual IPAddress::sptr getBroadcastAddr(uint32_t prefix_len) const = 0;
  virtual IPAddress::sptr getNetworkAddr(uint32_t prefix_len) const = 0;
  virtual IPAddress::sptr getSubnetMask(uint32_t prefix_len) const = 0;

  virtual uint16_t getPort() const = 0;
  virtual void setPort(uint16_t port) = 0;
};

class IPV4Address : public IPAddress {
 public:
  using sptr = std::shared_ptr<IPV4Address>;

  IPV4Address(const sockaddr_in& addr);
  IPV4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

  static sptr Create(const char* address = nullptr, uint16_t port = 0);
  //Address
  const sockaddr* getAddr() const override;
  socklen_t getAddrLen() const override;
  std::ostream& dump(std::ostream& os) const override;

  //IPAddress
  IPAddress::sptr getBroadcastAddr(uint32_t prefix_len) const override;
  IPAddress::sptr getNetworkAddr(uint32_t prefix_len) const override;
  IPAddress::sptr getSubnetMask(uint32_t prefix_len) const override;
  uint16_t getPort() const override;
  void setPort(uint16_t port) override;

 private:
  sockaddr_in m_addr;
};

class IPV6Address : public IPAddress {
 public:
  using sptr = std::shared_ptr<IPV6Address>;

  IPV6Address();
  IPV6Address(const sockaddr_in6& address);
  IPV6Address(const char* address, uint16_t port = 0);

  static sptr Create(const char* address = nullptr, uint32_t port = 0);
  //Address
  const sockaddr* getAddr() const override;
  socklen_t getAddrLen() const override;
  std::ostream& dump(std::ostream& os) const override;

  //IPAddress
  IPAddress::sptr getBroadcastAddr(uint32_t prefix_len) const override;
  IPAddress::sptr getNetworkAddr(uint32_t prefix_len) const override;
  IPAddress::sptr getSubnetMask(uint32_t prefix_len) const override;
  uint16_t getPort() const override;
  void setPort(uint16_t port) override;

 private:
  sockaddr_in6 m_addr;
};

class UnixAddress : public Address {
 public:
  using sptr = std::shared_ptr<UnixAddress>;

  UnixAddress();
  UnixAddress(const std::string& path);

  //Address
  const sockaddr* getAddr() const override;
  socklen_t getAddrLen() const override;
  void setAddrLen(uint32_t len);
  std::ostream& dump(std::ostream& os) const override;

 private:
  sockaddr_un m_addr;
  socklen_t m_length;
};

class UnknownAddress : public Address {
 public:
  using sptr = std::shared_ptr<UnknownAddress>;
  UnknownAddress(int family);
  UnknownAddress(const sockaddr& addr);
  //Address
  const sockaddr* getAddr() const override;
  socklen_t getAddrLen() const override;
  std::ostream& dump(std::ostream& os) const override;

 private:
  sockaddr m_addr;
};

std::ostream& operator<<(std::ostream& os, const Address& addr);
}  // namespace East
