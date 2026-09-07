#pragma once

#include <vector>
#include <map>
#include <string>
#include "type.hpp"
#include "varying.hpp"

#include "nlohmann/json.hpp"


namespace rtti {


struct PropSnapshot {
    rtti::type type_;
    std::map<std::string, rtti::varying> props;

    // group "" is the default group and should always be present, unless there's no props at all
    std::vector<std::string> group_order; // groups in display order
    std::map<std::string, std::vector<std::string>> group_members; // group -> props in display order

    void clear() {
        type_ = rtti::type(0);
        props.clear();
        group_order.clear();
        group_members.clear();
    }

    void add(const std::string& name, rtti::varying var, const std::string& group = "") {
        props[name] = std::move(var);

        auto& members = group_members[group];
        if (std::find(members.begin(), members.end(), name) == members.end()) {
            members.push_back(name);
        }

        if (std::find(group_order.begin(), group_order.end(), group) == group_order.end()) {
            group_order.push_back(group);
        }
    }

    varying* get_var(const std::string& key) {
        auto it = props.find(key);
        if (it == props.end()) {
            return nullptr;
        }
        return &it->second;
    }

    template<typename T>
    T* get(const std::string& key) const {
        auto it = props.find(key);
        if (it == props.end()) {
            return nullptr;
        }

        const rtti::varying& var = it->second;
        return const_cast<T*>(var.get<T>());
    }

    void toJson(nlohmann::json&) const;
    void fromJson(const PropSnapshot& schema, const nlohmann::json&);
};


}

