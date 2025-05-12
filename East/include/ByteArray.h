/*
 * @Author: Xudong0722 
 * @Date: 2025-05-12 22:12:03 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-05-12 23:11:10
 */

#include <stdint.h>
#include <memory>
#include <string>
#include <vector>
#include <arpa/inet.h>
#include <stddef.h>

namespace East
{
class ByteArray
{
public:
  using sptr = std::shared_ptr<ByteArray>;

  struct Node {
    Node(size_t s);
    Node();
    ~Node();

    char* ptr{nullptr};
    Node* next{nullptr};
    size_t size{0};
  };

  ByteArray(size_t base_size = 4096);
  ~ByteArray();

  //write
  void writeFixInt8(int8_t val);
  void writeFixUInt8(uint8_t val);
  void writeFixInt16(int16_t val);
  void writeFixUInt16(uint16_t val);
  void writeFixInt32(int32_t val);
  void writeFixUInt32(uint32_t val);
  void writeFixInt64(int64_t val);
  void writeFixUInt64(uint64_t val);

  //maybe suppress
  void writeInt32(int32_t val);
  void writeUInt32(uint32_t val);
  void writeInt64(int64_t val);
  void writeUInt64(uint64_t val);

  void writeFloat(float val);
  void writeDouble(double val);

  //len: int16
  void writeStringFix16(const std::string& val);
  //len: int32
  void writeStringFix32(const std::string& val);
  //len: int64
  void writeStringFix64(const std::string& val);
  //len: varint
  void writeStringVarint(const std::string& val);
  //without len
  void writeStringWithoutLen(const std::string& val);

  //read
  int8_t readFixInt8();
  uint8_t readFixUInt8();
  int16_t readFixInt16();
  uint16_t readFixUInt16();
  int32_t readFixInt32();
  uint32_t readFixUInt32(); 
  int64_t readFixInt64();
  uint64_t readFixUInt64();
  
  float readFloat();
  double readDouble();

  std::string readStringFix16();
  std::string readStringFix32();
  std::string readStringFix64();
  std::string readStringVarint();

  void clear();

  void write(const void* buf, size_t size);
  void read(void* buf, size_t size);
  void read(void* buf, size_t size, size_t offset);

  size_t getOffset() const;
  void setOffset(size_t offset);

  bool writeToFile(const std::string& file_name) const;
  bool readFromFile(const std::string& file_name);

  size_t getBaseSize() const;  //返回内存块的大小

  size_t getReadableSize() const;  //返回可读的数据大小

  bool isLittleEndian() const;  //是否是小端字节序
  void setLittleEndian(bool little_endian);  //设置字节序

  std::string toString() const;

  std::string toHexString() const;  //十六进制字符串

  uint64_t getReadableBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t offset) const;

  uint64_t getWriteableBuffers(std::vector<iovec>& buffers, uint64_t len);

  size_t getSize() const;

private:
  void addCapacity(size_t size);  //扩容，如果容量足够，什么也不做

  size_t getCapacity() const; //返回当前的可写入容量

private:
  size_t m_block_size{0};  //内存块的大小
  size_t m_offset{0};     //当前读写的偏移量
  size_t m_size{0};      //当前数据的大小
  size_t m_capacity{0};  //当前总容量
  int8_t m_endian{0}; //字节序, 默认大端(网络传输默认大端)
  Node* m_root;  //内存块链表的头指针
  Node* m_cur;  //当前内存块的指针
};
} // namespace East
