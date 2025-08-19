/*
 * @Author: Xudong0722 
 * @Date: 2025-05-12 22:12:03 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-05-18 16:19:07
 */

#include <arpa/inet.h>
#include <stddef.h>
#include <stdint.h>
#include <memory>
#include <string>
#include <vector>

namespace East {

/**
 * @brief 字节数组类
 *
 * ByteArray类提供了高效的字节序列管理，支持动态扩容和多种数据类型的读写。
 * 使用链表结构管理内存块，避免频繁的内存重新分配。
 *
 * 主要特性：
 * - 支持多种数据类型的读写（整数、浮点数、字符串等）
 * - 自动内存管理和扩容
 * - 支持字节序转换
 * - 支持ZigZag编码的变长整数
 * - 支持文件读写操作
 * - 支持iovec操作，便于零拷贝传输
 */
class ByteArray {
 public:
  using sptr = std::shared_ptr<ByteArray>;

  /**
   * @brief 内存块节点结构
   *
   * Node结构表示ByteArray中的一个内存块，使用链表结构连接多个内存块。
   * 每个节点包含固定大小的内存空间，支持动态扩容。
   */
  struct Node {
    /**
     * @brief 构造函数，分配指定大小的内存
     * @param s 内存块大小（字节）
     */
    Node(size_t s);
    
    /**
     * @brief 默认构造函数
     */
    Node();
    
    /**
     * @brief 析构函数，释放分配的内存
     */
    ~Node();

    char* ptr{nullptr};    ///< 内存块指针
    Node* next{nullptr};   ///< 下一个内存块指针
    size_t size{0};        ///< 内存块大小
  };

  /**
   * @brief 构造函数
   * @param base_size 基础内存块大小，默认为4096字节
   */
  ByteArray(size_t base_size = 4096);
  
  /**
   * @brief 析构函数，释放所有内存块
   */
  ~ByteArray();

  // ZigZag编码相关函数
  /**
   * @brief 将32位有符号整数编码为ZigZag格式
   * @param n 要编码的32位整数
   * @return ZigZag编码后的32位无符号整数
   * 
   * ZigZag编码将负数映射到正数，便于变长编码
   */
  static uint32_t EncodeZigZagI32(int32_t n);
  
  /**
   * @brief 将64位有符号整数编码为ZigZag格式
   * @param n 要编码的64位整数
   * @return ZigZag编码后的64位无符号整数
   */
  static uint64_t EncodeZigZagI64(int64_t n);
  
  /**
   * @brief 将ZigZag编码的32位无符号整数解码为有符号整数
   * @param n ZigZag编码的32位无符号整数
   * @return 解码后的32位有符号整数
   */
  static int32_t DecodeZigZagI32(uint32_t n);
  
  /**
   * @brief 将ZigZag编码的64位无符号整数解码为有符号整数
   * @param n ZigZag编码的64位无符号整数
   * @return 解码后的64位有符号整数
   */
  static int64_t DecodeZigZagI64(uint64_t n);

  // 固定长度整数写入函数
  /**
   * @brief 写入8位有符号整数
   * @param val 要写入的值
   */
  void writeFixInt8(int8_t val);
  
  /**
   * @brief 写入8位无符号整数
   * @param val 要写入的值
   */
  void writeFixUInt8(uint8_t val);
  
  /**
   * @brief 写入16位有符号整数
   * @param val 要写入的值
   */
  void writeFixInt16(int16_t val);
  
  /**
   * @brief 写入16位无符号整数
   * @param val 要写入的值
   */
  void writeFixUInt16(uint16_t val);
  
  /**
   * @brief 写入32位有符号整数
   * @param val 要写入的值
   */
  void writeFixInt32(int32_t val);
  
  /**
   * @brief 写入32位无符号整数
   * @param val 要写入的值
   */
  void writeFixUInt32(uint32_t val);
  
  /**
   * @brief 写入64位有符号整数
   * @param val 要写入的值
   */
  void writeFixInt64(int64_t val);
  
  /**
   * @brief 写入64位无符号整数
   * @param val 要写入的值
   */
  void writeFixUInt64(uint64_t val);

