//
// Created by jxq on 20-2-15.
//

#ifndef MYSYLAR_CONFIG_H
#define MYSYLAR_CONFIG_H

#include <memory>
#include <string>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <unordered_map>
#include <list>
#include <set>
#include <map>
#include <unordered_set>
#include <functional>
#include <yaml-cpp/yaml.h>
#include "log.h"
#include "util.h"

namespace sylar{

    // 配置变量的基类 名称 + 描述
    class ConfigVarBase {
    public:
        typedef std::shared_ptr<ConfigVarBase> ptr;
        ConfigVarBase(const std::string& name, const std::string& description = "")
            : m_name(name), m_description(description)
        {
            std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
        }

        virtual ~ConfigVarBase() {}

        const std::string getName() const { return m_name; }
        const std::string getDescription() const { return m_description; }

        virtual std::string toString() = 0;
        virtual bool fromString(const std::string& val) = 0;

        /**
         * @brief 返回配置参数值的类型名称
         */
        virtual std::string getTypeName() const = 0;
    protected:
        std::string m_name;
        std::string m_description;
    };

    // 类型转换模版类(F 源类型， T 目标类型)
    template<class F, class T>
    class LexicalCast
    {
    public:
        T operator()(const F& v)
        {
            return boost::lexical_cast<T>(v);
        }
    };

