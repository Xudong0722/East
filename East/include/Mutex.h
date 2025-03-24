/*
 * @Author: Xudong0722 
 * @Date: 2025-03-21 15:16:50 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-21 18:21:13
 */

#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include "Noncopyable.h"

namespace East {

class Semaphore : private noncopymoveable {
 public:
  Semaphore(uint32_t count = 0);
  ~Semaphore();

  void wait();
  void notify();

 private:
  sem_t m_semaphore;
};

template <class T>
class ScopedLock : private noncopymoveable {
 public:
  ScopedLock(T& lock) : m_lock(lock) {
    m_lock.lock();
    m_locked = true;
  }

  ~ScopedLock() { unlock(); }
  void lock() {
    if (!m_locked) {
      m_lock.lock();
      m_locked = true;
    }
  }

  void unlock() {
    if (m_locked) {
      m_lock.unlock();
      m_locked = false;
    }
  }

 private:
  T& m_lock;
  bool m_locked;
};

class RWLock : private noncopymoveable {
 public:
  RWLock() { pthread_rwlock_init(&m_rw_lock, nullptr); }

  ~RWLock() { pthread_rwlock_destroy(&m_rw_lock); }

  //上读锁
  void rdlock() { pthread_rwlock_rdlock(&m_rw_lock); }

  //上写锁
  void wrlock() { pthread_rwlock_wrlock(&m_rw_lock); }

  //解锁
  void unlock() { pthread_rwlock_unlock(&m_rw_lock); }

 private:
  pthread_rwlock_t m_rw_lock;
};

template <class T>
class ReadScopedLock : private noncopymoveable {
 public:
  ReadScopedLock(T& lock) : m_lock(lock) {
    m_lock.rdlock();
    m_locked = true;
  }

  ~ReadScopedLock() { unlock(); }
  void lock() {
    if (!m_locked) {
      m_lock.rdlock();
      m_locked = true;
    }
  }

  void unlock() {
    if (m_locked) {
      m_lock.unlock();
      m_locked = false;
    }
  }

 private:
  T& m_lock;
  bool m_locked;
};

template <class T>
class WriteScopedLock : private noncopymoveable {
 public:
  WriteScopedLock(T& lock) : m_lock(lock) {
    m_lock.wrlock();
    m_locked = true;
  }

  ~WriteScopedLock() { unlock(); }
  void lock() {
    if (!m_locked) {
      m_lock.wrlock();
      m_locked = true;
    }
  }

  void unlock() {
    if (m_locked) {
      m_lock.unlock();
      m_locked = false;
    }
  }

 private:
  T& m_lock;
  bool m_locked;
};

class Mutex : private noncopymoveable {
 public:
  Mutex() { pthread_mutex_init(&m_mutex, nullptr); }

  ~Mutex() { pthread_mutex_destroy(&m_mutex); }

  void lock() { pthread_mutex_lock(&m_mutex); }

  void unlock() { pthread_mutex_unlock(&m_mutex); }

 private:
  pthread_mutex_t m_mutex;
};

using RLock = ReadScopedLock<RWLock>;
using WLock = WriteScopedLock<RWLock>;
using ScopedMutex = ScopedLock<Mutex>;

}  // namespace East
