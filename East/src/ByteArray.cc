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

ByteArray::Node::Node(size_t s) : ptr(new char[s]), next(nullptr), size(s) {}

ByteArray::Node::Node() {}

ByteArray::Node::~Node() {
  if (nullptr != ptr) {
    delete[] ptr;
  }
}

ByteArray::ByteArray(size_t base_size)
    : m_block_size(base_size),
      m_offset(0),
      m_size(base_size),
      m_capacity(base_size),
      m_endian(EAST_BIG_ENDIAN),
      m_root(new Node(base_size)),
      m_cur(m_root) {}

ByteArray::~ByteArray() {
  auto tmp = m_root;
  while (tmp) {
    m_cur = tmp;
    tmp = tmp->next;
    delete m_cur;
    m_cur = nullptr;
  }
}

uint32_t ByteArray::EncodeZigZagI32(int32_t n) {
  return (n << 1) ^ (n >> 31);
}

uint64_t ByteArray::EncodeZigZagI64(int64_t n) {
  return (n << 1) ^ (n >> 63);
}

int32_t ByteArray::DecodeZigZagI32(uint32_t n) {
  return (n >> 1) ^ -(n & 1);
}

int64_t ByteArray::DecodeZigZagI64(uint64_t n) {
  return (n >> 1) ^ -(n & 1);
}

//write
void ByteArray::writeFixInt8(int8_t val) {
  write(&val, sizeof(val));
}

void ByteArray::writeFixUInt8(uint8_t val) {
  write(&val, sizeof(val));
}

void ByteArray::writeFixInt16(int16_t val) {
  if (m_endian != EAST_BYTE_ORDER) {
    val = byteswap(val);
  }
  write(&val, sizeof(val));
}

void ByteArray::writeFixUInt16(uint16_t val) {
  if (m_endian != EAST_BYTE_ORDER) {
    val = byteswap(val);
  }
  write(&val, sizeof(val));
}

void ByteArray::writeFixInt32(int32_t val) {
  if (m_endian != EAST_BYTE_ORDER) {
    val = byteswap(val);
  }
  write(&val, sizeof(val));
}

void ByteArray::writeFixUInt32(uint32_t val) {
  if (m_endian != EAST_BYTE_ORDER) {
    val = byteswap(val);
  }
  write(&val, sizeof(val));
}

void ByteArray::writeFixInt64(int64_t val) {
  if (m_endian != EAST_BYTE_ORDER) {
    val = byteswap(val);
  }
  write(&val, sizeof(val));
}

void ByteArray::writeFixUInt64(uint64_t val) {
  if (m_endian != EAST_BYTE_ORDER) {
    val = byteswap(val);
  }
  write(&val, sizeof(val));
}

void ByteArray::writeInt32(int32_t val) {
  //可能为负，我们将其转换一下
  uint32_t zigzag = EncodeZigZagI32(val);
  writeUInt32(zigzag);
}

void ByteArray::writeUInt32(uint32_t val) {
  uint8_t buf[5];  //5*7 > 32
  uint8_t i = 0;
  while (val >= 0x80) {  //当前字节不是最后一个字节
    buf[i++] = (val & 0x7f) | 0x80;
    val >>= 7;
  }
  buf[i++] = val & 0x7f;
  write(buf, i);
}

void ByteArray::writeInt64(int64_t val) {
  uint64_t zigzag = EncodeZigZagI64(val);
  writeUInt64(zigzag);
}

void ByteArray::writeUInt64(uint64_t val) {
  uint8_t buf[10];  //10*7 > 64
  uint8_t i = 0;
  while (val >= 0x80) {  //当前字节不是最后一个字节
    buf[i++] = (val & 0x7f) | 0x80;
    val >>= 7;
  }
  buf[i++] = val & 0x7f;
  write(buf, i);
}

void ByteArray::writeFloat(float val) {
  uint32_t tmp{0};
  memcpy(&tmp, &val, sizeof(val));
  writeUInt32(tmp);
}

void ByteArray::writeDouble(double val) {
  uint64_t tmp{0};
  memcpy(&tmp, &val, sizeof(val));
  writeUInt64(tmp);
}

void ByteArray::writeStringFix16(const std::string& val) {
  writeFixUInt16(static_cast<uint16_t>(val.size()));
  write(val.c_str(), val.size());
}

void ByteArray::writeStringFix32(const std::string& val) {
  writeFixUInt32(static_cast<uint32_t>(val.size()));
  write(val.c_str(), val.size());
}

void ByteArray::writeStringFix64(const std::string& val) {
  writeFixUInt64(val.size());
  write(val.c_str(), val.size());
}

void ByteArray::writeStringVarint(const std::string& val) {
  //这里直接用uint64来处理
  writeUInt64(val.size());
  write(val.c_str(), val.size());
}

void ByteArray::writeStringWithoutLen(const std::string& val) {
  write(val.c_str(), val.size());
}

#define READ(type)                   \
  type tmp{};                        \
  read(&tmp, sizeof(tmp));           \
  if (m_endian == EAST_BYTE_ORDER) { \
    return tmp;                      \
  }                                  \
  return byteswap(tmp);

int8_t ByteArray::readFixInt8() {
  int8_t v{0};
  read(&v, sizeof(v));
  return v;
}

uint8_t ByteArray::readFixUInt8() {
  uint8_t v{0};
  read(&v, sizeof(v));
  return v;
}

int16_t ByteArray::readFixInt16() {
  READ(int16_t);
}

uint16_t ByteArray::readFixUInt16() {
  READ(uint16_t);
}

int32_t ByteArray::readFixInt32() {
  READ(int32_t);
}

uint32_t ByteArray::readFixUInt32() {
  READ(uint32_t);
}

