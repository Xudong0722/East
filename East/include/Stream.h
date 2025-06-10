/*
 * @Author: Xudong0722 
 * @Date: 2025-06-10 21:05:07 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-10 21:24:24
 */

#pragma once
#include <memory>
#include "ByteArray.h"

namespace East {

class Stream {
public:
  using sptr = std::shared_ptr<Stream>;
  virtual ~Stream() {}

  virtual int read(void* buffer, size_t len) = 0;
  virtual int read(ByteArray::sptr ba, size_t len) = 0;

  virtual int readFixSize(void* buffer, size_t len);
  virtual int readFixSize(ByteArray::sptr ba, size_t len);

  virtual int write(const void* buffer, size_t len) = 0;
  virtual int write(ByteArray::sptr ba, size_t len) = 0;

  virtual int writeFixSize(const void* buffer, size_t len);
  virtual int writeFixSize(ByteArray::sptr ba, size_t len);

  virtual void close() = 0;
};
} // namespace East
