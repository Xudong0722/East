/*
 * @Author: Xudong0722
 * @Date: 2025-03-03 14:42:59
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-22 13:57:12
 */

#pragma once
#include <sys/syscall.h>
#include <unistd.h>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace East {

// get os thread id for troubleshooting
pid_t GetThreadId();

uint32_t GetFiberId();

void Backtrace(std::vector<std::string>& call_stack, int size, int skip = 1);

std::string BacktraceToStr(int size, int skip = 2,
                           const std::string& prefix = "");

//get current time(ms)
uint64_t GetCurrentTimeInMs();

//get current time(us)
uint64_t GetCurrentTimeInUs();

template<typename T>
auto Enum2Utype(T e) -> std::underlying_type_t<T> {
  return static_cast<std::underlying_type_t<T>>(e);
}

}  // namespace East
