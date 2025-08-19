/*
 * @Author: Xudong0722 
 * @Date: 2025-05-12 22:12:30 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-10 21:43:35
 */

#include "ByteArray.h"
#include <cstring>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include "Elog.h"
#include "Endian.h"

namespace East {
static East::Logger::sptr g_logger = ELOG_NAME("system");

/**
 * @brief Node构造函数，分配指定大小的内存
 * @param s 内存块大小（字节）
 */
ByteArray::Node::Node(size_t s) : ptr(new char[s]), next(nullptr), size(s) {}

/**
 * @brief Node默认构造函数
 */
ByteArray::Node::Node() {}

/**
 * @brief Node析构函数，释放分配的内存
 */
ByteArray::Node::~Node() {
  if (nullptr != ptr) {
    delete[] ptr;
  }
}

/**
 * @brief ByteArray构造函数
 * @param base_size 基础内存块大小，默认为4096字节
 * 
 * 初始化ByteArray对象，创建根节点并设置初始容量
 */
ByteArray::ByteArray(size_t base_size)
    : m_block_size(base_size),
      m_offset(0),
      m_size(base_size),
      m_capacity(base_size),
      m_endian(EAST_BIG_ENDIAN),
      m_root(new Node(base_size)),
      m_cur(m_root) {}

/**
 * @brief ByteArray析构函数
 * 
 * 释放所有内存块，清理链表结构
 */
ByteArray::~ByteArray() {
  auto tmp = m_root;
  while (tmp) {
    m_cur = tmp;
    tmp = tmp->next;
    delete m_cur;
    m_cur = nullptr;
  }
}

/**
 * @brief 将32位有符号整数编码为ZigZag格式
 * @param n 要编码的32位整数
 * @return ZigZag编码后的32位无符号整数
 * 
 * ZigZag编码将负数映射到正数：0->0, -1->1, 1->2, -2->3, 2->4...
 * 公式：(n << 1) ^ (n >> 31)
 */
uint32_t ByteArray::EncodeZigZagI32(int32_t n) {
  return (n << 1) ^ (n >> 31);
}

/**
 * @brief 将64位有符号整数编码为ZigZag格式
 * @param n 要编码的64位整数
 * @return ZigZag编码后的64位无符号整数
 * 
 * 64位版本的ZigZag编码，公式：(n << 1) ^ (n >> 63)
 */
uint64_t ByteArray::EncodeZigZagI64(int64_t n) {
  return (n << 1) ^ (n >> 63);
}

/**
 * @brief 将ZigZag编码的32位无符号整数解码为有符号整数
 * @param n ZigZag编码的32位无符号整数
 * @return 解码后的32位有符号整数
 * 
 * 解码公式：(n >> 1) ^ -(n & 1)
 */
int32_t ByteArray::DecodeZigZagI32(uint32_t n) {
  return (n >> 1) ^ -(n & 1);
}

/**
 * @brief 将ZigZag编码的64位无符号整数解码为有符号整数
 * @param n ZigZag编码的64位无符号整数
 * @return 解码后的64位有符号整数
 * 
 * 64位版本的ZigZag解码，公式：(n >> 1) ^ -(n & 1)
 */
int64_t ByteArray::DecodeZigZagI64(uint64_t n) {
  return (n >> 1) ^ -(n & 1);
}

// 固定长度整数写入函数
/**
 * @brief 写入8位有符号整数
 * @param val 要写入的值
 */
void ByteArray::writeFixInt8(int8_t val) {
  write(&val, sizeof(val));
}

/**
 * @brief 写入8位无符号整数
 * @param val 要写入的值
 */
void ByteArray::writeFixUInt8(uint8_t val) {
  write(&val, sizeof(val));
}

/**
 * @brief 写入16位有符号整数
 * @param val 要写入的值
 * 
 * 根据字节序设置进行字节序转换
 */
void ByteArray::writeFixInt16(int16_t val) {
  if (m_endian != EAST_BYTE_ORDER) {
    val = byteswap(val);
  }
  write(&val, sizeof(val));
}

/**
 * @brief 写入16位无符号整数
 * @param val 要写入的值
 * 
 * 根据字节序设置进行字节序转换
 */
void ByteArray::writeFixUInt16(uint16_t val) {
  if (m_endian != EAST_BYTE_ORDER) {
    val = byteswap(val);
  }
  write(&val, sizeof(val));
}

/**
 * @brief 写入32位有符号整数
 * @param val 要写入的值
 * 
 * 根据字节序设置进行字节序转换
 */
void ByteArray::writeFixInt32(int32_t val) {
  if (m_endian != EAST_BYTE_ORDER) {
    val = byteswap(val);
  }
  write(&val, sizeof(val));
}

/**
 * @brief 写入32位无符号整数
 * @param val 要写入的值
 * 
 * 根据字节序设置进行字节序转换
 */
void ByteArray::writeFixUInt32(uint32_t val) {
  if (m_endian != EAST_BYTE_ORDER) {
    val = byteswap(val);
  }
  write(&val, sizeof(val));
}

/**
 * @brief 写入64位有符号整数
 * @param val 要写入的值
 * 
 * 根据字节序设置进行字节序转换
 */
void ByteArray::writeFixInt64(int64_t val) {
  if (m_endian != EAST_BYTE_ORDER) {
    val = byteswap(val);
  }
  write(&val, sizeof(val));
}

/**
 * @brief 写入64位无符号整数
 * @param val 要写入的值
 * 
 * 根据字节序设置进行字节序转换
 */
void ByteArray::writeFixUInt64(uint64_t val) {
  if (m_endian != EAST_BYTE_ORDER) {
    val = byteswap(val);
  }
  write(&val, sizeof(val));
}

/**
 * @brief 写入32位有符号整数（使用ZigZag编码和变长编码）
 * @param val 要写入的值
 * 
 * 先将负数转换为ZigZag格式，然后使用变长编码写入
 */
void ByteArray::writeInt32(int32_t val) {
  // 可能为负，我们将其转换一下
  uint32_t zigzag = EncodeZigZagI32(val);
  writeUInt32(zigzag);
}

/**
 * @brief 写入32位无符号整数（使用变长编码）
 * @param val 要写入的值
 * 
 * 使用变长编码，每个字节使用7位存储数据，最高位表示是否还有后续字节
 */
void ByteArray::writeUInt32(uint32_t val) {
  uint8_t buf[5];  // 5*7 > 32，最多需要5个字节
  uint8_t i = 0;
  while (val >= 0x80) {  // 当前字节不是最后一个字节
    buf[i++] = (val & 0x7f) | 0x80;
    val >>= 7;
  }
  buf[i++] = val & 0x7f;
  write(buf, i);
}

/**
 * @brief 写入64位有符号整数（使用ZigZag编码和变长编码）
 * @param val 要写入的值
 * 
 * 先将负数转换为ZigZag格式，然后使用变长编码写入
 */
void ByteArray::writeInt64(int64_t val) {
  uint64_t zigzag = EncodeZigZagI64(val);
  writeUInt64(zigzag);
}

/**
 * @brief 写入64位无符号整数（使用变长编码）
 * @param val 要写入的值
 * 
 * 使用变长编码，每个字节使用7位存储数据，最高位表示是否还有后续字节
 */
void ByteArray::writeUInt64(uint64_t val) {
  uint8_t buf[10];  // 10*7 > 64，最多需要10个字节
  uint8_t i = 0;
  while (val >= 0x80) {  // 当前字节不是最后一个字节
    buf[i++] = (val & 0x7f) | 0x80;
    val >>= 7;
  }
  buf[i++] = val & 0x7f;
  write(buf, i);
}

/**
 * @brief 写入32位浮点数
 * @param val 要写入的值
 * 
 * 将浮点数转换为32位整数后写入
 */
void ByteArray::writeFloat(float val) {
  uint32_t tmp{0};
  memcpy(&tmp, &val, sizeof(val));
  writeUInt32(tmp);
}

/**
 * @brief 写入64位浮点数
 * @param val 要写入的值
 * 
 * 将浮点数转换为64位整数后写入
 */
void ByteArray::writeDouble(double val) {
  uint64_t tmp{0};
  memcpy(&tmp, &val, sizeof(val));
  writeUInt64(tmp);
}

/**
 * @brief 写入字符串，长度使用16位整数表示
 * @param val 要写入的字符串
 */
void ByteArray::writeStringFix16(const std::string& val) {
  writeFixUInt16(static_cast<uint16_t>(val.size()));
  write(val.c_str(), val.size());
}

/**
 * @brief 写入字符串，长度使用32位整数表示
 * @param val 要写入的字符串
 */
void ByteArray::writeStringFix32(const std::string& val) {
  writeFixUInt32(static_cast<uint32_t>(val.size()));
  write(val.c_str(), val.size());
}

/**
 * @brief 写入字符串，长度使用64位整数表示
 * @param val 要写入的字符串
 */
void ByteArray::writeStringFix64(const std::string& val) {
  writeFixUInt64(val.size());
  write(val.c_str(), val.size());
}

/**
 * @brief 写入字符串，长度使用变长整数表示
 * @param val 要写入的字符串
 * 
 * 这里直接用uint64来处理字符串长度
 */
void ByteArray::writeStringVarint(const std::string& val) {
  // 这里直接用uint64来处理
  writeUInt64(val.size());
  write(val.c_str(), val.size());
}

/**
 * @brief 写入字符串，不包含长度信息
 * @param val 要写入的字符串
 */
void ByteArray::writeStringWithoutLen(const std::string& val) {
  write(val.c_str(), val.size());
}

/**
 * @brief 固定长度整数读取宏定义
 * 
 * 用于简化固定长度整数读取的代码，自动处理字节序转换
 */
#define READ(type)                   \
  type tmp{};                        \
  read(&tmp, sizeof(tmp));           \
  if (m_endian == EAST_BYTE_ORDER) { \
    return tmp;                      \
  }                                  \
  return byteswap(tmp);

/**
 * @brief 读取8位有符号整数
 * @return 读取的8位有符号整数
 */
int8_t ByteArray::readFixInt8() {
  int8_t v{0};
  read(&v, sizeof(v));
  return v;
}

/**
 * @brief 读取8位无符号整数
 * @return 读取的8位无符号整数
 */
uint8_t ByteArray::readFixUInt8() {
  uint8_t v{0};
  read(&v, sizeof(v));
  return v;
}

/**
 * @brief 读取16位有符号整数
 * @return 读取的16位有符号整数
 */
int16_t ByteArray::readFixInt16() {
  READ(int16_t);
}

/**
 * @brief 读取16位无符号整数
 * @return 读取的16位无符号整数
 */
uint16_t ByteArray::readFixUInt16() {
  READ(uint16_t);
}

/**
 * @brief 读取32位有符号整数
 * @return 读取的32位有符号整数
 */
int32_t ByteArray::readFixInt32() {
  READ(int32_t);
}

/**
 * @brief 读取32位无符号整数
 * @return 读取的32位无符号整数
 */
uint32_t ByteArray::readFixUInt32() {
  READ(uint32_t);
}

/**
 * @brief 读取64位有符号整数
 * @return 读取的64位有符号整数
 */
int64_t ByteArray::readFixInt64() {
  READ(int64_t);
}

/**
 * @brief 读取64位无符号整数
 * @return 读取的64位无符号整数
 */
uint64_t ByteArray::readFixUInt64() {
  READ(uint64_t);
}

#undef READ

/**
 * @brief 读取32位有符号整数（变长编码）
 * @return 读取的32位有符号整数
 * 
 * 先读取变长编码的uint32，然后解码为int32
 */
int32_t ByteArray::readInt32() {
  return DecodeZigZagI32(readUInt32());
}

/**
 * @brief 读取32位无符号整数（变长编码）
 * @return 读取的32位无符号整数
 * 
 * 使用变长编码读取，每个字节使用7位存储数据
 */
uint32_t ByteArray::readUInt32() {
  uint32_t res{0};
  for (int i = 0; i < 32; i += 7) {
    uint8_t byte = readFixUInt8();
    if ((byte & 0x80) != 0x80) {
      res |= ((uint32_t)byte) << i;
      break;
    }
    res |= ((uint32_t)(byte & 0x7f)) << i;
  }
  return res;
}

/**
 * @brief 读取64位有符号整数（变长编码）
 * @return 读取的64位有符号整数
 * 
 * 先读取变长编码的uint64，然后解码为int64
 */
int64_t ByteArray::readInt64() {
  return DecodeZigZagI64(readUInt64());
}

/**
 * @brief 读取64位无符号整数（变长编码）
 * @return 读取的64位无符号整数
 * 
 * 使用变长编码读取，每个字节使用7位存储数据
 */
uint64_t ByteArray::readUInt64() {
  uint64_t res{0};
  for (int i = 0; i < 64; i += 7) {
    uint8_t byte = readFixUInt8();
    if ((byte & 0x80) != 0x80) {
      res |= ((uint64_t)byte) << i;
      break;
    } else {
      res |= ((uint64_t)(byte & 0x7f)) << i;
    }
  }
  return res;
}

/**
 * @brief 读取32位浮点数
 * @return 读取的32位浮点数
 * 
 * 先读取uint32，然后转换为float
 */
float ByteArray::readFloat() {
  uint32_t tmp = readUInt32();
  float val{0.0f};
  memcpy(&val, &tmp, sizeof(tmp));
  return val;
}

/**
 * @brief 读取64位浮点数
 * @return 读取的64位浮点数
 * 
 * 先读取uint64，然后转换为double
 */
double ByteArray::readDouble() {
  uint64_t tmp = readUInt64();
  double val{0.0};
  memcpy(&val, &tmp, sizeof(tmp));
  return val;
}

/**
 * @brief 读取字符串，长度使用16位整数表示
 * @return 读取的字符串
 */
std::string ByteArray::readStringFix16() {
  uint16_t len = readFixUInt16();
  if (len == 0) {
    return std::string{};
  }
  std::string str(len, ' ');
  read(&str[0], len);
  return str;
}

/**
 * @brief 读取字符串，长度使用32位整数表示
 * @return 读取的字符串
 */
std::string ByteArray::readStringFix32() {
  uint32_t len = readFixUInt32();
  if (len == 0) {
    return std::string{};
  }
  std::string str(len, ' ');
  read(&str[0], len);
  return str;
}

/**
 * @brief 读取字符串，长度使用64位整数表示
 * @return 读取的字符串
 */
std::string ByteArray::readStringFix64() {
  uint64_t len = readFixUInt64();
  if (len == 0) {
    return std::string{};
  }
  std::string str(len, ' ');
  read(&str[0], len);
  return str;
}

/**
 * @brief 读取字符串，长度使用变长整数表示
 * @return 读取的字符串
 */
std::string ByteArray::readStringVarint() {
  uint64_t len = readInt64();
  if (len == 0) {
    return std::string{};
  }
  std::string str(len, ' ');
  read(&str[0], len);
  return str;
}

/**
 * @brief 清空所有数据，保留根节点
 * 
 * 重置偏移量和大小，删除除根节点外的所有内存块
 */
void ByteArray::clear() {
  // 只保留根节点
  m_offset = 0;
  m_size = 0;
  m_capacity = m_block_size;
  Node* tmp = m_root->next;
  while (tmp) {
    m_cur = tmp;
    tmp = tmp->next;
    delete m_cur;
  }
  m_cur = m_root;
  m_root->next = nullptr;
}

/**
 * @brief 写入原始字节数据
 * @param buf 要写入的数据缓冲区
 * @param size 数据大小
 * 
 * 自动扩容并跨内存块写入数据
 */
void ByteArray::write(const void* buf, size_t size) {
  if (size == 0)
    return;
  addCapacity(size);  // 尝试扩容，如果足够，什么也不做

  size_t cur_offset = m_offset % m_block_size;  // 当前内存块写到哪里了
  size_t cur_writeable = m_cur->size - cur_offset;  // 当前内存块剩余的可写空间
  size_t buf_offset{0};

  while (size > 0) {
    if (cur_writeable >= size) {
      // 当前内存块剩余空间足够
      memcpy((char*)m_cur->ptr + cur_offset, (const char*)buf + buf_offset,
             size);
      if (m_cur->size == (cur_offset + size)) {
        m_cur = m_cur->next;
      }
      m_offset += size;
      buf_offset += size;
      break;
    }
    memcpy((char*)m_cur->ptr + cur_offset, (const char*)buf + buf_offset,
           cur_writeable);  // 先写这么多，剩下的放后面的节点中写
    m_offset += cur_writeable;
    size -= cur_writeable;
    buf_offset += cur_writeable;
    m_cur = m_cur->next;  // 换到下一个内存块
    cur_offset = 0;
    cur_writeable = m_cur->size;
  }
  if (m_offset > m_size) {
    m_size = m_offset;
  }
}

/**
 * @brief 从当前偏移位置读取数据
 * @param buf 输出缓冲区
 * @param size 要读取的数据大小
 * @throw std::out_of_range 当读取大小超过可读数据时抛出异常
 * 
 * 跨内存块读取数据，自动更新当前内存块指针
 */
void ByteArray::read(void* buf, size_t size) {
  // read一般从头开始读，所以使用前会改变m_offset的位置
  if (size > getReadableSize()) {
    throw std::out_of_range("read error");
  }

  size_t cur_offset = m_offset % m_block_size;
  size_t cur_readable = m_cur->size - cur_offset;
  size_t buf_offset{0};

  while (size > 0) {
    if (cur_readable >= size) {
      memcpy((char*)buf + buf_offset, m_cur->ptr + cur_offset, size);

      // 如果正好读完，移动下当前内存指针
      if (cur_offset + size == m_cur->size) {
        m_cur = m_cur->next;
      }
      m_offset += size;
      buf_offset += size;
      size = 0;
    } else {
      memcpy((char*)buf + buf_offset, m_cur->ptr + cur_offset, cur_readable);
      m_offset += cur_readable;
      buf_offset += cur_readable;
      size -= cur_readable;
      m_cur = m_cur->next;
      cur_offset = 0;
      cur_readable = m_cur->size;
    }
  }
}

/**
 * @brief 从指定偏移位置读取数据（不改变当前偏移）
 * @param buf 输出缓冲区
 * @param size 要读取的数据大小
 * @param offset 读取偏移位置
 * @throw std::out_of_range 当读取大小超过可读数据时抛出异常
 * 
 * 从指定位置读取数据，不影响当前读写位置
 */
void ByteArray::read(void* buf, size_t size, size_t offset) const {
  if (size > (m_size - offset)) {
    throw std::out_of_range("read error");
  }

  size_t cur_offset = offset % m_block_size;
  size_t cur_readable = m_cur->size - cur_offset;
  size_t buf_offset{0};
  Node* cur = m_cur;
  while (size > 0) {
    if (cur_readable >= size) {
      memcpy((char*)buf + buf_offset, cur->ptr + cur_offset, size);

      // 如果正好读完，移动下当前内存指针
      if ((cur_offset + size) == cur->size) {
        cur = cur->next;
      }
      offset += size;
      buf_offset += size;
      size = 0;
    } else {
      memcpy((char*)buf + buf_offset, cur->ptr + cur_offset, cur_readable);
      offset += cur_readable;
      buf_offset += cur_readable;
      size -= cur_readable;
      cur = cur->next;
      cur_offset = 0;
      cur_readable = cur->size;
    }
  }
}

/**
 * @brief 获取当前读写偏移位置
 * @return 当前偏移位置
 */
size_t ByteArray::getOffset() const {
  return m_offset;
}

/**
 * @brief 设置读写偏移位置
 * @param offset 新的偏移位置
 * @throw std::out_of_range 当偏移超出容量时抛出异常
 * 
 * 设置偏移位置并更新当前内存块指针
 */
void ByteArray::setOffset(size_t offset) {
  if (offset > m_capacity) {
    throw std::out_of_range("set offset out of range");
  }

  m_offset = offset;
  if (m_offset > m_size) {
    m_size = m_offset;
  }
  m_cur = m_root;
  while (offset >= m_cur->size) {
    offset -= m_cur->size;
    m_cur = m_cur->next;
  }

  if (offset == m_cur->size) {
    m_cur = m_cur->next;
  }
}

/**
 * @brief 将数据写入文件
 * @param file_name 文件名
 * @return 写入成功返回true，失败返回false
 * 
 * 以二进制模式写入文件，从当前偏移位置开始写入所有可读数据
 */
bool ByteArray::writeToFile(const std::string& file_name) const {
  std::ofstream ofs;
  ofs.open(file_name, std::ios::trunc | std::ios::binary);
  if (!ofs) {
    ELOG_ERROR(g_logger) << ", file_name = " << file_name
                         << ", errno = " << errno
                         << ", error str = " << strerror(errno);
    return false;
  }

  int64_t read_size = getReadableSize();
  int64_t offset = m_offset;
  Node* cur = m_cur;

  while (read_size > 0) {
    int64_t cur_offset = offset % m_block_size;
    int64_t len = (read_size + cur_offset > (int64_t)m_block_size
                       ? m_block_size - cur_offset
                       : read_size);
    ofs.write(cur->ptr + cur_offset, len);
    cur = cur->next;
    offset += len;
    read_size -= len;
  }
  return true;
}

/**
 * @brief 从文件读取数据
 * @param file_name 文件名
 * @return 读取成功返回true，失败返回false
 * 
 * 以二进制模式读取文件，将文件内容写入ByteArray
 */
bool ByteArray::readFromFile(const std::string& file_name) {
  std::ifstream ifs;
  ifs.open(file_name, std::ios::binary);
  if (!ifs) {
    ELOG_ERROR(g_logger) << "readFromFile, file_name = " << file_name
                         << ", errno = " << errno
                         << ", str errno = " << strerror(errno);
    return false;
  }

  std::unique_ptr<char[]> buf(new char[m_block_size]);
  while (!ifs.eof()) {
    ifs.read(buf.get(), m_block_size);
    write(buf.get(), ifs.gcount());
  }
  return true;
}

/**
 * @brief 获取基础内存块大小
 * @return 基础内存块大小
 */
size_t ByteArray::getBaseSize() const {
  return m_block_size;
}

/**
 * @brief 获取可读数据大小
 * @return 可读数据大小（总大小减去当前偏移）
 */
size_t ByteArray::getReadableSize() const {
  return m_size - m_offset;
}

/**
 * @brief 检查是否为小端字节序
 * @return 是小端字节序返回true，否则返回false
 */
bool ByteArray::isLittleEndian() const {
  return m_endian;
}

/**
 * @brief 设置字节序
 * @param little_endian true表示小端，false表示大端
 */
void ByteArray::setLittleEndian(bool little_endian) {
  if (little_endian) {
    m_endian = EAST_LITTLE_ENDIAN;
  } else {
    m_endian = EAST_BIG_ENDIAN;
  }
}

/**
 * @brief 将数据转换为字符串
 * @return 数据的字符串表示
 * 
 * 从当前偏移位置读取所有可读数据并转换为字符串
 */
std::string ByteArray::toString() const {
  std::string res;
  res.resize(getReadableSize());

  if (res.empty())
    return res;
  read(&res[0], res.size(), m_offset);
  return res;
}

/**
 * @brief 将数据转换为十六进制字符串
 * @return 数据的十六进制字符串表示
 * 
 * 每32个字节换行，便于查看大数据的十六进制表示
 */
std::string ByteArray::toHexString() const {
  std::string res = toString();
  std::stringstream ss;
  for (auto i = 0u; i < res.size(); ++i) {
    if (i > 0 && i % 32 == 0) {
      ss << std::endl;
    }
    ss << std::setw(2) << std::setfill('0') << std::hex << (int)(uint8_t)res[i]
       << " ";
  }
  return ss.str();
}

/**
 * @brief 获取可读数据的iovec数组（从当前偏移开始）
 * @param buffers 输出参数，存储iovec结构
 * @param len 要获取的数据长度，默认为所有可读数据
 * @return 实际获取的数据长度
 * 
 * 用于零拷贝传输，将数据分割为多个iovec结构
 */
uint64_t ByteArray::getReadableBuffers(std::vector<iovec>& buffers,
                                       uint64_t len) const {
  len = len > getReadableSize() ? getReadableSize() : len;
  if (len == 0u)
    return 0;

  uint64_t real_size = len;
  uint64_t cur_offset = m_offset % m_block_size;
  uint64_t cur_readable = m_cur->size - cur_offset;
  Node* cur = m_cur;
  struct iovec iov;

  while (len > 0) {
    if (cur_readable >= len) {
      // 当前这个节点已经可以放下了，直接放这里面就可以了
      iov.iov_base = cur->ptr + cur_offset;
      iov.iov_len = len;
      len = 0;
    } else {
      // 把当前节点放满，然后移动到下一个节点
      iov.iov_base = cur->ptr + cur_offset;
      iov.iov_len = cur_readable;
      len -= cur_readable;
      cur = cur->next;
      cur_offset = 0;
      cur_readable = cur->size;
    }
    buffers.emplace_back(std::move(iov));
  }
  return real_size;
}

/**
 * @brief 获取指定偏移位置数据的iovec数组
 * @param buffers 输出参数，存储iovec结构
 * @param len 要获取的数据长度
 * @param offset 数据偏移位置
 * @return 实际获取的数据长度
 * 
 * 从指定偏移位置开始，将数据分割为多个iovec结构
 */
uint64_t ByteArray::getReadableBuffers(std::vector<iovec>& buffers,
                                       uint64_t len, uint64_t offset) const {
  len = len > getReadableSize() ? getReadableSize() : len;
  if (len == 0u)
    return 0;

  uint64_t real_size = len;
  uint64_t cur_offset = offset % m_block_size;
  // 找到对应的当前节点
  Node* cur = m_root;
  size_t count = offset / m_block_size;
  while (count--) {
    cur = cur->next;
  }
  uint64_t cur_readable = cur->size - cur_offset;
  struct iovec iov;

  while (len > 0) {
    if (cur_readable >= len) {
      // 当前这个节点已经可以放下了，直接放这里面就可以了
      iov.iov_base = cur->ptr + cur_offset;
      iov.iov_len = len;
      len = 0;
    } else {
      // 把当前节点放满，然后移动到下一个节点
      iov.iov_base = cur->ptr + cur_offset;
      iov.iov_len = cur_readable;
      len -= cur_readable;
      cur = cur->next;
      cur_offset = 0;
      cur_readable = cur->size;
    }
    buffers.emplace_back(std::move(iov));
  }
  return real_size;
}

/**
 * @brief 获取可写数据的iovec数组
 * @param buffers 输出参数，存储iovec结构
 * @param len 要获取的数据长度
 * @return 实际获取的数据长度
 * 
 * 用于零拷贝写入，自动扩容并分割为多个iovec结构
 */
uint64_t ByteArray::getWriteableBuffers(std::vector<iovec>& buffers,
                                        uint64_t len) {
  if (len == 0)
    return 0;
  addCapacity(len);
  uint64_t size = len;
  size_t npos = m_offset % m_block_size;
  size_t ncap = m_cur->size - npos;
  struct iovec iov;
  Node* cur = m_cur;
  while (len > 0) {
    if (ncap >= len) {
      iov.iov_base = cur->ptr + npos;
      iov.iov_len = len;
      len = 0;
    } else {
      iov.iov_base = cur->ptr + npos;
      iov.iov_len = ncap;

      len -= ncap;
      cur = cur->next;
      npos = 0;
      ncap = m_cur->size - npos;
    }
    buffers.emplace_back(iov);
  }
  return size;
}

/**
 * @brief 获取总数据大小
 * @return 总数据大小
 */
size_t ByteArray::getSize() const {
  return m_size;
}

/**
 * @brief 增加容量，如果容量足够则不做任何操作
 * @param size 需要的最小容量
 * 
 * 自动计算需要添加的内存块数量并创建新的内存块
 */
void ByteArray::addCapacity(size_t size) {
  if (size == 0)
    return;

  size_t old_cap = getWriteableCapacity();
  if (old_cap >= size) {
    return;
  }

  int count = (size - old_cap + m_block_size - 1) / m_block_size;  // ceil

  Node* tmp = m_root;
  while (tmp->next) {
    tmp = tmp->next;
  }
  Node* head{nullptr};  // 用于调整m_cur
  while (count--) {
    tmp->next = new Node(m_block_size);
    if (head == nullptr) {
      head = tmp->next;
    }
    tmp = tmp->next;
    m_capacity += m_block_size;
  }

  if (old_cap == 0) {
    m_cur = head;
  }
}

/**
 * @brief 获取当前可写容量
 * @return 当前可写容量
 * 
 * 总容量减去当前的偏移量就是剩下可写的空间
 */
size_t ByteArray::getWriteableCapacity() const {
  return m_capacity - m_offset;  // 总容量减去当前的偏移量就是剩下可写的空间
}

}  // namespace East
