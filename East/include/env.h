/*
 * @Author: Xudong0722 
 * @Date: 2025-08-21 10:55:24 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-08-21 21:01:55
 */
#pragma once

#include "Mutex.h"
#include "Thread.h"
#include "singleton.h"

#include <map>
#include <vector>

namespace East {
class Env {
 public:
  using RWMutexType = RWLock;

  bool init(int argc, char** argv);

  void add(const std::string& key, const std::string& val);
  bool has(const std::string& key);
  void del(const std::string& key);
  std::string get(const std::string& key, const std::string& default_val = "");
  void printArgs();

  void addHelp(const std::string& key, const std::string& desc);
  void removeHelp(const std::string& key);
  void printHelp();

  const std::string& getExe() const { return m_exe; }
  const std::string& getCwd() const { return m_cwd; }

  bool setEnv(const std::string& key, const std::string& value);
  std::string getEnv(const std::string& key, const std::string& default_val = "");

  std::string getAbsolutePath(const std::string& path) const;
 private:
  std::string m_program;
  std::string m_exe;
  std::string m_cwd;
  RWMutexType m_mutex;
  std::map<std::string, std::string> m_args;
  std::vector<std::pair<std::string, std::string>> m_helps;
};

using EnvMgr = East::Singleton<Env>;
}  // namespace East
