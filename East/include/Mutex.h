/*
 * @Author: Xudong0722 
 * @Date: 2025-03-21 15:16:50 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-12 23:30:42
 */
#pragma once
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <atomic>
#include "Noncopyable.h"

namespace East {

/**
 * @brief 信号量类
 *
 * Semaphore类封装了POSIX信号量，提供线程同步机制。
 * 支持多个线程等待和通知操作，常用于生产者-消费者模式。
 *
 * 主要功能：
 * - 线程等待（wait）：当信号量计数为0时阻塞
 * - 线程通知（notify）：增加信号量计数，唤醒等待的线程
 * - 自动资源管理：析构时自动清理信号量资源
 */
class Semaphore : private noncopymoveable {
 public:
  /**
   * @brief 构造函数
   * @param count 初始信号量计数，默认为0
   */
  Semaphore(uint32_t count = 0);
  
  /**
   * @brief 析构函数，自动清理信号量资源
   */
  ~Semaphore();

  /**
   * @brief 等待信号量
   * 
   * 如果信号量计数大于0，则减少计数并返回；
   * 如果计数为0，则阻塞当前线程直到被其他线程通知
   */
  void wait();
  
  /**
   * @brief 通知信号量
   * 
   * 增加信号量计数，唤醒一个等待的线程
   */
  void notify();

 private:
  sem_t m_semaphore;  ///< POSIX信号量句柄
};

/**
 * @brief 作用域锁模板类
 *
 * ScopedLock是一个RAII风格的锁包装器，自动管理锁的获取和释放。
 * 在构造时自动加锁，在析构时自动解锁，避免忘记解锁的问题。
 *
 * @tparam T 锁类型，必须支持lock()和unlock()方法
 */
template <class T>
class ScopedLock : private noncopymoveable {
 public:
  /**
   * @brief 构造函数，自动获取锁
   * @param lock 要管理的锁对象引用
   */
  ScopedLock(T& lock) : m_lock(lock) {
    m_lock.lock();
    m_locked = true;
  }

  /**
   * @brief 析构函数，自动释放锁
   */
  ~ScopedLock() { unlock(); }
  
  /**
   * @brief 手动获取锁
   * 
   * 如果当前未锁定，则获取锁；如果已锁定，则不执行任何操作
   */
  void lock() {
    if (!m_locked) {
      m_lock.lock();
      m_locked = true;
    }
  }

  /**
   * @brief 手动释放锁
   * 
   * 如果当前已锁定，则释放锁；如果未锁定，则不执行任何操作
   */
  void unlock() {
    if (m_locked) {
      m_lock.unlock();
      m_locked = false;
    }
  }

 private:
  T& m_lock;           ///< 锁对象引用
  bool m_locked;       ///< 当前锁定状态
};

/**
 * @brief 读作用域锁模板类
 *
 * ReadScopedLock专门用于读写锁的读锁管理，在构造时自动获取读锁。
 * 支持多个线程同时持有读锁，提高并发性能。
 *
 * @tparam T 读写锁类型，必须支持rdlock()和unlock()方法
 */
template <class T>
class ReadScopedLock : private noncopymoveable {
 public:
  /**
   * @brief 构造函数，自动获取读锁
   * @param lock 要管理的读写锁对象引用
   */
  ReadScopedLock(T& lock) : m_lock(lock) { this->lock(); }

  /**
   * @brief 析构函数，自动释放读锁
   */
  ~ReadScopedLock() { unlock(); }
  
  /**
   * @brief 手动获取读锁
   * 
   * 如果当前未锁定，则获取读锁；如果已锁定，则不执行任何操作
   */
  void lock() {
    if (!m_locked) {
      m_lock.rdlock();
      m_locked = true;
    }
  }

  /**
   * @brief 手动释放读锁
   * 
   * 如果当前已锁定，则释放读锁；如果未锁定，则不执行任何操作
   */
  void unlock() {
    if (m_locked) {
      m_lock.unlock();
      m_locked = false;
    }
  }

 private:
  T& m_lock;                    ///< 读写锁对象引用
  bool m_locked{false};         ///< 当前锁定状态
};

/**
 * @brief 写作用域锁模板类
 *
 * WriteScopedLock专门用于读写锁的写锁管理，在构造时自动获取写锁。
 * 写锁是独占锁，同一时间只能有一个线程持有写锁。
 *
 * @tparam T 读写锁类型，必须支持wrlock()和unlock()方法
 */
template <class T>
class WriteScopedLock : private noncopymoveable {
 public:
  /**
   * @brief 构造函数，自动获取写锁
   * @param lock 要管理的读写锁对象引用
   */
  WriteScopedLock(T& lock) : m_lock(lock) { this->lock(); }

  /**
   * @brief 析构函数，自动释放写锁
   */
  ~WriteScopedLock() { unlock(); }
  
  /**
   * @brief 手动获取写锁
   * 
   * 如果当前未锁定，则获取写锁；如果已锁定，则不执行任何操作
   */
  void lock() {
    if (!m_locked) {
      m_lock.wrlock();
      m_locked = true;
    }
  }

  /**
   * @brief 手动释放写锁
   * 
   * 如果当前已锁定，则释放写锁；如果未锁定，则不执行任何操作
   */
  void unlock() {
    if (m_locked) {
      m_lock.unlock();
      m_locked = false;
    }
  }

