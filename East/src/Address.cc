/*
 * @Author: Xudong0722 
 * @Date: 2025-04-14 18:46:54 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-04-14 23:18:17
 */

#include <string.h>
#include "Address.h"
#include "Endian.h"

namespace East {

int Address::getFamily() const {
  return getAddr()->sa_family;
}

std::string Address::toString() {
  std::stringstream ss;
  dump(ss);
  return ss.str();
}

bool Address::operator<(const Address& rhs) const {
  socklen_t min_len = std::min(getAddrLen(), rhs.getAddrLen());
  int res = memcmp(getAddr(), rhs.getAddr(), min_len);
  if (res < 0) {
    return true;
  } else if (res > 0) {
    return false;
  } else if (getAddrLen() < rhs.getAddrLen()) {
    return true;
  }
  return false;
}

bool Address::operator==(const Address& rhs) const {
  return getAddrLen() == rhs.getAddrLen() &&
         memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
}

bool Address::operator!=(const Address& rhs) const {
  return !(*this == rhs);
}

//IPV4
IPV4Address::IPV4Address(uint32_t address, uint16_t port) {
  memset(&m_addr, 0, sizeof(m_addr));
  m_addr.sin_family = AF_INET;
  m_addr.sin_port = byteswapOnLittleEndian(port);   //转成大端字节序
  m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address);  //转成大端字节序
}

const sockaddr* IPV4Address::getAddr() const {
    return (sockaddr*)&m_addr;
}

socklen_t IPV4Address::getAddrLen() const {
    return sizeof(m_addr);
}

std::ostream& IPV4Address::dump(std::ostream& os) const {
    uint32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
    os << ((addr >> 24) & 0xff) << "."
       << ((addr >> 16) & 0xff) << "."
       << ((addr >> 8) & 0xff) << "."
       << (addr & 0xff) << ":" << byteswapOnLittleEndian(m_addr.sin_port);
    return os; 
}

IPAddress::sptr IPV4Address::getBroadcastAddr() const {}

IPAddress::sptr IPV4Address::getNetworkAddr() const {}

IPAddress::sptr IPV4Address::getSubnetMask() const {}

uint32_t IPV4Address::getPort() const {}

void IPV4Address::setPort(uint32_t port) {}

//IPV6

IPV6Address::IPV6Address(uint32_t address, uint16_t port) {}

const sockaddr* IPV6Address::getAddr() const {}

socklen_t IPV6Address::getAddrLen() const {}

std::ostream& IPV6Address::dump(std::ostream& os) const {
    return os;
}

IPAddress::sptr IPV6Address::getBroadcastAddr() const {}

IPAddress::sptr IPV6Address::getNetworkAddr() const {}

IPAddress::sptr IPV6Address::getSubnetMask() const {}

uint32_t IPV6Address::getPort() const {}

void IPV6Address::setPort(uint32_t port) {}

//UnixAddress
UnixAddress::UnixAddress(const std::string& path) {}

//Address
const sockaddr* UnixAddress::getAddr() const {}

socklen_t UnixAddress::getAddrLen() const {}

std::ostream& UnixAddress::dump(std::ostream& os) const {
    return os;
}

//UnknownAddress

const sockaddr* UnknownAddress::getAddr() const {}

socklen_t UnknownAddress::getAddrLen() const {}

std::ostream& UnknownAddress::dump(std::ostream& os) const {
    return os;
}
}  // namespace East
