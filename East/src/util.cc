/*
 * @Author: Xudong0722
 * @Date: 2025-03-03 14:42:56
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-04-06 16:38:35
 */

#include <execinfo.h>
#include <sys/time.h>
#include <sstream>

#include "Elog.h"
#include "Fiber.h"
#include "util.h"

namespace East {

Logger::sptr g_logger = ELOG_NAME("system");

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
}  // namespace East
