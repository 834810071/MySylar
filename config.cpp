//
// Created by jxq on 20-2-15.
//

#include <yaml-cpp/yaml.h>
#include "config.h"

namespace sylar
{
    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    ConfigVarBase::ptr Config::LookupBase(const std::string &name)
    {
        auto it = GetDatas().find(name);
        return it == GetDatas().end() ? nullptr : it->second;
    }
    //"A.B", 10
    //A:
    //  B: 10
    //  C: str

    static void ListAllMember(const std::string& prefix,
                              const YAML::Node& node,
                              std::list<std::pair<std::string, const YAML::Node> >& output) {
        if(prefix.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678")
           != std::string::npos) {
            SYLAR_LOG_ERROR(g_logger) << "Config invalid name: " << prefix << " : " << node;
            return;
        }
        output.push_back(std::make_pair(prefix, node));

        if(node.IsMap()) {
            for(auto it = node.begin(); it != node.end(); ++it) {
                ListAllMember(prefix.empty() ? it->first.Scalar()
                                             : prefix + "." + it->first.Scalar(), it->second, output);
            }
        }
    }

    void Config::LoadFromYaml(const  YAML::Node& root)
    {
        std::list<std::pair<std::string, const YAML::Node> > all_nodes;
        ListAllMember("", root , all_nodes);    // 加载map

        for (auto& i : all_nodes)
        {
            std::string key = i.first;
            if (key.empty())    // 起始的时候
            {
                continue;
            }

            std::transform(key.begin(), key.end(), key.begin(), ::tolower); // key转换为小写
            ConfigVarBase::ptr var = LookupBase(key);   // 获取当前配置值

            if (var)    // 存则进行修改
            {
                if (i.second.IsScalar())
                {
                    var->fromString(i.second.Scalar());
                }
                else
                {
                    std::stringstream ss;
                    ss << i.second;
                    var->fromString(ss.str());
                }
            }
        }
    }
}