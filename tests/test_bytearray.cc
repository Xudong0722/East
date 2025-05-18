/*
 * @Author: Xudong0722 
 * @Date: 2025-05-18 13:05:55 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-05-18 16:58:39
 */

#include <functional>
#include <vector>
#include "../East/include/ByteArray.h"
#include "../East/include/Elog.h"
#include "../East/include/Macro.h"

static East::Logger::sptr g_logger = ELOG_NAME("root");

template <typename T, typename WF, typename RF, int len, int base_len>
void unit_test(WF wf, RF rf) {
  std::vector<T> vec;
  for (int i = 0; i < len; ++i) {
    vec.emplace_back(rand());
  }

  East::ByteArray::sptr sba = std::make_shared<East::ByteArray>(base_len);
  for (const auto& i : vec) {
    (sba.get()->*wf)(i);
  }
  sba->setOffset(0);
  for (size_t i = 0; i < vec.size(); ++i) {
    T tmp = (sba.get()->*rf)();
    EAST_ASSERT(tmp == vec[i]);
  }

  EAST_ASSERT(sba->getReadableSize() == 0);
  //ELOG_INFO(g_logger) << (char*)(wf) << " " << (char*)(rf);
}

void unit_test2() {
#define XX(type, len, write_fun, read_fun, base_len)                           \
  {                                                                            \
    std::vector<type> vec;                                                     \
    for (int i = 0; i < len; ++i) {                                            \
      vec.push_back(rand());                                                   \
    }                                                                          \
    East::ByteArray::sptr ba(new East::ByteArray(base_len));                   \
    for (auto& i : vec) {                                                      \
      ba->write_fun(i);                                                        \
    }                                                                          \
    ba->setOffset(0);                                                          \
    for (size_t i = 0; i < vec.size(); ++i) {                                  \
      type v = ba->read_fun();                                                 \
      EAST_ASSERT(v == vec[i]);                                                \
    }                                                                          \
    EAST_ASSERT(ba->getReadableSize() == 0);                                   \
    ELOG_INFO(g_logger) << #write_fun "/" #read_fun " (" #type ") len=" << len \
                        << " base_len=" << base_len                            \
                        << " size=" << ba->getSize();                          \
  }

  XX(int8_t, 100, writeFixInt8, readFixInt8, 1);
  XX(uint8_t, 100, writeFixUInt8, readFixUInt8, 1);
  XX(int16_t, 100, writeFixInt16, readFixInt16, 1);
  XX(uint16_t, 100, writeFixUInt16, readFixUInt16, 1);
  XX(int32_t, 100, writeFixInt32, readFixInt32, 1);
  XX(uint32_t, 100, writeFixUInt32, readFixUInt32, 1);
  XX(int64_t, 100, writeFixInt64, readFixInt64, 1);
  XX(uint64_t, 100, writeFixUInt64, readFixUInt64, 1);

  XX(int32_t, 100, writeInt32, readInt32, 1);
  XX(uint32_t, 100, writeUInt32, readUInt32, 1);
  XX(int64_t, 100, writeInt64, readInt64, 1);
  XX(uint64_t, 100, writeUInt64, readUInt64, 1);
#undef XX
}

void unit_test3() {
#define XX(type, len, write_fun, read_fun, base_len)                           \
  {                                                                            \
    std::vector<type> vec;                                                     \
    for (int i = 0; i < len; ++i) {                                            \
      vec.push_back(rand());                                                   \
    }                                                                          \
    East::ByteArray::sptr ba(new East::ByteArray(base_len));                   \
    for (auto& i : vec) {                                                      \
      ba->write_fun(i);                                                        \
    }                                                                          \
    ba->setOffset(0);                                                          \
    for (size_t i = 0; i < vec.size(); ++i) {                                  \
      type v = ba->read_fun();                                                 \
      EAST_ASSERT(v == vec[i]);                                                \
    }                                                                          \
    EAST_ASSERT(ba->getReadableSize() == 0);                                   \
    ELOG_INFO(g_logger) << #write_fun "/" #read_fun " (" #type ") len=" << len \
                        << " base_len=" << base_len                            \
                        << " size=" << ba->getSize();                          \
    ba->setOffset(0);                                                          \
    EAST_ASSERT(ba->writeToFile("/tmp/" #type "-" #len "-" #read_fun ".dat")); \
    East::ByteArray::sptr ba2(new East::ByteArray(base_len * 2));              \
    EAST_ASSERT(                                                               \
        ba2->readFromFile("/tmp/" #type "-" #len "-" #read_fun ".dat"));       \
    ba2->setOffset(0);                                                         \
    EAST_ASSERT(ba->toString() == ba2->toString());                            \
    EAST_ASSERT(ba->getOffset() == 0);                                         \
    EAST_ASSERT(ba->getOffset() == 0);                                         \
  }

  XX(int8_t, 100, writeFixInt8, readFixInt8, 1);
  XX(uint8_t, 100, writeFixUInt8, readFixUInt8, 1);
  XX(int16_t, 100, writeFixInt16, readFixInt16, 1);
  XX(uint16_t, 100, writeFixUInt16, readFixUInt16, 1);
  XX(int32_t, 100, writeFixInt32, readFixInt32, 1);
  XX(uint32_t, 100, writeFixUInt32, readFixUInt32, 1);
  XX(int64_t, 100, writeFixInt64, readFixInt64, 1);
  XX(uint64_t, 100, writeFixUInt64, readFixUInt64, 1);

  XX(int32_t, 100, writeInt32, readInt32, 1);
  XX(uint32_t, 100, writeUInt32, readUInt32, 1);
  XX(int64_t, 100, writeInt64, readInt64, 1);
  XX(uint64_t, 100, writeUInt64, readUInt64, 1);
#undef XX
}

void test() {
  using namespace East;
  //    unit_test<int8_t, decltype(&ByteArray::writeFixInt8), decltype(&ByteArray::readFixInt8),100, 1>(
  //     &ByteArray::writeFixInt8,
  //     &ByteArray::readFixInt8
  //    );
  ELOG_INFO(g_logger)
      << "----------------test write/read stream--------------------";
  unit_test2();

  ELOG_INFO(g_logger)
      << "----------------test write/read file-----------------------";

  unit_test3();
}

int main() {
  test();
  return 0;
}