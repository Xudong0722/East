/*
 * @Author: Xudong0722
 * @Date: 2025-03-03 14:42:56
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-04 00:29:32
 */

#include "../include/util.h"

namespace East {

// int32_t getThreadId(){
// #if defined(__linux__) || defined(__unix__)
//     return static_cast<size_t>(pthread_self());
// #else
//     return std::hash<std::thread::id>{}(std::this_thread::get_id());
// #endif
// }

pid_t getThreadId() {
  return syscall(SYS_gettid);
}

uint32_t getFiberId() {
  return 0u;
}

}  // namespace East
