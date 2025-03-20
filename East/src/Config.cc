/*
 * @Author: Xudong0722
 * @Date: 2025-03-05 22:59:46
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-06 00:18:54
 */

#include "Config.h"

namespace East {

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
    ELOG_ERROR(ELOG_ROOT())
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
}  // namespace East