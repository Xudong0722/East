/*
 * @Author: Xudong0722 
 * @Date: 2025-04-14 18:46:54 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-06 15:40:57
 */

#include "Address.h"
#include <ifaddrs.h>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "Elog.h"
#include "Endian.h"

namespace East {

East::Logger::sptr g_logger = ELOG_NAME("system");

/**
 * @brief 创建指定位数的掩码
 * @tparam T 掩码类型
 * @param bits 掩码位数
 * @return 对应的掩码值
 * 
 * 例如：CreateMask<uint32_t>(24) 返回 0xFF000000
 */
template <class T>
static T CreateMask(uint32_t bits) {
  return (1 << (sizeof(T) * 8 - bits)) - 1;  //TODO
}

/**
 * @brief 计算一个值中1的位数
 * @tparam T 值类型
 * @param val 要计算的值
 * @return 1的位数
 * 
 * 使用Brian Kernighan算法，每次清除最低位的1
 */
template <class T>
static uint32_t CountBitIsOne(T val) {
  uint32_t cnt{0};
  while (val) {
    ++cnt;
    val &= (val - 1);  //清除最低位的1
  }
  return cnt;
}

Address::sptr Address::Create(const sockaddr* addr, socklen_t addrlen) {
  Address::sptr address{nullptr};
  if (nullptr == addr)
    return address;

  // 根据地址族类型创建对应的Address对象
  switch (addr->sa_family) {
    case AF_INET:
      address = std::make_shared<IPV4Address>(*(sockaddr_in*)addr);
      break;
    case AF_INET6:
      address = std::make_shared<IPV6Address>(*(sockaddr_in6*)addr);
      break;
    default:
      address = std::make_shared<UnknownAddress>(*addr);
      break;
  }
  return address;
}

bool Address::Lookup(std::vector<Address::sptr>& result,
                     const std::string& host, int family, int type,
                     int protocol) {
  addrinfo hints, *results, *next;
  // 初始化地址信息提示结构
  hints.ai_flags = 0;
  hints.ai_family = family;
  hints.ai_protocol = protocol;
  hints.ai_socktype = type;
  hints.ai_addrlen = 0;
  hints.ai_canonname = nullptr;
  hints.ai_addr = nullptr;
  hints.ai_next = nullptr;

  std::string node;
  const char* service{nullptr};

  // 检查IPv6地址格式：[....]:80
  if (!host.empty() && host[0] == '[') {
    const char* endipv6 =
        (const char*)memchr(host.c_str() + 1, ']', host.size() - 1);
    if (nullptr != endipv6) {
      if (*(endipv6 + 1) == ':') {
        service = endipv6 + 2;  //端口号，如80
      }
      node = host.substr(1, endipv6 - host.c_str() - 1);  //ipv6的地址
    }
  }

  // IPv4地址格式：ip:port 或域名:port形式
  if (node.empty()) {
    service = (const char*)memchr(host.c_str(), ':', host.size() - 1);
    if (nullptr != service) {
      if (nullptr ==
          memchr(service + 1, ':',
                 host.c_str() + host.size() - service - 1)) {  //排除ipv6
        node = host.substr(0, service - host.c_str());
        ++service;
      }
    }
  }

  if (node.empty()) {
    node = host;
  }

  // 使用getaddrinfo进行地址解析
  int error =
      getaddrinfo(node.c_str(), service, &hints, &results);  //resulst是一个链表
  if (error) {
    ELOG_ERROR(g_logger) << "Address::Lockup ( " << host << ", " << family
                         << ", " << type << ", " << protocol
                         << ") err: " << error
                         << ", error str: " << strerror(error);
    return false;
  }

  // 遍历所有解析结果，创建对应的Address对象
  next = results;
  while (next) {
    result.emplace_back(Create(next->ai_addr, next->ai_addrlen));
    ELOG_DEBUG(g_logger) << "family: " << next->ai_family
                         << ", sock type: " << next->ai_socktype;
    next = next->ai_next;
  }
  freeaddrinfo(results);
  return !result.empty();
}

Address::sptr Address::LookupAny(const std::string& host, int family, int type,
                                 int protocol) {
  std::vector<Address::sptr> result{};
  if (Lookup(result, host, family, type, protocol)) {
    return result[0];  // 返回第一个找到的地址
  }
  return nullptr;
}

std::shared_ptr<IPAddress> Address::LookupAnyIPAddress(const std::string& host,
                                                       int family, int type,
                                                       int protocol) {
  std::vector<Address::sptr> result{};
  if (Lookup(result, host, family, type, protocol)) {
    // 查找第一个IP地址类型的地址
    for (auto& item : result) {
      IPAddress::sptr v = std::dynamic_pointer_cast<IPAddress>(item);
      if (nullptr != v) {
        return v;
      }
    }
  }
  return nullptr;
}

bool Address::GetInterfaceAddresses(
    std::multimap<std::string, std::pair<Address::sptr, uint32_t>>& result,
    int family) {
  struct ifaddrs *next, *results;
  if (0 != getifaddrs(&results)) {
    ELOG_ERROR(g_logger) << "Address::GetInterfaceAddresses getifaddrs err: "
                         << errno << ", errstr: " << strerror(errno);
    return false;
  }

  try {
    for (next = results; next; next = next->ifa_next) {
      // 过滤指定地址族的接口
      if (family != AF_UNSPEC && family != next->ifa_addr->sa_family)
        continue;
      
      Address::sptr addr{nullptr};
      uint32_t prefix_len = ~0u;
      
      switch (next->ifa_addr->sa_family) {
        case AF_INET: {
          addr = Create(next->ifa_addr, sizeof(sockaddr_in));
          uint32_t netmask = ((sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;
          prefix_len = CountBitIsOne(netmask);
        } break;
        case AF_INET6: {
          addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
          in6_addr& netmask = ((sockaddr_in6*)next->ifa_netmask)->sin6_addr;
          prefix_len = 0;
          for (int i = 0; i < 16; ++i) {
            prefix_len += CountBitIsOne(netmask.s6_addr[i]);
          }
        } break;
        default:
          break;
      }

      if (nullptr != addr) {
        result.insert({next->ifa_name, {addr, prefix_len}});
      }
    }
  } catch (...) {
    ELOG_ERROR(g_logger) << "Address::GetInterfaceAddresses exception";
    freeifaddrs(results);
    return false;
  }
  freeifaddrs(results);
  return !result.empty();
}

bool Address::GetInterfaceAddresses(
    std::vector<std::pair<Address::sptr, uint32_t>>& result,
    const std::string& iface, int family) {
  // 如果接口名为空或"*"，返回所有接口的地址
  if (iface.empty() || iface == "*") {
    if (family == AF_INET || family == AF_INET) {
      result.emplace_back(std::make_pair(std::make_shared<IPV4Address>(), 0));
    }
    if (family == AF_INET6 || family == AF_INET6) {
      result.emplace_back(std::make_pair(std::make_shared<IPV6Address>(), 0));
    }
    return true;
  }

  // 获取所有接口地址，然后过滤指定接口
  std::multimap<std::string, std::pair<Address::sptr, uint32_t>> results;
  if (!GetInterfaceAddresses(results, family)) {
    return false;
  }

  auto range = results.equal_range(iface);
  for (auto it = range.first; it != range.second; ++it) {
    result.emplace_back(it->second);
  }
  return !result.empty();
}

int Address::getFamily() const {
  return getAddr()->sa_family;
}

std::string Address::toString() const {
  std::stringstream ss;
  dump(ss);
  return ss.str();
}

bool Address::operator<(const Address& rhs) const {
  // 先比较地址内容，再比较地址长度
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

IPAddress::sptr IPAddress::Create(const char* address, uint16_t port) {
  if (nullptr == address)
    return nullptr;
  
  addrinfo hints, *results;
  memset(&hints, 0, sizeof(hints));
  hints.ai_flags = 0;
  hints.ai_family = AF_UNSPEC;

  // 解析地址字符串
  int error =
      getaddrinfo(address, nullptr, &hints, &results);  //resulst是一个链表
  if (error) {
    ELOG_ERROR(g_logger) << "Address::Create ( " << address << ", " << port
                         << ") err: " << error
                         << ", error str: " << strerror(error);
    return nullptr;
  }

  IPAddress::sptr result{nullptr};
  try {
    result = std::dynamic_pointer_cast<IPAddress>(
        Address::Create(results->ai_addr, results->ai_addrlen));
    if (nullptr != result) {
      result->setPort(port);
    }
  } catch (...) {
    freeaddrinfo(results);
    return nullptr;
  }
  freeaddrinfo(results);
  return result;
}

// IPv4地址实现
IPV4Address::IPV4Address(const sockaddr_in& addr) {
  m_addr = addr;
}

IPV4Address::IPV4Address(uint32_t address, uint16_t port) {
  memset(&m_addr, 0, sizeof(m_addr));
  m_addr.sin_family = AF_INET;
  m_addr.sin_port = byteswapOnLittleEndian(port);  //转成大端字节序
  m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address);  //转成大端字节序
}

IPV4Address::sptr IPV4Address::Create(const char* address, uint16_t port) {
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
  // 将网络字节序转换为主机字节序进行显示
  uint32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
  os << ((addr >> 24) & 0xff) << "." << ((addr >> 16) & 0xff) << "."
     << ((addr >> 8) & 0xff) << "." << (addr & 0xff) << ":"
     << byteswapOnLittleEndian(m_addr.sin_port);
  return os;
}

// 计算广播地址：192.168.1.255，发送给192.168.1.0这个子网上的所有主机
IPAddress::sptr IPV4Address::getBroadcastAddr(uint32_t prefix_len) const {
  if (prefix_len > 32) {
    return nullptr;
  }
  sockaddr_in broadcast_addr(m_addr);
  // 广播地址 = 网络地址 | 掩码
  broadcast_addr.sin_addr.s_addr |= byteswapOnLittleEndian(
      CreateMask<uint32_t>(prefix_len));
  return std::make_shared<IPV4Address>(broadcast_addr);
}

// 计算网络地址：192.168.1.0这就是一个子网地址，对应的广播地址是192.168.1.255
IPAddress::sptr IPV4Address::getNetworkAddr(uint32_t prefix_len) const {
  if (prefix_len > 32) {
    return nullptr;
  }
  sockaddr_in network_addr(m_addr);
  // 网络地址 = 主机地址 & (~掩码)
  network_addr.sin_addr.s_addr &=
      byteswapOnLittleEndian(~CreateMask<uint32_t>(prefix_len));
  return std::make_shared<IPV4Address>(network_addr);
}

IPAddress::sptr IPV4Address::getSubnetMask(uint32_t prefix_len) const {
  sockaddr_in subnet_mask;
  memset(&subnet_mask, 0, sizeof(subnet_mask));
  subnet_mask.sin_family = AF_INET;
  // 子网掩码 = ~(前缀掩码)
  subnet_mask.sin_addr.s_addr =
      ~byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
  return std::make_shared<IPV4Address>(subnet_mask);
}

uint16_t IPV4Address::getPort() const {
  return byteswapOnLittleEndian(m_addr.sin_port);
}

void IPV4Address::setPort(uint16_t port) {
  m_addr.sin_port = byteswapOnLittleEndian(port);
}

// IPv6地址实现
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
  // 将uint8_t[16]转换为8个uint16_t进行处理
  uint16_t* addr =
      (uint16_t*)&m_addr.sin6_addr.s6_addr;
  bool used_zeros{false};
  
  // 处理IPv6地址的压缩表示（省略连续的0）
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
  // TODO: IPv6广播地址实现
  sockaddr_in6 broadcast_addr(m_addr);
  broadcast_addr.sin6_addr.s6_addr[prefix_len / 8] |=
      CreateMask<uint8_t>(prefix_len % 8);
  for (int i = prefix_len / 8 + 1; i < 16; ++i) {
    broadcast_addr.sin6_addr.s6_addr[i] = 0xff;
  }
  return std::make_shared<IPV6Address>(broadcast_addr);
}

IPAddress::sptr IPV6Address::getNetworkAddr(uint32_t prefix_len) const {
  // TODO: IPv6网络地址实现
  sockaddr_in6 network_addr(m_addr);
  network_addr.sin6_addr.s6_addr[prefix_len / 8] |=
      CreateMask<uint8_t>(prefix_len % 8);
  for (int i = prefix_len / 8 + 1; i < 16; ++i) {
    network_addr.sin6_addr.s6_addr[i] = 0x00;
  }
  return std::make_shared<IPV6Address>(network_addr);
}

IPAddress::sptr IPV6Address::getSubnetMask(uint32_t prefix_len) const {
  // TODO: IPv6子网掩码实现
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

uint16_t IPV6Address::getPort() const {
  return byteswapOnLittleEndian(m_addr.sin6_port);
}

void IPV6Address::setPort(uint16_t port) {
  m_addr.sin6_port = byteswapOnLittleEndian(port);
}

// Unix域套接字地址实现
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

const sockaddr* UnixAddress::getAddr() const {
  return (sockaddr*)&m_addr;
}

socklen_t UnixAddress::getAddrLen() const {
  return m_length;
}

void UnixAddress::setAddrLen(uint32_t len) {
  m_length = len;
}

std::ostream& UnixAddress::dump(std::ostream& os) const {
  if (m_length > offsetof(sockaddr_un, sun_path) &&
      m_addr.sun_path[0] == '\0') {
    // 抽象套接字，以'\0'开头
    return os << "\\0"
              << std::string(m_addr.sun_path + 1,
                             m_length - offsetof(sockaddr_un, sun_path) - 1);
  }
  return os << m_addr.sun_path;
}

// 未知类型地址实现
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

std::ostream& operator<<(std::ostream& os, const Address& addr) {
  return addr.dump(os);
}

}  // namespace East
