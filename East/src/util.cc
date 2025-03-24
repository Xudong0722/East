/*
 * @Author: Xudong0722
 * @Date: 2025-03-03 14:42:56
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-24 21:09:09
 */

#include "util.h"
#include <execinfo.h>
#include <sstream>
#include "Elog.h"

namespace East {

Logger::sptr g_logger = ELOG_NAME("system");

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

}  // namespace East
