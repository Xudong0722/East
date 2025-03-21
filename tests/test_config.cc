/*
 * @Author: Xudong0722
 * @Date: 2025-03-05 23:34:28
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-21 13:43:34
 */

#include <yaml-cpp/yaml.h>
#include <iostream>
#include "../East/include/Config.h"
#include "../East/include/Elog.h"

East::ConfigVar<int>::sptr g_int_value_config =
    East::Config::Lookup("system.port", (int)8080, "system port");

//East::ConfigVar<double>::sptr g_double_value2_config =
//    East::Config::Lookup("system.port", (double)8080, "system port d");    //test case: same port, different type, will return nullptr

East::ConfigVar<float>::sptr g_float_value_config =
    East::Config::Lookup("system.value", 10.2f, "system value");

East::ConfigVar<std::vector<int>>::sptr g_int_vec_value_config =
    East::Config::Lookup("system.int_vec", std::vector<int>{1, 2},
                         "system int vector");

East::ConfigVar<std::list<int>>::sptr g_int_list_value_config =
    East::Config::Lookup("system.int_list", std::list<int>{3, 4},
                         "system int list");

East::ConfigVar<std::set<int>>::sptr g_int_set_value_config =
    East::Config::Lookup("system.int_set", std::set<int>{5, 6},
                         "system int set");

East::ConfigVar<std::unordered_set<int>>::sptr g_int_uset_value_config =
    East::Config::Lookup("system.int_uset", std::unordered_set<int>{7, 8},
                         "system int uset");

East::ConfigVar<std::map<std::string, int>>::sptr g_str_int_map_value_config =
    East::Config::Lookup("system.str_int_map",
                         std::map<std::string, int>{{"k", 12}},
                         "system str int map");

East::ConfigVar<std::unordered_map<std::string, int>>::sptr
    g_str_int_umap_value_config = East::Config::Lookup(
        "system.str_int_umap", std::unordered_map<std::string, int>{{"k", 444}},
        "system str int umap");

class Person {
 public:
  std::string m_name;
  int m_age{0};
  bool is_male{false};
  std::string toString() const {
    std::stringstream ss;
    ss << "Name: " << m_name << " age: " << m_age << " is male: " << is_male;
    return ss.str();
  }

  bool operator==(const Person& rhs) const {
    return m_name == rhs.m_name && m_age == rhs.m_age && is_male == rhs.is_male;
  }
};

// support string -> Person
template <>
class East::LexicalCast<std::string, Person> {
 public:
  Person operator()(const std::string& str) {
    YAML::Node node = YAML::Load(str);
    Person p{};
    p.m_name = node["name"].as<std::string>();
    p.is_male = node["is_male"].as<bool>();
    p.m_age = node["age"].as<int>();
    return p;
  }
};