int64_t ByteArray::readFixInt64() {
  READ(int64_t);
}

uint64_t ByteArray::readFixUInt64() {
  READ(uint64_t);
}

#undef READ

int32_t ByteArray::readInt32() {
  return DecodeZigZagI32(readUInt32());
}

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

int64_t ByteArray::readInt64() {
  return DecodeZigZagI64(readUInt64());
}

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

float ByteArray::readFloat() {
  uint32_t tmp = readUInt32();
  float val{0.0f};
  memcpy(&val, &tmp, sizeof(tmp));
  return val;
}

double ByteArray::readDouble() {
  uint64_t tmp = readUInt64();
  double val{0.0};
  memcpy(&val, &tmp, sizeof(tmp));
  return val;
}

std::string ByteArray::readStringFix16() {
  uint16_t len = readFixUInt16();
  if (len == 0) {
    return std::string{};
  }
  std::string str(len, ' ');
  read(&str[0], len);
  return str;
}

std::string ByteArray::readStringFix32() {
  uint32_t len = readFixUInt32();
  if (len == 0) {
    return std::string{};
  }
  std::string str(len, ' ');
  read(&str[0], len);
  return str;
}

std::string ByteArray::readStringFix64() {
  uint64_t len = readFixUInt64();
  if (len == 0) {
    return std::string{};
  }
  std::string str(len, ' ');
  read(&str[0], len);
  return str;
}

std::string ByteArray::readStringVarint() {
  uint64_t len = readInt64();
  if (len == 0) {
    return std::string{};
  }
  std::string str(len, ' ');
  read(&str[0], len);
  return str;
}

void ByteArray::clear() {
  //只保留根节点
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

void ByteArray::write(const void* buf, size_t size) {
  if (size == 0)
    return;
  addCapacity(size);  //尝试扩容，如果足够，什么也不做

  size_t cur_offset = m_offset % m_block_size;  //当前内存块写到哪里了
  size_t cur_writeable = m_cur->size - cur_offset;  //当前内存块剩余的可写空间
  size_t buf_offset{0};

  while (size > 0) {
    if (cur_writeable >= size) {
      //当前内存块剩余空间足够
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
           cur_writeable);  //先写这么多，剩下的放后面的节点中写
    m_offset += cur_writeable;
    size -= cur_writeable;
    buf_offset += cur_writeable;
    m_cur = m_cur->next;  //换到下一个内存块
    cur_offset = 0;
    cur_writeable = m_cur->size;
  }
  if (m_offset > m_size) {
    m_size = m_offset;
  }
}

void ByteArray::read(void* buf, size_t size) {
  //read一般从头开始读，所以使用前会改变m_offset的位置
  if (size > getReadableSize()) {
    throw std::out_of_range("read error");
  }

  size_t cur_offset = m_offset % m_block_size;
  size_t cur_readable = m_cur->size - cur_offset;
  size_t buf_offset{0};

  while (size > 0) {
    if (cur_readable >= size) {
      memcpy((char*)buf + buf_offset, m_cur->ptr + cur_offset, size);

      //如果正好读完，移动下当前内存指针
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

      //如果正好读完，移动下当前内存指针
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

size_t ByteArray::getOffset() const {
  return m_offset;
}

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

size_t ByteArray::getBaseSize() const {
  return m_block_size;
}

size_t ByteArray::getReadableSize() const {
  return m_size - m_offset;
}

bool ByteArray::isLittleEndian() const {
  return m_endian;
}

void ByteArray::setLittleEndian(bool little_endian) {
  if (little_endian) {
    m_endian = EAST_LITTLE_ENDIAN;
  } else {
    m_endian = EAST_BIG_ENDIAN;
  }
}

std::string ByteArray::toString() const {
  std::string res;
  res.resize(getReadableSize());

  if (res.empty())
    return res;
  read(&res[0], res.size(), m_offset);
  return res;
}

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
      //当前这个节点已经可以放下了，直接放这里面就可以了
      iov.iov_base = cur->ptr + cur_offset;
      iov.iov_len = len;
      len = 0;
    } else {
      //把当前节点放满，然后移动到下一个节点
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

uint64_t ByteArray::getReadableBuffers(std::vector<iovec>& buffers,
                                       uint64_t len, uint64_t offset) const {
  len = len > getReadableSize() ? getReadableSize() : len;
  if (len == 0u)
    return 0;

  uint64_t real_size = len;
  uint64_t cur_offset = offset % m_block_size;
  //找到对应的当前节点
  Node* cur = m_root;
  size_t count = offset / m_block_size;
  while (count--) {
    cur = cur->next;
  }
  uint64_t cur_readable = cur->size - cur_offset;
  struct iovec iov;

  while (len > 0) {
    if (cur_readable >= len) {
      //当前这个节点已经可以放下了，直接放这里面就可以了
      iov.iov_base = cur->ptr + cur_offset;
      iov.iov_len = len;
      len = 0;
    } else {
      //把当前节点放满，然后移动到下一个节点
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

size_t ByteArray::getSize() const {
  return m_size;
}

void ByteArray::addCapacity(size_t size) {
  if (size == 0)
    return;

  size_t old_cap = getWriteableCapacity();
  if (old_cap >= size) {
    return;
  }

  int count = (size - old_cap + m_block_size - 1) / m_block_size;  //ceil

  Node* tmp = m_root;
  while (tmp->next) {
    tmp = tmp->next;
  }
  Node* head{nullptr};  //用于调整m_cur
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

size_t ByteArray::getWriteableCapacity() const {
  return m_capacity - m_offset;  //总容量减去当前的偏移量就是剩下可写的空间
}

}  // namespace East
