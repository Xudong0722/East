/*
 * @Author: Xudong0722 
 * @Date: 2025-04-14 22:31:18 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-04-14 22:59:54
 */
#pragma once

#include <endian.h>
#include <stdint.h>

#define EAST_LITTLE_ENDIAN 1
#define EAST_BIG_ENDIAN 2

namespace East {

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define EAST_BYTE_ORDER EAST_LITTLE_ENDIAN
#elif __BYTE_ORDER == __BIG_ENDIAN
#define EAST_BYTE_ORDER EAST_BIG_ENDIAN
#endif

template <typename T>
auto byteswap(T value) {
  if constexpr (sizeof(T) == sizeof(uint64_t)) {
    return (T)__builtin_bswap64((uint64_t)value);
  } else if constexpr (sizeof(T) == sizeof(uint32_t)) {
    return (T)__builtin_bswap32((uint32_t)value);
  } else if constexpr (sizeof(T) == sizeof(uint16_t)) {
    return (T)__builtin_bswap16((uint16_t)value);
  }
}

#if EAST_BYTE_ORDER == EAST_BIG_ENDIAN
//大端机器，只处理针对大端的转换
template <typename T>
inline T byteswapOnLittleEndian(T value) {
  return value;
}
template <typename T>
inline T byteswapOnBigEndian(T value) {
  return byteswap(value);
}
#else
//小端机器，只处理针对小端的转换
template <typename T>
inline T byteswapOnLittleEndian(T value) {
  return byteswap(value);
}

template <typename T>
inline T byteswapOnBigEndian(T value) {
  return value;
}
#endif
}  // namespace East
