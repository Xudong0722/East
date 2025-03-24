/*
 * @Author: Xudong0722 
 * @Date: 2025-03-21 13:43:29 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-23 22:25:05
 */

#include <unistd.h>
#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>
#include "../East/include/Config.h"
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

void fun2() {
  for(;;)
    ELOG_INFO(g_logger) << "=============================================================================================";
}

void fun3() {
  for (;;)
    ELOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
}

int main() {
  ELOG_INFO(g_logger) << "Thread test begin----";
  YAML::Node root = YAML::LoadFile("/elvis/East/bin/conf/test_mthread_log.yml");
  East::Config::LoadFromYML(root);
  std::vector<East::Thread::sptr> threads{};
  for (int i = 0; i < 5; ++i) {
    East::Thread::sptr t1 =
        std::make_shared<East::Thread>("name_" + std::to_string(i * 2), &fun2);
    East::Thread::sptr t2 = std::make_shared<East::Thread>(
        "name_" + std::to_string(i * 2 + 1), &fun3);
    threads.emplace_back(t1);
    threads.emplace_back(t2);
  }

  for (auto& i : threads) {
    i->join();
  }

  ELOG_INFO(g_logger) << "Thread test end-----";
  ELOG_INFO(g_logger) << "count = " << count;
  return 0;
}