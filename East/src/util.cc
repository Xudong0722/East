/*
 * @Author: Xudong0722
 * @Date: 2025-03-03 14:42:56
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-08-21 00:31:31
 */

#include <execinfo.h>
#include <sys/time.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>

#include "Elog.h"
#include "Fiber.h"
#include "util.h"

namespace East {

static Logger::sptr g_logger = ELOG_NAME("system");

// int32_t getThreadId(){
// #if defined(__linux__) || defined(__unix__)
//     return static_cast<size_t>(pthread_self());
// #else
//     return std::hash<std::thread::id>{}(std::this_thread::get_id());
// #endif
// }

pid_t GetThreadId() {
  return syscall(SYS_gettid);
}

uint32_t GetFiberId() {
  return static_cast<uint32_t>(Fiber::GetFiberId());
}

void Backtrace(std::vector<std::string>& call_stack, int size, int skip) {
  void** buffer = (void**)malloc(sizeof(void*) * size);

  int nptrs = ::backtrace(buffer, size);

  char** strings = ::backtrace_symbols(buffer, nptrs);
  if (nullptr == strings) {
    ELOG_ERROR(g_logger) << "backtrace_symbols error";
    free(buffer);
    return;
  }

  call_stack.clear();
  for (int i = skip; i < nptrs; ++i) {
    call_stack.emplace_back(strings[i]);
  }

  free(strings);
  free(buffer);
}

std::string BacktraceToStr(int size, int skip, const std::string& prefix) {
  std::vector<std::string> call_stack{};
  Backtrace(call_stack, size, skip);

  std::stringstream ss;
  for (const auto& s : call_stack) {
    ss << prefix << s << '\n';
  }
  return ss.str();
}

//get current time(ms),
uint64_t GetCurrentTimeInMs() {
  timespec ts{};
  ::clock_gettime(CLOCK_REALTIME, &ts);
  return static_cast<uint64_t>(ts.tv_sec) * 1000 + ts.tv_nsec / 1000000;
}

//get current time(us)
uint64_t GetCurrentTimeInUs() {
  timespec ts{};
  ::clock_gettime(CLOCK_REALTIME, &ts);
  return static_cast<uint64_t>(ts.tv_sec) * 1000000 | ts.tv_nsec / 1000;
}

std::string TimeSinceEpochToString(uint64_t tm) {
  using namespace std::chrono;
  std::time_t t = static_cast<std::time_t>(tm);
  std::tm tm_local;
  localtime_r(&t, &tm_local);
  std::stringstream ss;
  ss << std::put_time(&tm_local, "%Y-%m-%d %H:%M:%S");
  return ss.str();
}

void FSUtil::ListAllFile(std::vector<std::string>& files, const std::string& path, const std::string& suffix){
  if(access(path.c_str(), 0) != 0) {
    return ;
  }

  auto dir = opendir(path.c_str());
  if(nullptr == dir) return ;

  struct dirent* dp = nullptr;
  while((dp = readdir(dir)) && dp != nullptr) {
    if(dp->d_type == DT_DIR) {
      if(!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) continue;
      ListAllFile(files, path + "/" + dp->d_name, suffix);
    }else if(dp->d_type == DT_REG){
      std::string filename(dp->d_name);
      if(suffix.empty()) {
        files.push_back(path + "/" + filename);
      }else{
        //simple hanle
        std::string ext;
        size_t pos = filename.find_last_of('.');
        if(pos != std::string::npos)
          ext = filename.substr(pos+1);
        if(ext == suffix) {
          files.push_back(path + "/" + filename);
        }
      }
    }
  }

  closedir(dir);
}
}  // namespace East