    // 类型转换模版类片特化（YAML String 转换程 std::vector<T>）
    template<class T>
    class LexicalCast<std::string, std::vector<T> >
    {
    public:
        std::vector<T> operator()(const std::string& v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::vector<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); ++i)
            {
                ss.str("");
                ss << node[i];
                vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    // 类型转换模板类片特化(std::vector<T> 转换成 YAML String)
    template< class T>
    class LexicalCast<std::vector<T>, std::string>
    {
    public:
        std::string operator()(const std::vector<T>& v)
        {
            YAML::Node node(YAML::NodeType::Sequence);  // 列表
            for (auto& i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>() (i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 类型转换模板类片特化(YAML String 转换成 std::list<T>)
    template<class T>
    class LexicalCast<std::string, std::list<T> >
    {
    public:
        std::list<T> operator() (const std::string& v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::list<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); ++i)
            {
                ss.str("");
                ss << node[i];
                vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    // 类型转换模板类片特化(std::list<T> 转换成 YAML String)
    template<class T>
    class LexicalCast<std::list<T>, std::string>
    {
    public:
        std::string operator() (const std::list<T>& v)
        {
            YAML::Node node(YAML::NodeType::Sequence);
            for (auto& i : v)
            {
                node.push_back(YAML::Node(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 类型转换模板类片特化(YAML String 转换成 std::set<T>)
    template<class T>
    class LexicalCast<std::string, std::set<T> >
    {
    public:
        std::set<T> operator()(const std::string& v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::set<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); ++i)
            {
                ss.str("");
                ss << node[i];
                vec.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    // 类型转换模板类片特化(std::set<T> 转换成 YAML String)
    template<class T>
    class LexicalCast<std::set<T>, std::string>
    {
    public:
        std::string operator() (const std::set<T>& v)
        {
            YAML::Node node(YAML::NodeType::Sequence);
            for (auto& i : v)
            {
                node.push_back(LexicalCast<T, std::string>()(i));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 类型转换模板类片特化(YAML String 转换成 std::unordered_set<T>)
    template<class T>
    class LexicalCast<std::string, std::unordered_set<T> >
    {
    public:
        std::unordered_set<T> operator() (const std::string& v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::unordered_set<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); ++i)
            {
                ss.str("");
                ss << node[i];
                vec.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    // 类型转换模板类片特化(std::unordered_set<T> 转换成 YAML String)
    template<class T>
    class LexicalCast<std::unordered_set<T>, std::string>
    {
    public:
        std::string operator() (const std::unordered_set<T>& v)
        {
            YAML::Node node(YAML::NodeType::Sequence);
            for(auto& i : v)
            {
                node.push_back(LexicalCast<T, std::string>()(i));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 类型转换模板类片特化(YAML String 转换成 std::map<std::string, T>)
    template<class T>
    class LexicalCast<std::string, std::map<std::string, T> >
    {
    public:
        std::map<std::string, T> operator() (const std::string& v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::map<std::string, T> vec;
            std::stringstream ss;
            for (auto it = node.begin(); it != node.end(); ++it)
            {
                ss.str("");
                ss << it->second;
                vec.insert(std::make_pair(it->first.Scalar(),
                        LexicalCast<std::string, T>()(ss.str())));
            }
            return vec;
        };
    };

    /**
     * @brief 类型转换模板类片特化(std::map<std::string, T> 转换成 YAML String)
     */
    template<class T>
    class LexicalCast<std::map<std::string, T>, std::string> {
    public:
        std::string operator() (const std::map<std::string, T>& v) {
            YAML::Node node(YAML::NodeType::Map);
            for(auto& i : v) {
                node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    /**
     * @brief 类型转换模板类片特化(YAML String 转换成 std::unordered_map<std::string, T>)
     */
    template<class T>
    class LexicalCast<std::string, std::unordered_map<std::string, T> > {
    public:
        std::unordered_map<std::string, T> operator() (const std::string& v) {
            YAML::Node node = YAML::Load(v);
            typename std::unordered_map<std::string, T> vec;
            std::stringstream ss;
            for(auto it = node.begin();
                it != node.end(); ++it) {
                ss.str("");
                ss << it->second;
                vec.insert(std::make_pair(it->first.Scalar(),
                                          LexicalCast<std::string, T>()(ss.str())));
            }
            return vec;
        }
    };

    /**
     * @brief 类型转换模板类片特化(std::unordered_map<std::string, T> 转换成 YAML String)
     */
    template<class T>
    class LexicalCast<std::unordered_map<std::string, T>, std::string> {
    public:
        std::string operator() (const std::unordered_map<std::string, T>& v) {
            YAML::Node node(YAML::NodeType::Map);
            for(auto& i : v) {
                node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 配置参数模版子类，保存对应类型的参数值
    template<class T, class FromStr = LexicalCast<std::string, T>,
                      class ToStr = LexicalCast<T, std::string> >
    class ConfigVar : public ConfigVarBase {
    public:
        typedef std::shared_ptr<ConfigVar> ptr;
        typedef std::function<void(const T& old_value, const T& new_value)> on_change_cb;

        ConfigVar(const std::string& name, const T& default_value, const std::string& description = "")
            : ConfigVarBase(name, description)
            , m_val(default_value)
        {

        }

        // 从T转换成std::string的仿函数
        std::string toString() override {
            try {
                // ------ T类型 --> 字符串
                //return boost::lexical_cast<std::string>(m_val);
                return ToStr()(m_val);
            } catch (std::exception& e)
            {
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::toString exception "
                    << e.what();
//                    << " convert: " << TypeToName<T> << " to string"
//                    << " name=" << m_name;
            }
            return "";
        }

        // 从std::string转换成T类型的仿函数
        bool fromString(const std::string& val) override {
            try {
                //m_val = boost::lexical_cast<T>(val);
                setValue(FromStr()(val));
            } catch (std::exception& e)
            {
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::fromString exception "
                    << e.what();
            }
            return false;
        }

        const T getValue()
        {
            return m_val;
        }

        void setValue(const T& v)
        {
            if (v == m_val) // 自定类 == 重写
            {
                return;
            }
            for (auto& i : m_cbs)
            {
                i.second(m_val, v);
            }

            m_val = v;
        }


        /**
         * @brief 返回参数值的类型名称(typeinfo)
         */
        std::string getTypeName() const override { return TypeToName<T>();}


        /**
         * @brief 添加变化回调函数
         * @return 返回该回调函数对应的唯一id,用于删除回调
         */
        uint64_t addListener(on_change_cb cb) {
            static uint64_t s_fun_id = 0;
            // RWMutexType::WriteLock lock(m_mutex);
            ++s_fun_id;
            m_cbs[s_fun_id] = cb;
            return s_fun_id;
        }

        /**
         * @brief 删除回调函数
         * @param[in] key 回调函数的唯一id
         */
        void delListener(uint64_t key) {
            //RWMutexType::WriteLock lock(m_mutex);
            m_cbs.erase(key);
        }

        /**
         * @brief 获取回调函数
         * @param[in] key 回调函数的唯一id
         * @return 如果存在返回对应的回调函数,否则返回nullptr
         */
        on_change_cb getListener(uint64_t key) {
            //RWMutexType::ReadLock lock(m_mutex);
            auto it = m_cbs.find(key);
            return it == m_cbs.end() ? nullptr : it->second;
        }

        /**
         * @brief 清理所有的回调函数
         */
        void clearListener() {
            //RWMutexType::WriteLock lock(m_mutex);
            m_cbs.clear();
        }
    private:
        T m_val;
        //变更回调函数组, uint64_t key,要求唯一，一般可以用hash
        std::map<uint64_t, on_change_cb> m_cbs;
    };

    // ConfigVar的管理类
    class Config {
    public:
        typedef std::unordered_map<std::string, ConfigVarBase::ptr> ConfigVarMap;

        template<class T>
        static typename ConfigVar<T>::ptr Lookup(const std::string& name,
                const T& default_value, const std::string& description = "")
        {
            auto it = GetDatas().find(name);
            if (it != GetDatas().end())
            {
                auto tmp = std::dynamic_pointer_cast<ConfigVar<T> >(it->second);
                if (tmp)
                {
                    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Lookup name = " << name << "exists";
                    return tmp;
                }
                else
                {
                    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name=" << name << " exists but type not "
                                                      << TypeToName<T>() << " real_type=" << it->second->getTypeName()
                                                      << " " << it->second->toString();
                    return nullptr;
                }
            }
            if (name.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678") != std::string::npos)
            {
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name invalid " << name;
                throw std::invalid_argument(name);
            }

            typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
            GetDatas()[name] = v;
            return v;
        }

        // 使用YAML::Node 初始化配置魔抗
        static void LoadFromYaml(const  YAML::Node& root);

        /**
         * @brief 查找配置参数,返回配置参数的基类
         * @param[in] name 配置参数名称
         */
        static ConfigVarBase::ptr LookupBase(const std::string& name);
    private:
        // 返回所有的配置项
        static ConfigVarMap& GetDatas()
        {
            static ConfigVarMap s_datas;    // 数据存放处
            return s_datas;
        }
    };
};


#endif //MYSYLAR_CONFIG_H
