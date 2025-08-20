/*
 * @Author: Xudong0722 
 * @Date: 2025-08-20 21:03:23 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-08-20 21:10:43
 */

#pragma once

#include "singleton.h"
#include <functional>

namespace East
{
//存放进程信息
struct ProcessInfo {
  pid_t parent_id;    //父进程id
  pid_t main_id;      //主进程id
  uint64_t parent_start_time{0u};  //父进程启动的时间
  uint64_t main_start_time{0u}; //主进程启动的时间
  uint32_t restart_count{0u};  //主进程重启的次数
};

using ProcessInfoMgr = East::Singleton<ProcessInfo>;

int start_daemon(int argc, char** argv, std::function<int(int argc, char** argv)> main_cb, bool is_daemon);

} // namespace East
