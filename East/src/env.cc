/*
 * @Author: Xudong0722 
 * @Date: 2025-08-21 11:25:40 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-08-21 21:22:19
 */

#include "env.h"
#include "Elog.h"
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>

namespace East {

static East::Logger::sptr g_logger = ELOG_NAME("system");

bool Env::init(int argc, char** argv) {
  m_program = argv[0];
  char link[1024];
  char path[1024];
  sprintf(link, "/proc/%d/exe", getpid());
  readlink(link, path, sizeof(path));

  m_exe = path;
  auto pos = m_exe.find_last_of("/");
  m_cwd = m_exe.substr(0, pos) + "/";
  
  const char* key = nullptr;
  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') {
      if (strlen(argv[i]) > 1) {
        //exclude '-' case
        if (key != nullptr) {
          add(key, "");
        }
        key = argv[i] + 1;
      } else {
        ELOG_ERROR(g_logger)
            << "Invalid arg idx: " << i << ", val: " << argv[i];
        return false;
      }
    } else {
      if (key != nullptr) {
        add(key, argv[i]);
        key = nullptr;
      } else {
        ELOG_ERROR(g_logger)
            << "Invalid arg idx: " << i << ", val: " << argv[i];
        return false;
      }
    }
  }
  if (key != nullptr) {
    add(key, "");
  }
  return true;
}

void Env::add(const std::string& key, const std::string& val) {
  RWMutexType::WLockGuard wlock(m_mutex);
  m_args[key] = val;
}

bool Env::has(const std::string& key) {
  RWMutexType::RLockGuard rlock(m_mutex);
  if (m_args.find(key) != m_args.end())
    return true;
  return false;
}

void Env::del(const std::string& key) {
  RWMutexType::WLockGuard wlock(m_mutex);
  m_args.erase(key);
}

std::string Env::get(const std::string& key, const std::string& default_val) {
  RWMutexType::RLockGuard rlock(m_mutex);
  auto it = m_args.find(key);
  if (it == m_args.end())
    return default_val;
  return it->second;
}

void Env::printArgs() {
  RWMutexType::RLockGuard rlock(m_mutex);
  std::cout << "Usage: " << m_program << " [options]" << std::endl;
  for (const auto& [k, v] : m_args) {
    std::cout << std::setw(5) << k << "-" << v << '\n';
  }
}

void Env::addHelp(const std::string& key, const std::string& desc) {
  RWMutexType::WLockGuard wlock(m_mutex);
  m_helps.erase(
      std::remove_if(m_helps.begin(), m_helps.end(),
                     [&key](const auto& pair) { return pair.first == key; }),
      m_helps.end());
  m_helps.emplace_back(std::make_pair(key, desc));
}

void Env::removeHelp(const std::string& key) {
  RWMutexType::WLockGuard wlock(m_mutex);
  m_helps.erase(
      std::remove_if(m_helps.begin(), m_helps.end(),
                     [&key](const auto& pair) { return pair.first == key; }),
      m_helps.end());
}

void Env::printHelp() {
  RWMutexType::RLockGuard rlock(m_mutex);
  std::cout << "Usage: " << m_program << " [options]" << std::endl;
  for (const auto& [k, v] : m_helps) {
    std::cout << std::setw(5) << k << "-" << v << '\n';
  }
}

bool Env::setEnv(const std::string& key, const std::string& value) {
  return !setenv(key.c_str(), value.c_str(), 1);
}

std::string Env::getEnv(const std::string& key, const std::string& default_val){
  auto res = getenv(key.c_str());
  if(res == nullptr) return default_val;
  return std::string(res);
}

}  // namespace East
