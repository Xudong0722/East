/*
 * @Author: Xudong0722 
 * @Date: 2025-06-10 21:10:00 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-10 22:03:00
 */

#include "Stream.h"

namespace East {

int Stream::readFixSize(void* buffer, size_t len) {
  size_t offset{0};
  size_t left = len;
  while (left > 0) {
    auto rt = read((char*)buffer + offset, left);
    if (rt <= 0) {
      return -1;
    }

    offset += rt;
    left -= rt;
  }
  return len;
}

int Stream::readFixSize(ByteArray::sptr ba, size_t len) {
  size_t left = len;
  while (left > 0) {
    auto rt = read(ba, left);
    if (rt <= 0) {
      return -1;
    }
    left -= rt;
  }
  return len;
}

int Stream::writeFixSize(const void* buffer, size_t len) {
  size_t offset{0};
  size_t left = len;
  while (left > 0) {
    auto rt = write((char*)buffer + offset, left);
    if (rt <= 0) {
      return -1;
    }

    offset += rt;
    left -= rt;
  }
  return len;
}

int Stream::writeFixSize(ByteArray::sptr ba, size_t len) {
  size_t left = len;
  while (left > 0) {
    auto rt = write(ba, left);
    if (rt <= 0) {
      return -1;
    }
    left -= rt;
  }
  return len;
}

}  // namespace East