/*
 * @Author: Xudong0722 
 * @Date: 2025-04-14 18:46:44 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-04-14 21:36:26
 */

#pragma once

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/un.h>
#include <memory>
#include <sstream>
#include <string>

namespace East {
class Address {
 public:
  using sptr = std::shared_ptr<Address>;
  Address() = default;
  virtual ~Address() = default;

  int getFamily() const;

  virtual const sockaddr* getAddr() const = 0;
  virtual socklen_t getAddrLen() const = 0;

  virtual std::ostream& dump(std::ostream& os) const = 0;
  std::string toString();

  bool operator<(const Address& rhs) const;
  bool operator==(const Address& rhs) const;
  bool operator!=(const Address& rhs) const;
};

class IPAddress : public Address {
 public:
  using sptr = std::shared_ptr<IPAddress>;

  virtual IPAddress::sptr getBroadcastAddr() const = 0;
  virtual IPAddress::sptr getNetworkAddr() const = 0;
  virtual IPAddress::sptr getSubnetMask() const = 0;

  virtual uint32_t getPort() const = 0;
  virtual void setPort(uint32_t port) = 0;
};

class IPV4Address : public IPAddress {
 public:
  using sptr = std::shared_ptr<IPV4Address>;

  IPV4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

  //Address
  const sockaddr* getAddr() const override;
  socklen_t getAddrLen() const override;
  std::ostream& dump(std::ostream& os) const override;

  //IPAddress
  IPAddress::sptr getBroadcastAddr() const override;
  IPAddress::sptr getNetworkAddr() const override;
  IPAddress::sptr getSubnetMask() const override;
  uint32_t getPort() const override;
  void setPort(uint32_t port) override;

 private:
  sockaddr_in m_addr;
};

class IPV6Address : public IPAddress {
 public:
  using sptr = std::shared_ptr<IPV6Address>;

  IPV6Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

  //Address
  const sockaddr* getAddr() const override;
  socklen_t getAddrLen() const override;
  std::ostream& dump(std::ostream& os) const override;

  //IPAddress
  IPAddress::sptr getBroadcastAddr() const override;
  IPAddress::sptr getNetworkAddr() const override;
  IPAddress::sptr getSubnetMask() const override;
  uint32_t getPort() const override;
  void setPort(uint32_t port) override;

 private:
  sockaddr_in6 m_addr;
};

class UnixAddress : public Address {
 public:
  using sptr = std::shared_ptr<UnixAddress>;

  UnixAddress(const std::string& path);

  //Address
  const sockaddr* getAddr() const override;
  socklen_t getAddrLen() const override;
  std::ostream& dump(std::ostream& os) const override;

 private:
  sockaddr_un m_addr;
  socklen_t m_length;
};

class UnknownAddress : public Address {
 public:
  using sptr = std::shared_ptr<UnknownAddress>;

  //Address
  const sockaddr* getAddr() const override;
  socklen_t getAddrLen() const override;
  std::ostream& dump(std::ostream& os) const override;

 private:
  sockaddr m_addr;
};
}  // namespace East
