/*
 * @Author: Xudong0722 
 * @Date: 2025-03-21 10:47:27 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-21 16:00:59
 */

#pragma once
#include <pthread.h>
#include <functional>
#include <memory>
#include <string>
#include "Mutex.h"
#include "Noncopyable.h"

namespace East {

class Thread : private noncopymoveable {
 public:
  using sptr = std::shared_ptr<Thread>;
  Thread(const std::string& name, std::function<void()> cb);
  ~Thread();

  pid_t getId();
  void setId(pid_t id);
  const std::string& getName();
  void setName(const std::string& name);
  void join();

  static Thread* GetThis();
  static std::string GetName();
  static void SetName(const std::string& name);

 private:
  static void* run(void* arg);

 private:
  pid_t m_id{-1};
  pthread_t m_thread{0};
  std::string m_name;
  std::function<void()> m_cb;
  Semaphore m_semaphore;
};
}  // namespace East