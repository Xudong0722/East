/*
 * @Author: Xudong0722
 * @Date: 2025-03-03 14:42:59
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-04 00:29:09
 */

#pragma once
#include <thread>
#include <sstream>
#include <string>
#include <sys/syscall.h>
#include <unistd.h>

namespace East
{

    // get os thread id for troubleshooting
    pid_t getThreadId();

    uint32_t getFiberId();

} // namespace East
