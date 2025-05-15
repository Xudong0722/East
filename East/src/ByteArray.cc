/*
 * @Author: Xudong0722 
 * @Date: 2025-05-12 22:12:30 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-05-12 23:10:51
 */

#include <cstring>
#include <string>
#include <stdexcept>
#include "ByteArray.h"
#include "Endian.h"

namespace East
{
ByteArray::Node::Node(size_t s) 
  : ptr(new char[s])
  , next(nullptr)
  , size(s) {
  
}

ByteArray::Node::Node() {

}

ByteArray::Node::~Node() {
  if(nullptr != ptr) {
    delete[] ptr;
  }
}

ByteArray::ByteArray(size_t base_size)
  : m_block_size(base_size)
  , m_offset(0)
  , m_size(base_size)
  , m_capacity(base_size)
  , m_endian(EAST_BIG_ENDIAN)
  , m_root(new Node(base_size))
  , m_cur(m_root) {
  
}

ByteArray::~ByteArray() {
  auto tmp = m_root;
  while(tmp) {
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
  if(m_endian != EAST_BYTE_ORDER) {
    val = byteswap(val);
  }
  write(&val, sizeof(val));
}

void ByteArray::writeFixUInt16(uint16_t val) {
  if(m_endian != EAST_BYTE_ORDER) {
    val = byteswap(val);
  }
  write(&val, sizeof(val));
}

void ByteArray::writeFixInt32(int32_t val) {
  if(m_endian != EAST_BYTE_ORDER) {
    val = byteswap(val);
  }  
  write(&val, sizeof(val));
}

void ByteArray::writeFixUInt32(uint32_t val) {
  if(m_endian != EAST_BYTE_ORDER) {
    val = byteswap(val);
  }
  write(&val, sizeof(val));
}

void ByteArray::writeFixInt64(int64_t val) {
  if(m_endian != EAST_BYTE_ORDER) {
    val = byteswap(val);
  }
  write(&val, sizeof(val));
}

void ByteArray::writeFixUInt64(uint64_t val) {
  if(m_endian != EAST_BYTE_ORDER) {
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
  while(val >= 0x80) {  //当前字节不是最后一个字节
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
  while(val >= 0x80) {  //当前字节不是最后一个字节
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

#define READ(type) \
  type tmp{};      \
  read(&tmp, sizeof(tmp));\
  if(m_endian == EAST_BYTE_ORDER){\
    return tmp;\
  }\
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
  for(int i = 0; i<32; i += 7) {
    uint8_t byte = readFixUInt8();
    if((byte & 0x80) != 0x80) {
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
  for(int i = 0; i<64; i += 7) {
    uint8_t byte = readFixUInt8();
    if((byte & 0x80) != 0x80) {
      res |= ((uint64_t)byte) << i;
      break;
    }else{
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
  if(len == 0) {
    return std::string{};
  }
  std::string str(len, ' ');
  read(&str[0], len);
  return str;
}

std::string ByteArray::readStringFix32() {
  uint32_t len = readFixUInt32();
  if(len == 0) {
    return std::string{};
  }
  std::string str(len, ' ');
  read(&str[0], len);
  return str;
}

std::string ByteArray::readStringFix64() {
  uint64_t len = readFixUInt64();
  if(len == 0) {
    return std::string{};
  }
  std::string str(len, ' ');
  read(&str[0], len);
  return str;
}

std::string ByteArray::readStringVarint() {
  uint64_t len = readInt64();
  if(len == 0) {
    return std::string{};
  }
  std::string str(len, ' ');
  read(&str[0], len);
  return str;
}

void ByteArray::clear() {
  m_offset = 0;
  m_size = 0;
  m_capacity = m_block_size;
  Node* tmp = m_root;
  while(tmp) {
    m_cur = tmp;
    tmp = tmp->next;
    delete m_cur;
  }
  m_cur = m_root;
  m_root->next = nullptr;
}

void ByteArray::write(const void* buf, size_t size) {
  if(size == 0) return;
  addCapacity(size);  //尝试扩容，如果足够，什么也不做
  
  size_t cur_offset = m_offset % m_block_size;  //当前内存块写到哪里了
  size_t cur_writeable = m_cur->size - cur_offset;  //当前内存块剩余的可写空间
  size_t buf_offset{0};

  while(size > 0) {
    if(cur_writeable >= size) {
      //当前内存块剩余空间足够
      memcpy(m_cur->ptr + cur_offset, buf + buf_offset, size);
      if(m_cur->size == (cur_offset + size)) {
        m_cur = m_cur->next;
      }
      m_offset += size;
      buf_offset += size;
      break;
    }
    memcpy(m_cur->ptr + cur_offset, buf + buf_offset, cur_writeable);  //先写这么多，剩下的放后面的节点中写
    m_offset += cur_writeable;
    size -= cur_writeable;
    buf_offset += cur_writeable;
    m_cur = m_cur->next;  //换到下一个内存块
    cur_offset = 0;
    cur_writeable = m_cur->size;
  }
  if(m_offset > m_size) {
    m_size = m_offset;
  }
}

void ByteArray::read(void* buf, size_t size) {
  //read一般从头开始读，所以使用前会改变m_offset的位置
  if(size > getReadableSize()) {
    throw std::out_of_range("read error");
  }

  size_t cur_offset = m_offset % m_block_size;
  size_t cur_readable = m_cur->size - cur_offset;
  size_t buf_offset{0};
  
  while(size > 0) {
    if(cur_readable >= size) {
      memcpy((char*)buf + buf_offset, m_cur->ptr + cur_offset, size);

      //如果正好读完，移动下当前内存指针
      if(cur_offset + size == m_cur->size) {
        m_cur = m_cur->next;
      }
      m_offset += size;
      buf_offset += size;
      size = 0;
    }else{
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

void ByteArray::read(void* buf, size_t size, size_t offset) {
  if(size > (m_size - offset)) {
    throw std::out_of_range("read error");
  }

  size_t cur_offset = offset % m_block_size;
  size_t cur_readable = m_cur->size - cur_offset;
  size_t buf_offset{0};
  
  while(size > 0) {
    if(cur_readable >= size) {
      memcpy((char*)buf + buf_offset, m_cur->ptr + cur_offset, size);

      //如果正好读完，移动下当前内存指针
      if(cur_offset + size == m_cur->size) {
        m_cur = m_cur->next;
      }
      offset += size;
      buf_offset += size;
      size = 0;
    }else{
      memcpy((char*)buf + buf_offset, m_cur->ptr + cur_offset, cur_readable);
      offset += cur_readable;
      buf_offset += cur_readable;
      size -= cur_readable;
      m_cur = m_cur->next;
      cur_offset = 0;
      cur_readable = m_cur->size;
    }
  }
}

size_t ByteArray::getOffset() const {
  return m_offset;
}

void ByteArray::setOffset(size_t offset) {
  if(offset > m_size) {
    throw std::out_of_range("set offset out of range");
  }

  m_offset = offset;
  m_cur = m_root;
  while(offset >= m_cur->size) {
    offset -= m_cur->size;
    m_cur = m_cur->next;
  }

  if(offset == m_cur->size){
    m_cur = m_cur->next;
  }
}

bool ByteArray::writeToFile(const std::string& file_name) const {

}

bool ByteArray::readFromFile(const std::string& file_name) {

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
    if(little_endian) {
        m_endian = EAST_LITTLE_ENDIAN;
    } else {
        m_endian = EAST_BIG_ENDIAN;
    }
}

std::string ByteArray::toString() const {

}

std::string ByteArray::toHexString() const {

}

uint64_t ByteArray::getReadableBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t offset) const {
  
}

uint64_t ByteArray::getWriteableBuffers(std::vector<iovec>& buffers, uint64_t len) {

}

size_t ByteArray::getSize() const {

}

void ByteArray::addCapacity(size_t size) {

}

size_t ByteArray::getCapacity() const {

}

} // namespace East
