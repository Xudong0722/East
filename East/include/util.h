/*
 * @Author: Xudong0722
 * @Date: 2025-03-03 14:42:59
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-24 19:28:09
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
pid_t getThreadId();

uint32_t getFiberId();

void Backtrace(std::vector<std::string>& call_stack, int size, int skip = 1);

std::string BacktraceToStr(int size, int skip = 2,
                           const std::string& prefix = "");

}  // namespace East
