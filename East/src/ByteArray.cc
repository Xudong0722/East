/*
 * @Author: Xudong0722 
 * @Date: 2025-05-12 22:12:30 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-05-12 23:10:51
 */

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

}

void ByteArray::writeDouble(double val) {

}

void ByteArray::writeStringFix16(const std::string& val) {

}

void ByteArray::writeStringFix32(const std::string& val) {

}

void ByteArray::writeStringFix64(const std::string& val) {

}

void ByteArray::writeStringVarint(const std::string& val) {

}

void ByteArray::writeStringWithoutLen(const std::string& val) {

}

int8_t ByteArray::readFixInt8() {

}

uint8_t ByteArray::readFixUInt8() {

}

int16_t ByteArray::readFixInt16() {

}

uint16_t ByteArray::readFixUInt16() {

}

int32_t ByteArray::readFixInt32() {

}

uint32_t ByteArray::readFixUInt32() {

}

int64_t ByteArray::readFixInt64() {

}

uint64_t ByteArray::readFixUInt64() {

}

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

int64_t readInt64() {

}

uint64_t readUInt64() {

}

float ByteArray::readFloat() {

}

double ByteArray::readDouble() {

}

std::string ByteArray::readStringFix16() {

}

std::string ByteArray::readStringFix32() {

}

std::string ByteArray::readStringFix64() {

}

std::string ByteArray::readStringVarint() {

}

void ByteArray::clear() {

}

void ByteArray::write(const void* buf, size_t size) {

}

void ByteArray::read(void* buf, size_t size) {

}

void ByteArray::read(void* buf, size_t size, size_t offset) {

}

size_t ByteArray::getOffset() const {

}

void ByteArray::setOffset(size_t offset) {

}

bool ByteArray::writeToFile(const std::string& file_name) const {

}

bool ByteArray::readFromFile(const std::string& file_name) {

}

size_t ByteArray::getBaseSize() const {

}

size_t ByteArray::getReadableSize() const {

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
