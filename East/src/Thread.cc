/*
 * @Author: Xudong0722 
 * @Date: 2025-03-21 10:47:41 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-21 16:02:12
 */

#include "Thread.h"
#include "Elog.h"

namespace East {

static thread_local Thread* t_cur_thread = nullptr;
static thread_local std::string t_cur_thread_name = "UNKOWN";

static East::Logger::sptr g_logger = ELOG_NAME("system");  //TODO

Thread* Thread::GetThis() {
  return t_cur_thread;
}

std::string Thread::GetName() {
  return t_cur_thread_name;
}

void Thread::SetName(const std::string& name) {
  if (nullptr != t_cur_thread) {
    t_cur_thread->setName(name);
  }
  t_cur_thread_name = name;
}

void* Thread::run(void* arg) {
  Thread* t = static_cast<Thread*>(arg);
  if (nullptr == t)
    return {};

  t_cur_thread = t;
  t_cur_thread_name = t->getName();
  t->setId(East::getThreadId());
  pthread_setname_np(pthread_self(), t->getName().substr(0, 15).c_str());

  std::function<void()> cb{};
  cb.swap(t->m_cb);  //TODO, understand

  t->m_semaphore.notify();
  cb();
  return 0;
}

Thread::Thread(const std::string& name, std::function<void()> cb)
    : m_name(name), m_cb(cb) {
  if (name.empty()) {
    m_name = "UNKOWN";
  }
  int res = pthread_create(&m_thread, nullptr, &run, this);
  if (0 != res) {
    ELOG_ERROR(g_logger) << "pthread_create failed, ret value: " << res
                         << ", name: " << name;
    throw std::logic_error("pthread create failed");
  }
  m_semaphore.wait();
}

Thread::~Thread() {
  if (0 != m_thread) {
    pthread_detach(m_thread);
  }
}

pid_t Thread::getId() {
  return m_id;
}

void Thread::setId(pid_t id) {
  m_id = id;
}

const std::string& Thread::getName() {
  return m_name;
}

void Thread::setName(const std::string& name) {
  m_name = name;
}

void Thread::join() {
  if (0 != m_thread) {
    int res = pthread_join(m_thread, nullptr);
    if (0 != res) {
      ELOG_ERROR(g_logger) << "pthread_join failed, ret value: " << res
                           << ", name: " << m_name;
      throw std::logic_error("pthread join failed");
    }
    m_thread = 0;
  }
}

}  // namespace East