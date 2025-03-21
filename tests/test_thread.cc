/*
 * @Author: Xudong0722 
 * @Date: 2025-03-21 13:43:29 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-21 18:23:50
 */

#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <chrono>
#include "../East/include/Elog.h"
#include "../East/include/Thread.h"

East::Logger::sptr g_logger = ELOG_ROOT();

int count = 0;
std::atomic_int a_count = 0;
East::RWLock rwlock;
std::mutex lock;
East::Mutex emutex;

void fun1() {
  ELOG_INFO(g_logger) << "name: " << East::Thread::GetName()
                      << ", this name: " << East::Thread::GetThis()->getName()
                      << ", id: " << East::getThreadId()
                      << ", this id: " << East::Thread::GetThis()->getId();
  for (int i = 0; i < 10000000; ++i) {
    //East::WLock wlock(rwlock);
    //std::unique_lock<std::mutex> lk(lock);   纯写场景， mutex笔wlock效率要高
    //++count;
    // ++a_count;
    East::ScopedLock mutex(emutex);
    ++count;
  }
}

void fun2() {}
int main() {
  ELOG_INFO(g_logger) << "Thread test begin----";

  std::vector<East::Thread::sptr> threads{};
  for (int i = 0; i < 10; ++i) {
    East::Thread::sptr t =
        std::make_shared<East::Thread>("name_" + std::to_string(i), &fun1);
    threads.emplace_back(t);
  }

  for (auto& i : threads) {
    i->join();
  }

  ELOG_INFO(g_logger) << "Thread test end-----";
  ELOG_INFO(g_logger) << "count = " << count;
  return 0;
}