// support Person -> string
template <>
class East::LexicalCast<Person, std::string> {
 public:
  std::string operator()(const Person& person) {
    YAML::Node node;
    node["name"] = person.m_name;
    node["age"] = person.m_age;
    node["is_male"] = person.is_male;
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

East::ConfigVar<Person>::sptr g_person_value_config = East::Config::Lookup(
    "system.class.person", Person{"elvis", 25, true}, "system class person");

void print_yml(const YAML::Node& node, int level) {
  if (node.IsScalar()) {
    ELOG_INFO(ELOG_ROOT()) << std::string(level * 4, ' ') << node.Scalar()
                           << " - " << node.Type() << " - " << level;
  } else if (node.IsNull()) {
    ELOG_INFO(ELOG_ROOT()) << std::string(level * 4, ' ') << "NULL -"
                           << node.Type() << "-" << level;
  } else if (node.IsMap()) {
    for (auto it = node.begin(); it != node.end(); ++it) {
      ELOG_INFO(ELOG_ROOT()) << std::string(level * 4, ' ') << it->second
                             << " - " << it->second.Type() << " - " << level;
      print_yml(it->second, level + 1);
    }
  } else if (node.IsSequence()) {
    for (auto i = 0u; i < node.size(); ++i) {
      ELOG_INFO(ELOG_ROOT()) << std::string(level * 4, ' ') << i << " - "
                             << node[i].Type() << " - " << level;
      print_yml(node[i], level + 1);
    }
  }
}

void test_yml() {
  YAML::Node root = YAML::LoadFile("/elvis/East/bin/conf/log.yml");
  print_yml(root, 0);
  // ELOG_INFO(ELOG_ROOT()) << root;
  // ELOG_INFO(ELOG_ROOT()) << root.Type();
}

void test_config() {
  ELOG_INFO(ELOG_ROOT()) << "Before:" << g_int_value_config->getName() << " "
                         << g_int_value_config->getValue() << " "
                         << g_int_value_config->toString();
  ELOG_INFO(ELOG_ROOT()) << "Before:" << g_float_value_config->getName() << " "
                         << g_float_value_config->getValue() << " "
                         << g_float_value_config->toString();

#define for_loop(g_val, name, prefix)                               \
  for (const auto& i : g_val->getValue()) {                         \
    ELOG_INFO(ELOG_ROOT()) << #prefix << ": " << #name << " " << i; \
  }                                                                 \
  ELOG_INFO(ELOG_ROOT()) << #prefix << ": " << #name << " "         \
                         << g_val->toString();

#define for_loop_map(g_val, name, prefix)                                    \
  for (const auto& [k, v] : g_val->getValue()) {                             \
    ELOG_INFO(ELOG_ROOT()) << #prefix << ": " << #name << " {" << k << " - " \
                           << v << " }";                                     \
  }                                                                          \
  ELOG_INFO(ELOG_ROOT()) << #prefix << ": " << #name << " "                  \
                         << g_val->toString();

  for_loop(g_int_vec_value_config, int_vec, before);
  for_loop(g_int_list_value_config, int_list, before);
  for_loop(g_int_set_value_config, int_set, before);
  for_loop(g_int_uset_value_config, int_uset, before);
  for_loop_map(g_str_int_map_value_config, str_int_map, before);
  for_loop_map(g_str_int_umap_value_config, int_umap, before);

  ELOG_INFO(ELOG_ROOT()) << "Before:"
                         << g_person_value_config->getValue().toString()
                         << " - " << g_person_value_config->toString();
  g_person_value_config->addListener(
      101, [](const Person& old_person, const Person& new_person) {
        ELOG_INFO(ELOG_ROOT()) << "Old value: " << old_person.toString()
                               << ", new value: " << new_person.toString();
      });

  YAML::Node root = YAML::LoadFile("/elvis/East/bin/conf/test.yml");
  East::Config::LoadFromYML(root);

  ELOG_INFO(ELOG_ROOT()) << "After:" << g_int_value_config->getName() << " "
                         << g_int_value_config->getValue() << " "
                         << g_int_value_config->toString();
  ELOG_INFO(ELOG_ROOT()) << "After:" << g_float_value_config->getName() << " "
                         << g_float_value_config->getValue() << " "
                         << g_float_value_config->toString();
  ELOG_INFO(ELOG_ROOT()) << "After:"
                         << g_person_value_config->getValue().toString()
                         << " - " << g_person_value_config->toString();

  for_loop(g_int_vec_value_config, int_vec, after);
  for_loop(g_int_list_value_config, int_list, after);
  for_loop(g_int_set_value_config, int_set, after);
  for_loop(g_int_uset_value_config, int_uset, after);
  for_loop_map(g_str_int_map_value_config, str_int_map, after);
  for_loop_map(g_str_int_umap_value_config, int_umap, after);
}

void test_log() {
  East::Logger::sptr system_log = ELOG_NAME("system");
  ELOG_INFO(system_log) << "Hello system";
  std::cout << East::LogMgr::GetInst()->toYamlString() << std::endl;
  YAML::Node root = YAML::LoadFile("/elvis/East/bin/conf/log.yml");
  East::Config::LoadFromYML(root);
  std::cout << "================================\n";
  std::cout << East::LogMgr::GetInst()->toYamlString() << std::endl;
  ELOG_INFO(system_log) << "Hello system";
}

int main() {
  // test_yml();
  //test_config();
  test_log();
  return 0;
}