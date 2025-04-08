/*
 * @Author: Xudong0722 
 * @Date: 2025-04-08 23:00:11 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-04-09 00:08:41
 */

#pragma once
#include <unistd.h>

namespace East {
bool is_hook_enable();
void set_hook_enable(bool enable);
}  // namespace East

extern "C" {
//declare the function pointer types
typedef unsigned int (*sleep_func)(unsigned int seconds);
extern sleep_func sleep_f;

typedef int (*usleep_func)(useconds_t usec);
extern usleep_func usleep_f;
}
