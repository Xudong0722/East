/*
 * @Author: Xudong0722 
 * @Date: 2025-04-24 00:02:16 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-04-24 00:37:38
 */

#include <vector>
#include "../East/include/Address.h"
#include "../East/include/Elog.h"

static East::Logger::sptr g_logger = ELOG_NAME("root");

void test() {
  std::vector<East::Address::sptr> result;
  East::Address::Lookup(result, "www.baidu.com:ftp");
  for (auto& item : result) {
    ELOG_INFO(g_logger) << "item: " << item->toString();
  }
}

void test_iface() {
  std::multimap<std::string, std::pair<East::Address::sptr, uint32_t>> result;

  bool res = East::Address::GetInterfaceAddresses(result);

  if (res) {
    for (auto& i : result) {
      ELOG_INFO(g_logger) << "iface: " << i.first
                          << ", addr: " << i.second.first->toString()
                          << ", mask: " << i.second.second;
    }
  }
}

void test_ipv4() {
  // auto addr = East::IPAddress::Create("www.baidu.com");
  auto addr = East::IPAddress::Create("127.0.0.111");
  if (nullptr != addr) {
    ELOG_INFO(g_logger) << "addr: " << addr->toString();
  } else {
    ELOG_ERROR(g_logger) << "addr is nullptr";
  }
}
int main() {
  // test();
  //test_iface();
  test_ipv4();
  return 0;
}
