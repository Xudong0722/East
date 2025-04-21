/*
 * @Author: Xudong0722 
 * @Date: 2025-04-14 18:46:54 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-04-14 23:18:17
 */

#include "Address.h"
#include <string.h>
#include "Elog.h"
#include "Endian.h"

namespace East {

East::Logger::sptr g_logger = ELOG_NAME("system");

template <class T>
static T CreateMask(uint32_t bits) {
  return (1 << (sizeof(T) * 8 - bits)) - 1;  //TODO
}

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
IPV4Address::IPV4Address(const sockaddr_in& addr) {
  m_addr = addr;
}

IPV4Address::IPV4Address(uint32_t address, uint16_t port) {
  memset(&m_addr, 0, sizeof(m_addr));
  m_addr.sin_family = AF_INET;
  m_addr.sin_port = byteswapOnLittleEndian(port);            //转成大端字节序
  m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address);  //转成大端字节序
}

IPV4Address::sptr IPV4Address::Create(const char* address, uint32_t port) {
  if (nullptr == address)
    return nullptr;
  IPV4Address::sptr addr = std::make_shared<IPV4Address>();
  int res = inet_pton(AF_INET, address, &addr->m_addr.sin_addr);
  if (res != 1) {
    ELOG_ERROR(g_logger) << "IPV4Address::Create(" << address << ", port"
                         << port << ") res = " << res << ", errno = " << errno
                         << ", err info: " << strerror(errno);
    return nullptr;
  }
  return addr;
}

const sockaddr* IPV4Address::getAddr() const {
  return (sockaddr*)&m_addr;
}

socklen_t IPV4Address::getAddrLen() const {
  return sizeof(m_addr);
}

std::ostream& IPV4Address::dump(std::ostream& os) const {
  uint32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
  os << ((addr >> 24) & 0xff) << "." << ((addr >> 16) & 0xff) << "."
     << ((addr >> 8) & 0xff) << "." << (addr & 0xff) << ":"
     << byteswapOnLittleEndian(m_addr.sin_port);
  return os;
}

//	192.168.1.255， 这是一个广播地址，发送给192.168.1.0这个子网上的所有主机
IPAddress::sptr IPV4Address::getBroadcastAddr(uint32_t prefix_len) const {
  //发送给某个某段上所有的主机
  if (prefix_len > 32) {
    return nullptr;
  }
  sockaddr_in broadcast_addr(m_addr);
  broadcast_addr.sin_addr.s_addr |= byteswapOnLittleEndian(
      CreateMask<uint32_t>(prefix_len));  //广播地址 = 网络地址 | 掩码
  return std::make_shared<IPV4Address>(broadcast_addr);
}

//	192.168.1.0这就是一个子网地址，对应的广播地址是192.168.1.255
IPAddress::sptr IPV4Address::getNetworkAddr(uint32_t prefix_len) const {
  if (prefix_len > 32) {
    return nullptr;
  }
  sockaddr_in network_addr(m_addr);
  network_addr.sin_addr.s_addr &=
      byteswapOnLittleEndian(~CreateMask<uint32_t>(prefix_len));
  return std::make_shared<IPV4Address>(network_addr);
}

IPAddress::sptr IPV4Address::getSubnetMask(uint32_t prefix_len) const {
  sockaddr_in subnet_mask;
  memset(&subnet_mask, 0, sizeof(subnet_mask));
  subnet_mask.sin_family = AF_INET;
  subnet_mask.sin_addr.s_addr =
      ~byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
  return std::make_shared<IPV4Address>(subnet_mask);
}

uint32_t IPV4Address::getPort() const {
  return byteswapOnLittleEndian(m_addr.sin_port);
}

void IPV4Address::setPort(uint32_t port) {
  m_addr.sin_port = byteswapOnLittleEndian(port);
}

//IPV6
IPV6Address::IPV6Address() {
  memset(&m_addr, 0, sizeof(m_addr));
}

IPV6Address::IPV6Address(const sockaddr_in6& address) {
  m_addr = address;
}

IPV6Address::IPV6Address(const char* address, uint16_t port) {
  memset(&m_addr, 0, sizeof(m_addr));
  m_addr.sin6_family = AF_INET6;
  m_addr.sin6_port = byteswapOnLittleEndian(port);  //转成大端字节序
  memcpy(&m_addr.sin6_addr.s6_addr, address, 16);
}

IPV6Address::sptr IPV6Address::Create(const char* address, uint32_t port) {
  if (nullptr == address)
    return nullptr;
  IPV6Address::sptr addr = std::make_shared<IPV6Address>();
  int res = inet_pton(AF_INET, address, &addr->m_addr.sin6_addr);
  if (res != 1) {
    ELOG_ERROR(g_logger) << "IPV6Address::Create(" << address << ", port"
                         << port << ") res = " << res << ", errno = " << errno
                         << ", err info: " << strerror(errno);
    return nullptr;
  }
  return addr;
}

const sockaddr* IPV6Address::getAddr() const {
  return (sockaddr*)&m_addr;
}

