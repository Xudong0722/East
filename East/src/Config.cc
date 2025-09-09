/*
 * @Author: Xudong0722
 * @Date: 2025-03-05 22:59:46
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-08-20 22:10:51
 */

#include "Config.h"
#include "env.h"
#include "Elog.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace East {

static East::Logger::sptr g_logger = ELOG_NAME("system");

ConfigVarBase::sptr Config::LoopupBase(const std::string& name) {
  auto it = GetDatas().find(name);
  return it == GetDatas().end() ? nullptr : it->second;
}

/*
    * A:
    *   B: 10
    *   C: string
    * 
    * "A.B" = 10
    */
void Config::ListAllMember(
    const std::string& prefix, const YAML::Node& node,
    std::list<std::pair<std::string, const YAML::Node>>& output) {
  if (prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789") !=
      std::string::npos) {
    ELOG_ERROR(g_logger)
        << "Config invalid name: " << prefix << " : " << node;
    return;
  }

  output.emplace_back(prefix, node);
  if (node.IsMap()) {
    for (auto it = node.begin(); it != node.end(); ++it) {
      ListAllMember(prefix.empty() ? it->first.Scalar()
                                   : prefix + "." + it->first.Scalar(),
                    it->second, output);
    }
  }
}

void Config::LoadFromYML(const YAML::Node& root) {
  std::list<std::pair<std::string, const YAML::Node>> all_nodes;
  ListAllMember("", root, all_nodes);

  for (auto& [k, v] : all_nodes) {
    if (k.empty())
      continue;

    std::transform(k.begin(), k.end(), k.begin(), ::tolower);
    auto var = LoopupBase(k);
    if (nullptr != var) {
      if (v.IsScalar()) {
        var->fromString(v.Scalar());
      } else {
        std::stringstream ss;
        //ss << v.Scalar(); bug from here!!!, v is not scalar,maybe map or sequence, just pass itself
        ss << v;
        var->fromString(ss.str());
      }
    }
  }
}

static std::map<std::string, uint64_t> s_file2modifytime;
static East::Mutex s_mutex;

void Config::LoadFromConfDir(const std::string& path) {
  std::string abs_path = East::EnvMgr::GetInst()->getAbsolutePath(path);
  std::vector<std::string> files;
  FSUtil::ListAllFile(files, abs_path, "yml");

  for(auto& i : files) {
    struct stat st;
    lstat(i.c_str(), &st);

    if(s_file2modifytime[i] == static_cast<uint64_t>(st.st_mtime)) {
      continue;
    }
    s_file2modifytime[i] = static_cast<uint64_t>(st.st_mtime);
    try{
      YAML::Node root = YAML::LoadFile(i);
      LoadFromYML(root);
      ELOG_INFO(g_logger) << "LoadConfFile file: " << i << " success.";
    }catch(...) {
      ELOG_ERROR(g_logger) << "LoadConfFile file: " << i << " failed.";
    }
  }
}

void Config::Visit(std::function<void(East::ConfigVarBase::sptr)> cb) {
  MutexType::RLockGuard lock(GetMutex());
  for(const auto& it : GetDatas()) {
    cb(it.second);
  }
}
}  // namespace East