  // 变长整数写入函数（可能压缩）
  /**
   * @brief 写入32位有符号整数（使用ZigZag编码和变长编码）
   * @param val 要写入的值
   */
  void writeInt32(int32_t val);
  
  /**
   * @brief 写入32位无符号整数（使用变长编码）
   * @param val 要写入的值
   */
  void writeUInt32(uint32_t val);
  
  /**
   * @brief 写入64位有符号整数（使用ZigZag编码和变长编码）
   * @param val 要写入的值
   */
  void writeInt64(int64_t val);
  
  /**
   * @brief 写入64位无符号整数（使用变长编码）
   * @param val 要写入的值
   */
  void writeUInt64(uint64_t val);

  /**
   * @brief 写入32位浮点数
   * @param val 要写入的值
   */
  void writeFloat(float val);
  
  /**
   * @brief 写入64位浮点数
   * @param val 要写入的值
   */
  void writeDouble(double val);

  // 字符串写入函数
  /**
   * @brief 写入字符串，长度使用16位整数表示
   * @param val 要写入的字符串
   */
  void writeStringFix16(const std::string& val);
  
  /**
   * @brief 写入字符串，长度使用32位整数表示
   * @param val 要写入的字符串
   */
  void writeStringFix32(const std::string& val);
  
  /**
   * @brief 写入字符串，长度使用64位整数表示
   * @param val 要写入的字符串
   */
  void writeStringFix64(const std::string& val);
  
  /**
   * @brief 写入字符串，长度使用变长整数表示
   * @param val 要写入的字符串
   */
  void writeStringVarint(const std::string& val);
  
  /**
   * @brief 写入字符串，不包含长度信息
   * @param val 要写入的字符串
   */
  void writeStringWithoutLen(const std::string& val);

  // 固定长度整数读取函数
  /**
   * @brief 读取8位有符号整数
   * @return 读取的8位有符号整数
   */
  int8_t readFixInt8();
  
  /**
   * @brief 读取8位无符号整数
   * @return 读取的8位无符号整数
   */
  uint8_t readFixUInt8();
  
  /**
   * @brief 读取16位有符号整数
   * @return 读取的16位有符号整数
   */
  int16_t readFixInt16();
  
  /**
   * @brief 读取16位无符号整数
   * @return 读取的16位无符号整数
   */
  uint16_t readFixUInt16();
  
  /**
   * @brief 读取32位有符号整数
   * @return 读取的32位有符号整数
   */
  int32_t readFixInt32();
  
  /**
   * @brief 读取32位无符号整数
   * @return 读取的32位无符号整数
   */
  uint32_t readFixUInt32();
  
  /**
   * @brief 读取64位有符号整数
   * @return 读取的64位有符号整数
   */
  int64_t readFixInt64();
  
  /**
   * @brief 读取64位无符号整数
   * @return 读取的64位无符号整数
   */
  uint64_t readFixUInt64();
  
  /**
   * @brief 读取32位有符号整数（变长编码）
   * @return 读取的32位有符号整数
   */
  int32_t readInt32();
  
  /**
   * @brief 读取32位无符号整数（变长编码）
   * @return 读取的32位无符号整数
   */
  uint32_t readUInt32();
  
  /**
   * @brief 读取64位有符号整数（变长编码）
   * @return 读取的64位有符号整数
   */
  int64_t readInt64();
  
  /**
   * @brief 读取64位无符号整数（变长编码）
   * @return 读取的64位无符号整数
   */
  uint64_t readUInt64();
  
  /**
   * @brief 读取32位浮点数
   * @return 读取的32位浮点数
   */
  float readFloat();
  
  /**
   * @brief 读取64位浮点数
   * @return 读取的64位浮点数
   */
  double readDouble();

  /**
   * @brief 读取字符串，长度使用16位整数表示
   * @return 读取的字符串
   */
  std::string readStringFix16();
  
  /**
   * @brief 读取字符串，长度使用32位整数表示
   * @return 读取的字符串
   */
  std::string readStringFix32();
  
  /**
   * @brief 读取字符串，长度使用64位整数表示
   * @return 读取的字符串
   */
  std::string readStringFix64();
  
  /**
   * @brief 读取字符串，长度使用变长整数表示
   * @return 读取的字符串
   */
  std::string readStringVarint();