 private:
  T& m_lock;                    ///< 读写锁对象引用
  bool m_locked{false};         ///< 当前锁定状态
};

/**
 * @brief 读写锁类
 *
 * RWLock类封装了POSIX读写锁，支持多个线程同时读取，但写操作是独占的。
 * 适用于读多写少的场景，提高并发性能。
 *
 * 主要特性：
 * - 多个线程可以同时持有读锁
 * - 写锁是独占的，与读锁和写锁都互斥
 * - 提供RAII风格的锁管理类
 */
class RWLock : private noncopymoveable {
 public:
  using RLockGuard = ReadScopedLock<RWLock>;   ///< 读锁RAII包装器类型
  using WLockGuard = WriteScopedLock<RWLock>;  ///< 写锁RAII包装器类型
  
  /**
   * @brief 构造函数，初始化读写锁
   */
  RWLock() { pthread_rwlock_init(&m_rw_lock, nullptr); }

  /**
   * @brief 析构函数，销毁读写锁
   */
  ~RWLock() { pthread_rwlock_destroy(&m_rw_lock); }

  /**
   * @brief 获取读锁
   * 
   * 如果当前没有写锁，则获取读锁；否则阻塞直到写锁被释放
   */
  void rdlock() { pthread_rwlock_rdlock(&m_rw_lock); }

  /**
   * @brief 获取写锁
   * 
   * 如果当前没有读锁和写锁，则获取写锁；否则阻塞直到所有锁被释放
   */
  void wrlock() { pthread_rwlock_wrlock(&m_rw_lock); }

  /**
   * @brief 释放锁
   * 
   * 释放当前持有的读锁或写锁
   */
  void unlock() { pthread_rwlock_unlock(&m_rw_lock); }

 private:
  pthread_rwlock_t m_rw_lock;  ///< POSIX读写锁句柄
};

/**
 * @brief 互斥锁类
 *
 * Mutex类封装了POSIX互斥锁，提供基本的线程同步机制。
 * 同一时间只能有一个线程持有锁，适用于保护共享资源的访问。
 *
 * 主要特性：
 * - 独占锁：同一时间只能有一个线程持有
 * - 自动死锁检测
 * - 提供RAII风格的锁管理类
 */
class Mutex : private noncopymoveable {
 public:
  using LockGuard = ScopedLock<Mutex>;  ///< 锁RAII包装器类型
  
  /**
   * @brief 构造函数，初始化互斥锁
   */
  Mutex() { pthread_mutex_init(&m_mutex, nullptr); }

  /**
   * @brief 析构函数，销毁互斥锁
   */
  ~Mutex() { pthread_mutex_destroy(&m_mutex); }

  /**
   * @brief 获取锁
   * 
   * 如果锁未被持有，则获取锁；否则阻塞直到锁被释放
   */
  void lock() { pthread_mutex_lock(&m_mutex); }

  /**
   * @brief 释放锁
   * 
   * 释放当前持有的锁
   */
  void unlock() { pthread_mutex_unlock(&m_mutex); }

 private:
  pthread_mutex_t m_mutex;  ///< POSIX互斥锁句柄
};

/**
 * @brief 自旋锁类
 *
 * SpinLock类封装了POSIX自旋锁，适用于锁竞争不激烈且等待时间很短的场景。
 * 与互斥锁不同，自旋锁不会让线程进入睡眠状态，而是忙等待。
 *
 * 主要特性：
 * - 忙等待：不会让线程进入睡眠状态
 * - 适用于短时间等待的场景
 * - 避免线程切换开销
 * - 提供RAII风格的锁管理类
 */
class SpinLock : private noncopymoveable {
 public:
  using LockGuard = ScopedLock<SpinLock>;  ///< 锁RAII包装器类型
  
  /**
   * @brief 构造函数，初始化自旋锁
   */
  SpinLock() { pthread_spin_init(&m_spin_lock, 0); }
  
  /**
   * @brief 析构函数，销毁自旋锁
   */
  ~SpinLock() { pthread_spin_destroy(&m_spin_lock); }

  /**
   * @brief 获取自旋锁
   * 
   * 如果锁未被持有，则获取锁；否则忙等待直到锁被释放
   */
  void lock() { pthread_spin_lock(&m_spin_lock); }

  /**
   * @brief 释放自旋锁
   * 
   * 释放当前持有的自旋锁
   */
  void unlock() { pthread_spin_unlock(&m_spin_lock); }

 private:
  pthread_spinlock_t m_spin_lock;  ///< POSIX自旋锁句柄
};

/**
 * @brief 比较并交换锁类
 *
 * CASLock类使用原子操作实现锁机制，基于compare-and-swap操作。
 * 适用于轻量级锁场景，避免系统调用的开销。
 *
 * 主要特性：
 * - 基于原子操作，无系统调用开销
 * - 轻量级实现
 * - 适用于竞争不激烈的场景
 * - 提供RAII风格的锁管理类
 */
class CASLock : noncopymoveable {
 public:
  using LockGuard = ScopedLock<CASLock>;  ///< 锁RAII包装器类型
  
  /**
   * @brief 构造函数，初始化原子标志
   */
  CASLock() { m_mutex.clear(); }

  /**
   * @brief 析构函数
   */
  ~CASLock() {}

  /**
   * @brief 获取锁
   * 
   * 使用自旋等待的方式获取锁，基于原子操作实现
   */
  void lock() {
    while (std::atomic_flag_test_and_set_explicit(&m_mutex,
                                                  std::memory_order_acquire))
      ;
  }

  /**
   * @brief 释放锁
   * 
   * 清除原子标志，释放锁
   */
  void unlock() {
    std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
  }

 private:
  std::atomic_flag m_mutex;  ///< 原子标志，用于实现锁机制
};

}  // namespace East