socklen_t IPV6Address::getAddrLen() const {
  return sizeof(m_addr);
}

std::ostream& IPV6Address::dump(std::ostream& os) const {
  os << "[";
  uint16_t* addr =
      (uint16_t*)&m_addr.sin6_addr.s6_addr;  //uint8_t[16] -> 8个uint16_t
  bool used_zeros{false};
  for (int i = 0; i < 8; ++i) {
    if (addr[i] == 0 && !used_zeros) {
      continue;
    }
    if (i && addr[i - 1] == 0 && !used_zeros) {
      os << ":";
      used_zeros = true;
    }
    if (i) {
      os << ":";
    }
    os << std::hex << (int)byteswapOnLittleEndian(addr[i]) << std::dec;
  }

  if (!used_zeros && addr[7] == 0) {
    os << "::";
  }
  os << "]:" << byteswapOnLittleEndian(m_addr.sin6_port);
  return os;
}

IPAddress::sptr IPV6Address::getBroadcastAddr(uint32_t prefix_len) const {
  //TODO
  sockaddr_in6 broadcast_addr(m_addr);
  broadcast_addr.sin6_addr.s6_addr[prefix_len / 8] |=
      CreateMask<uint8_t>(prefix_len % 8);
  for (int i = prefix_len / 8 + 1; i < 16; ++i) {
    broadcast_addr.sin6_addr.s6_addr[i] = 0xff;
  }
  return std::make_shared<IPV6Address>(broadcast_addr);
}

IPAddress::sptr IPV6Address::getNetworkAddr(uint32_t prefix_len) const {
  //TODO
  sockaddr_in6 network_addr(m_addr);
  network_addr.sin6_addr.s6_addr[prefix_len / 8] |=
      CreateMask<uint8_t>(prefix_len % 8);
  for (int i = prefix_len / 8 + 1; i < 16; ++i) {
    network_addr.sin6_addr.s6_addr[i] = 0x00;
  }
  return std::make_shared<IPV6Address>(network_addr);
}

IPAddress::sptr IPV6Address::getSubnetMask(uint32_t prefix_len) const {
  //TODO
  sockaddr_in6 subnet;
  memset(&subnet, 0, sizeof(subnet));
  subnet.sin6_family = AF_INET6;
  subnet.sin6_addr.s6_addr[prefix_len / 8] =
      ~CreateMask<uint8_t>(prefix_len % 8);

  for (uint32_t i = 0; i < prefix_len / 8; ++i) {
    subnet.sin6_addr.s6_addr[i] = 0xff;
  }
  return std::make_shared<IPV6Address>(subnet);
}

uint32_t IPV6Address::getPort() const {
  return byteswapOnLittleEndian(m_addr.sin6_port);
}

void IPV6Address::setPort(uint32_t port) {
  m_addr.sin6_port = byteswapOnLittleEndian(port);
}

//UnixAddress

static constexpr size_t MAX_PATH_LEN =
    sizeof(((sockaddr_un*)0)->sun_path) - 1;  //不会解引用，sizeof在编译器计算

UnixAddress::UnixAddress() {
  memset(&m_addr, 0, sizeof(m_addr));
  m_addr.sun_family = AF_UNIX;
  m_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
}
UnixAddress::UnixAddress(const std::string& path) {
  memset(&m_addr, 0, sizeof(m_addr));
  m_addr.sun_family = AF_UNIX;
  m_length = path.size() + 1;  //std::string does not include '\0'
  if (path.empty() || path[0] == '\0') {
    --m_length;
  }

  if (m_length > sizeof(m_addr.sun_path)) {
    throw std::runtime_error("path too long");
  }
  memcpy(m_addr.sun_path, path.c_str(), m_length);
  m_length += offsetof(sockaddr_un, sun_path);
}

//Address
const sockaddr* UnixAddress::getAddr() const {
  return (sockaddr*)&m_addr;
}

socklen_t UnixAddress::getAddrLen() const {
  return m_length;
}

std::ostream& UnixAddress::dump(std::ostream& os) const {
  if (m_length > offsetof(sockaddr_un, sun_path) &&
      m_addr.sun_path[0] == '\0') {
    //abstract socket, start with '\0'
    return os << "\\0"
              << std::string(m_addr.sun_path + 1,
                             m_length - offsetof(sockaddr_un, sun_path) - 1);
  }
  return os << m_addr.sun_path;
}

//UnknownAddress
UnknownAddress::UnknownAddress(int family) {
  memset(&m_addr, 0, sizeof(m_addr));
  m_addr.sa_family = family;
}

UnknownAddress::UnknownAddress(const sockaddr& addr) {
  m_addr = addr;
}

const sockaddr* UnknownAddress::getAddr() const {
  return &m_addr;
}

socklen_t UnknownAddress::getAddrLen() const {
  return sizeof(m_addr);
}

std::ostream& UnknownAddress::dump(std::ostream& os) const {
  os << "[UnknownAddress: family=" << m_addr.sa_family << "]";
  return os;
}
}  // namespace East
