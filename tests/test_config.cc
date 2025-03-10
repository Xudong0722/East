/*
 * @Author: Xudong0722
 * @Date: 2025-03-05 23:34:28
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-06 00:42:35
 */

#include "../East/include/Config.h"
#include "../East/include/Elog.h"
#include <yaml-cpp/yaml.h>


East::ConfigVar<int>::sptr g_int_value_config =
    East::Config::Lookup("system.port", (int)8080, "system port");

East::ConfigVar<float>::sptr g_float_value_config =
    East::Config::Lookup("system.value", 10.2f, "system value");

East::ConfigVar<std::vector<int>>::sptr g_int_vec_value_config =
    East::Config::Lookup("system.int_vec", std::vector<int>{1, 2}, "system int vector");

void print_yml(const YAML::Node& node, int level)
{
    if(node.IsScalar())
    {
        ELOG_INFO(ELOG_ROOT()) << std::string(level * 4, ' ')
            << node.Scalar() << " - " << node.Type() << " - " << level;
    }else if(node.IsNull())
    {
        ELOG_INFO(ELOG_ROOT()) << std::string(level * 4, ' ')
            << "NULL -" << node.Type() << "-" << level;
    }else if(node.IsMap())
    {
        for(auto it = node.begin(); it != node.end(); ++it)
        {
            ELOG_INFO(ELOG_ROOT()) << std::string(level * 4, ' ')
                << it->second << " - " << it->second.Type() << " - " << level;
            print_yml(it->second, level + 1);
        }
    }else if(node.IsSequence())
    {
        for(auto i = 0u; i<node.size(); ++i)
        {
            ELOG_INFO(ELOG_ROOT()) << std::string(level * 4, ' ')
                << i << " - " << node[i].Type() << " - " << level;
            print_yml(node[i], level + 1);
        }
    }
}

void test_yml()
{
    YAML::Node root = YAML::LoadFile("/elvis/East/bin/conf/log.yml");
    print_yml(root, 0);
    //ELOG_INFO(ELOG_ROOT()) << root;
    //ELOG_INFO(ELOG_ROOT()) << root.Type();
}

void test_config()
{
    ELOG_INFO(ELOG_ROOT()) << "Before:" << g_int_value_config->getName() << " " << g_int_value_config->getValue() << " " << g_int_value_config->toString();
    ELOG_INFO(ELOG_ROOT()) << "Before:" << g_float_value_config->getName() << " " << g_float_value_config->getValue() << " " << g_float_value_config->toString();
    for(int i : g_int_vec_value_config->getValue())
    {
        ELOG_INFO(ELOG_ROOT()) << "Before:" << i;
    }

    YAML::Node root = YAML::LoadFile("/elvis/East/bin/conf/log.yml");
    East::Config::LoadFromYML(root);

    ELOG_INFO(ELOG_ROOT()) << "After:" << g_int_value_config->getName() << " " << g_int_value_config->getValue() << " " << g_int_value_config->toString();
    ELOG_INFO(ELOG_ROOT()) << "After:" << g_float_value_config->getName() << " " << g_float_value_config->getValue() << " " << g_float_value_config->toString();
    for(int i : g_int_vec_value_config->getValue())
    {
        ELOG_INFO(ELOG_ROOT()) << "After:" << i;
    }
}

int main()
{
    //test_yml();
    test_config();
    return 0;
}