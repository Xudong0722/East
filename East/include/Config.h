/*
 * @Author: Xudong0722
 * @Date: 2025-03-05 22:28:48
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-06 00:19:04
 */

#pragma once

#include <yaml-cpp/yaml.h>
#include <boost/lexical_cast.hpp>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "Elog.h"
#include "util.h"

//Listener ID
#define LOG_INITIATOR_CALLBACK_ID 0x1LL

namespace East {
/*
     * Config Item base class
     * Item : (name, val, description)
     */
class ConfigVarBase {
 public:
  using sptr = std::shared_ptr<ConfigVarBase>;

  ConfigVarBase(const std::string& name, const std::string& description = "")
      : m_name(name), m_description(description) {
    std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
  }
  virtual ~ConfigVarBase() {}

  const std::string& getName() const { return m_name; }
  const std::string& getDescription() const { return m_description; }

  virtual std::string toString() = 0;
  virtual bool fromString(const std::string& str) = 0;
  virtual std::string getTypeName() const = 0;

 private:
  std::string m_name;
  std::string m_description;
};

template <class F, class T>
class LexicalCast {
 public:
  T operator()(const F& v) { return boost::lexical_cast<T>(v); }
};

// support string -> vector
template <class T>
class LexicalCast<std::string, std::vector<T>> {
 public:
  std::vector<T> operator()(const std::string& str) {
    YAML::Node node = YAML::Load(str);
    std::vector<T> res{};
    std::stringstream ss;
    for (auto i = 0u; i < node.size(); ++i) {
      ss.str("");
      ss << node[i];
      res.emplace_back(LexicalCast<std::string, T>()(ss.str()));
    }
    return res;
  }
};

// support vector -> string
template <class T>
class LexicalCast<std::vector<T>, std::string> {
 public:
  std::string operator()(const std::vector<T>& vec) {
    YAML::Node node(YAML::NodeType::Sequence);
    for (const auto& x : vec) {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(x)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

// support string -> list
template <class T>
class LexicalCast<std::string, std::list<T>> {
 public:
  std::list<T> operator()(const std::string& str) {
    YAML::Node node = YAML::Load(str);
    std::list<T> res{};
    std::stringstream ss;
    for (auto i = 0u; i < node.size(); ++i) {
      ss.str("");
      ss << node[i];
      res.emplace_back(LexicalCast<std::string, T>()(ss.str()));
    }
    return res;
  }
};

// support list -> string
template <class T>
class LexicalCast<std::list<T>, std::string> {
 public:
  std::string operator()(const std::list<T>& vec) {
    YAML::Node node(YAML::NodeType::Sequence);
    for (const auto& x : vec) {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(x)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

// support string -> set
template <class T>
class LexicalCast<std::string, std::set<T>> {
 public:
  std::set<T> operator()(const std::string& str) {
    YAML::Node node = YAML::Load(str);
    std::set<T> res{};
    std::stringstream ss;
    for (auto i = 0u; i < node.size(); ++i) {
      ss.str("");
      ss << node[i];
      res.insert(LexicalCast<std::string, T>()(ss.str()));
    }
    return res;
  }
};

// support set -> string
template <class T>
class LexicalCast<std::set<T>, std::string> {
 public:
  std::string operator()(const std::set<T>& vec) {
    YAML::Node node(YAML::NodeType::Sequence);
    for (const auto& x : vec) {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(x)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

// support string -> unordered_set
template <class T>
class LexicalCast<std::string, std::unordered_set<T>> {
 public:
  std::unordered_set<T> operator()(const std::string& str) {
    YAML::Node node = YAML::Load(str);
    std::unordered_set<T> res{};
    std::stringstream ss;
    for (auto i = 0u; i < node.size(); ++i) {
      ss.str("");
      ss << node[i];
      res.insert(LexicalCast<std::string, T>()(ss.str()));
    }
    return res;
  }
};

// support unordered_set -> string
template <class T>
class LexicalCast<std::unordered_set<T>, std::string> {
 public:
  std::string operator()(const std::unordered_set<T>& vec) {
    YAML::Node node(YAML::NodeType::Sequence);
    for (const auto& x : vec) {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(x)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

// support string -> map<string, T>
template <class T>
class LexicalCast<std::string, std::map<std::string, T>> {
 public:
  std::map<std::string, T> operator()(const std::string& str) {
    YAML::Node node = YAML::Load(str);
    std::map<std::string, T> res{};
    std::stringstream ss;
    for (auto it = node.begin(); it != node.end(); ++it) {
      ss.str("");
      ss << it->second;
      res.emplace(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str()));
    }
    return res;
  }
};

// support map<string, T> -> string
template <class T>
class LexicalCast<std::map<std::string, T>, std::string> {
 public:
  std::string operator()(const std::map<std::string, T>& mp) {
    YAML::Node node(YAML::NodeType::Map);
    for (const auto& [k, v] : mp) {
      node[k] = YAML::Load(LexicalCast<T, std::string>()(v));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

// support string -> unordered_map<string, T>
template <class T>
class LexicalCast<std::string, std::unordered_map<std::string, T>> {
 public:
  std::unordered_map<std::string, T> operator()(const std::string& str) {
    YAML::Node node = YAML::Load(str);
    std::unordered_map<std::string, T> res{};
    std::stringstream ss;
    for (auto it = node.begin(); it != node.end(); ++it) {
      ss.str("");
      ss << it->second;
      res.emplace(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str()));
    }
    return res;
  }
};

// support unordered_map<string, T> -> string
template <class T>
class LexicalCast<std::unordered_map<std::string, T>, std::string> {
 public:
  std::string operator()(const std::unordered_map<std::string, T>& mp) {
    YAML::Node node(YAML::NodeType::Map);
    for (const auto& [k, v] : mp) {
      node[k] = YAML::Load(LexicalCast<T, std::string>()(v));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

template <class T, class FromStr = LexicalCast<std::string, T>,
          class ToStr = LexicalCast<T, std::string>>
class ConfigVar : public ConfigVarBase {
 public:
  using sptr = std::shared_ptr<ConfigVar>;
  using listener = std::function<void(const T& old_v, const T& new_v)>;
  ConfigVar(const std::string& name, const T& val,
            const std::string& description = "")
      : ConfigVarBase(name, description), m_val(val) {}

  std::string toString() override {
    try {
      // return boost::lexical_cast<std::string>(m_val);
      return ToStr()(m_val);
    } catch (std::exception& e) {
      ELOG_ERROR(ELOG_ROOT())
          << __FUNCTION__ << " exception: " << e.what()
          << " convert: " << typeid(T).name() << " to string.";
    }
    return {};
  }

  bool fromString(const std::string& str) override {
    try {
      // m_val = boost::lexical_cast<T>(str);
      setValue(FromStr()(str));
      return true;
    } catch (std::exception& e) {
      ELOG_ERROR(ELOG_ROOT()) << __FUNCTION__ << " exception: " << e.what()
                              << " string convert to: " << typeid(T).name();
    }
    return false;
  }

  std::string getTypeName() const override { return typeid(T).name(); }

  const T& getValue() const { return m_val; }
  void setValue(const T& t) {
    if (t == m_val)
      return;
    for (const auto& [_, cb] : m_cbs) {
      cb(m_val, t);
    }
    m_val = t;
  }

  void addListener(uint64_t key, listener cb) { m_cbs[key] = cb; }

  void delListener(uint64_t key) { m_cbs.erase(key); }

  listener getListener(uint64_t key) { return m_cbs[key]; }

  void clearAllListeners() { m_cbs.clear(); }

 private:
  T m_val;
  //std::function not support operator==, we can ues map to store all the callbacks
  std::map<uint64_t, listener> m_cbs;
};

/*
     * store config item: map<name(string), config item(ConfigVar)>
     * func1: Lookup(key, def_val, description),
     * return ptr if find key in map, otherwise make a new item with default value and description
     * func2: Lookup(key)
     * return ptr if find key in map, otherwise return null
     */
class Config {
 public:
  using ConfigVarMap = std::map<std::string, ConfigVarBase::sptr>;

  template <class T>
  static typename ConfigVar<T>::sptr Lookup(
      const std::string& name, const T& default_val,
      const std::string description = "") {
    // auto res = Lookup<T>(name);
    // if (nullptr != res)                  // Maybe same key, different data type, we need to prevent this case
    // {
    //     ELOG_INFO(ELOG_ROOT()) << " Lookup name: " << name << " exists";
    //     return res;
    // }

    auto it = GetDatas().find(name);
    if (it != GetDatas().end()) {
      auto item = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
      if (nullptr == item) {
        ELOG_ERROR(ELOG_ROOT())
            << "Exists but the type not match, existed item's type: "
            << it->second->getTypeName()
            << ", current type is: " << typeid(T).name();
        return nullptr;
      } else {
        ELOG_INFO(ELOG_ROOT()) << "Existed!";
        return item;
      }
    }

    // name only support alpha '.' '_' and num
    if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789") !=
        std::string::npos) {
      ELOG_ERROR(ELOG_ROOT()) << "Lookup name invalid " << name;
      throw std::invalid_argument(name);
    }
    auto new_item =
        std::make_shared<ConfigVar<T>>(name, default_val, description);
    GetDatas()[name] = new_item;
    return new_item;
  }

  template <class T>
  static typename ConfigVar<T>::sptr Lookup(const std::string& name) {
    auto it = GetDatas().find(name);
    if (it == GetDatas().end())
      return nullptr;
    return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
  }

  static ConfigVarBase::sptr LoopupBase(const std::string& name);
  static void ListAllMember(
      const std::string& prefix, const YAML::Node& node,
      std::list<std::pair<std::string, const YAML::Node>>& output);
  static void LoadFromYML(const YAML::Node& root);

 private:
  static ConfigVarMap& GetDatas() {
    static ConfigVarMap s_datas;
    return s_datas;
  }
};
}  // namespace East
