/*
 * @Author: Xudong0722
 * @Date: 2025-03-05 22:28:48
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-06 00:19:04
 */

#pragma once

#include <string>
#include <map>
#include <memory>
#include <list>
#include <iostream> //test
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include "Elog.h"
#include "util.h"

namespace East
{
    /*
     * Config Item base class
     * Item : (name, val, description)
     */
    class ConfigVarBase
    {
    public:
        using sptr = std::shared_ptr<ConfigVarBase>;

        ConfigVarBase(const std::string &name, const std::string &description = "")
            : m_name(name), m_description(description) {
                std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
            }
        virtual ~ConfigVarBase() {}

        const std::string &getName() const { return m_name; }
        const std::string &getDescription() const { return m_description; }

        virtual std::string toString() = 0;
        virtual bool fromString(const std::string &str) = 0;

    private:
        std::string m_name;
        std::string m_description;
    };

    template<class F, class T>
    class LexicalCast
    { 
    public:
        T operator()(const F& v)
        {
            return boost::lexical_cast<T>(v);
        }
    };

    //support string -> vector
    template<class T>
    class LexicalCast<std::string, std::vector<T>>
    {
    public:
        std::vector<T> operator()(const std::string& str)
        {
            YAML::Node node = YAML::Load(str);
            typename std::vector<T> res{};
            std::stringstream ss;
            for(auto i = 0u; i < node.size(); ++i)
            {
                ss.str("");
                ss << node[i];
                res.emplace_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return res;
        }
    };

    //support vector -> string
    template<class T>
    class LexicalCast<std::vector<T>, std::string>
    {
    public:
        std::string operator()(const std::vector<T>& vec)
        {
            YAML::Node node(YAML::NodeType::Sequence);
            for(const auto& x : vec)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(x)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };


    template <class T, class FromStr = LexicalCast<std::string, T>, class ToStr = LexicalCast<T, std::string>>
    class ConfigVar : public ConfigVarBase
    {
    public:
        using sptr = std::shared_ptr<ConfigVar>;
        ConfigVar(const std::string &name, const T &val, const std::string &description = "")
            : ConfigVarBase(name, description), m_val(val)
        {
        }

        std::string toString() override
        {
            try
            {
                // return boost::lexical_cast<std::string>(m_val);
                return ToStr()(m_val);
            }
            catch (std::exception &e)
            {
                ELOG_ERROR(ELOG_ROOT()) << __FUNCTION__ << " exception: "
                                        << e.what() << " convert: " << typeid(T).name() << " to string.";
            }
            return {};
        }

        bool fromString(const std::string &str) override
        {
            try
            {
                // m_val = boost::lexical_cast<T>(str);
                setValue(FromStr()(str));
                return true;
            }
            catch (std::exception &e)
            {
                ELOG_ERROR(ELOG_ROOT()) << __FUNCTION__ << " exception: "
                                        << e.what() << " string convert to: " << typeid(T).name();
            }
            return false;
        }

        const T &getValue() const { return m_val; }
        void setValue(const T& t) { m_val = t; }

    private:
        T m_val;
    };

    /*
     * store config item: map<name(string), config item(ConfigVar)>
     * func1: Lookup(key, def_val, description),
     * return ptr if find key in map, otherwise make a new item with default value and description
     * func2: Lookup(key)
     * return ptr if find key in map, otherwise return null
     */
    class Config
    {
    public:
        using ConfigVarMap = std::map<std::string, ConfigVarBase::sptr>;

        template <class T>
        static typename ConfigVar<T>::sptr Lookup(const std::string &name, const T &default_val, const std::string description = "")
        {
            auto res = Lookup<T>(name);
            if (nullptr != res)
            {
                ELOG_INFO(ELOG_ROOT()) << " Lookup name: " << name << " exists";
                return res;
            }

            // name only support alpha '.' '_' and num
            if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789") != std::string::npos)
            {
                ELOG_ERROR(ELOG_ROOT()) << "Lookup name invalid " << name;
                throw std::invalid_argument(name);
            }
            auto new_item = std::make_shared<ConfigVar<T>>(name, default_val, description);
            s_datas[name] = new_item;
            return new_item;
        }

        template <class T>
        static typename ConfigVar<T>::sptr Lookup(const std::string &name)
        {
            auto it = s_datas.find(name);
            if (it == s_datas.end())
                return nullptr;
            return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
        }

        static ConfigVarBase::sptr LoopupBase(const std::string& name);
        static void ListAllMember(const std::string& prefix, const YAML::Node& node, 
            std::list<std::pair<std::string, const YAML::Node>>& output);
        static void LoadFromYML(const YAML::Node& root);
    private:
        static ConfigVarMap s_datas;
    };
} // namespace East
