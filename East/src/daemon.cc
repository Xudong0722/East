/*
 * @Author: Xudong0722 
 * @Date: 2025-08-20 21:09:45 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-08-21 00:35:26
 */

#include "daemon.h"
#include "Elog.h"
#include "Config.h"
#include "util.h"
#include <chrono>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

namespace East
{

static East::Logger::sptr g_logger = ELOG_NAME("system");
static East::ConfigVar<uint32_t>::sptr g_daemon_restart_interval
  = East::Config::Lookup("daemon.restart_interval", (uint32_t)5, "daemon restart interval");

std::string ProcessInfo::to_str() const {
    std::stringstream ss;
    ss << "[ProcessInfo] parent_id: " << parent_id
       << ", main_id: " << main_id
       << ", parent_start_time: " << TimeSinceEpochToString(parent_start_time)
       << ", main_start_time: " << TimeSinceEpochToString(main_start_time)
       << ", restart_count: " << restart_count;
    return ss.str();
}

static int real_start(int argc, char** argv, std::function<int(int argc, char** argv)> main_cb) {
    return main_cb(argc, argv);
}

static int real_daemon(int argc, char** argv, std::function<int(int argc, char** argv)> main_cb) {
    using namespace std::chrono;
    ProcessInfoMgr::GetInst()->parent_id = getpid();
    ProcessInfoMgr::GetInst()->parent_start_time = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();

    while(true) {
        pid_t pid = fork();
        //Two process run from here.
        if(pid == 0) {
            //子进程中返回0
            ProcessInfoMgr::GetInst()->main_id = getpid();
            ProcessInfoMgr::GetInst()->main_start_time = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
            ELOG_INFO(g_logger) << "child process start, pid: " << getpid();
            return real_start(argc, argv, main_cb);
        }else if(pid < 0) {
            //错误处理
            ELOG_ERROR(g_logger) << "fork failed, result: " << pid << " errno: " << errno << "errstr: " << strerror(errno);
            return -1;
        }else{
            //父进程中返回
            int status{0};
            waitpid(pid, &status, 0);
            if(status) {
                ELOG_ERROR(g_logger) << "child process terminated, pid: " << pid 
                  << " status: " << status;
            }else {
                //子进程正常退出
                ELOG_INFO(g_logger) << "child process finished, pid: " << pid;
                break;
            }
            ProcessInfoMgr::GetInst()->restart_count += 1;
            sleep(g_daemon_restart_interval->getValue());  //等待资源释放
        }
    }
    return 0;
}

int start_daemon(int argc, char** argv, std::function<int(int argc, char** argv)> main_cb, bool is_daemon) {
    if(!is_daemon) {
        return real_start(argc, argv, main_cb);
    }
    return real_daemon(argc, argv, main_cb);
}
} // namespace East
