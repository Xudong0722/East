/*
 * @Author: Xudong0722 
 * @Date: 2025-03-21 15:20:02 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-12 23:33:20
 */

#include "Mutex.h"
#include <stdexcept>

namespace East {
Semaphore::Semaphore(uint32_t count) {
  if (sem_init(&m_semaphore, 0, count)) {
    throw std::logic_error("sem init error");
  }
}

Semaphore::~Semaphore() {
  sem_destroy(&m_semaphore);
}

void Semaphore::wait() {
  if (sem_wait(&m_semaphore)) {
    throw std::logic_error("sem wait error");
  }
}

void Semaphore::notify() {
  if (sem_post(&m_semaphore)) {
    throw std::logic_error("sem post error");
  }
}
}  // namespace East