  /**
   * @brief 清空所有数据，保留根节点
   */
  void clear();

  /**
   * @brief 写入原始字节数据
   * @param buf 要写入的数据缓冲区
   * @param size 数据大小
   */
  void write(const void* buf, size_t size);
  
  /**
   * @brief 从当前偏移位置读取数据
   * @param buf 输出缓冲区
   * @param size 要读取的数据大小
   */
  void read(void* buf, size_t size);
  
  /**
   * @brief 从指定偏移位置读取数据（不改变当前偏移）
   * @param buf 输出缓冲区
   * @param size 要读取的数据大小
   * @param offset 读取偏移位置
   */
  void read(void* buf, size_t size, size_t offset) const;

  /**
   * @brief 获取当前读写偏移位置
   * @return 当前偏移位置
   */
  size_t getOffset() const;
  
  /**
   * @brief 设置读写偏移位置
   * @param offset 新的偏移位置
   * @throw std::out_of_range 当偏移超出容量时抛出异常
   */
  void setOffset(size_t offset);

  /**
   * @brief 将数据写入文件
   * @param file_name 文件名
   * @return 写入成功返回true，失败返回false
   */
  bool writeToFile(const std::string& file_name) const;
  
  /**
   * @brief 从文件读取数据
   * @param file_name 文件名
   * @return 读取成功返回true，失败返回false
   */
  bool readFromFile(const std::string& file_name);

  /**
   * @brief 获取基础内存块大小
   * @return 基础内存块大小
   */
  size_t getBaseSize() const;

  /**
   * @brief 获取可读数据大小
   * @return 可读数据大小（总大小减去当前偏移）
   */
  size_t getReadableSize() const;

  /**
   * @brief 检查是否为小端字节序
   * @return 是小端字节序返回true，否则返回false
   */
  bool isLittleEndian() const;
  
  /**
   * @brief 设置字节序
   * @param little_endian true表示小端，false表示大端
   */
  void setLittleEndian(bool little_endian);

  /**
   * @brief 将数据转换为字符串
   * @return 数据的字符串表示
   */
  std::string toString() const;

  /**
   * @brief 将数据转换为十六进制字符串
   * @return 数据的十六进制字符串表示
   */
  std::string toHexString() const;

  /**
   * @brief 获取可读数据的iovec数组（从当前偏移开始）
   * @param buffers 输出参数，存储iovec结构
   * @param len 要获取的数据长度，默认为所有可读数据
   * @return 实际获取的数据长度
   */
  uint64_t getReadableBuffers(std::vector<iovec>& buffers,
                              uint64_t len = ~0ull) const;
  
  /**
   * @brief 获取指定偏移位置数据的iovec数组
   * @param buffers 输出参数，存储iovec结构
   * @param len 要获取的数据长度
   * @param offset 数据偏移位置
   * @return 实际获取的数据长度
   */
  uint64_t getReadableBuffers(std::vector<iovec>& buffers, uint64_t len,
                              uint64_t offset) const;

  /**
   * @brief 获取可写数据的iovec数组
   * @param buffers 输出参数，存储iovec结构
   * @param len 要获取的数据长度
   * @return 实际获取的数据长度
   */
  uint64_t getWriteableBuffers(std::vector<iovec>& buffers, uint64_t len);

  /**
   * @brief 获取总数据大小
   * @return 总数据大小
   */
  size_t getSize() const;

 private:
  /**
   * @brief 增加容量，如果容量足够则不做任何操作
   * @param size 需要的最小容量
   */
  void addCapacity(size_t size);

  /**
   * @brief 获取当前可写容量
   * @return 当前可写容量
   */
  size_t getWriteableCapacity() const;

 private:
  size_t m_block_size{0};  ///< 基础内存块大小
  size_t m_offset{0};      ///< 当前读写偏移位置
  size_t m_size{0};        ///< 实际承载数据的大小
  size_t m_capacity{0};    ///< 总容量
  int8_t m_endian{0};      ///< 字节序，默认大端（网络传输默认大端）
  Node* m_root;            ///< 内存块链表的头指针
  Node* m_cur;             ///< 当前内存块的指针
};

}  // namespace East
