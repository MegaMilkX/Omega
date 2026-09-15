#include "prop_snapshot.hpp"

#include "log/log.hpp"
#include "reflection.hpp"


namespace rtti {


void PropSnapshot::clear() {
    type_ = rtti::type(0);
    props.clear();
    group_order.clear();
    group_members.clear();
}

void PropSnapshot::add(const std::string& name, rtti::varying var, const std::string& group) {
    props[name] = std::move(var);

    auto& members = group_members[group];
    if (std::find(members.begin(), members.end(), name) == members.end()) {
        members.push_back(name);
    }

    if (std::find(group_order.begin(), group_order.end(), group) == group_order.end()) {
        group_order.push_back(group);
    }
}
void PropSnapshot::add(const std::vector<std::string>& path, rtti::varying var, const std::string& group) {
    PropSnapshot* last = this;
    for (int i = 0; i < int(path.size()) - 1; ++i) {
        const std::string& name = path[i];
        auto it = last->props.find(name);
        if (it == last->props.end()) {
            it = last->props.insert(std::make_pair(name, varying::make(PropSnapshot()))).first;
        }
        if (it->second.get_type() != type_get<PropSnapshot>()) {
            it->second = varying::make(PropSnapshot());
        }
        last = it->second.get<PropSnapshot>();
    }
    last->add(path.back(), var, group);
}

varying* PropSnapshot::get_var(const std::string& key) {
    auto it = props.find(key);
    if (it == props.end()) {
        return nullptr;
    }
    return &it->second;
}
varying* PropSnapshot::get_var(const std::vector<std::string>& path) {
    PropSnapshot* last = this;
    for (int i = 0; i < int(path.size()) - 1; ++i) {
        varying* var = last->get_var(path[i]);
        if (!var) {
            return nullptr;
        }
        if (var->get_type() != rtti::type_get<PropSnapshot>()) {
            return nullptr;
        }
        last = var->get<PropSnapshot>();
    }
    return last->get_var(path.back());
}

void PropSnapshot::toJson(nlohmann::json& json) const {
    json = nlohmann::json::object();

    json["@type"] = type_.get_name();

    for (auto& kv : props) {
        const auto& name = kv.first;
        const auto& var = kv.second;

        nlohmann::json& jval = json[name];
        if (var.get_type() == rtti::type_get<PropSnapshot>()) {
            var.get<PropSnapshot>()->toJson(jval);
        } else {
            var.to_json(jval);
        }
    }
}
void PropSnapshot::fromJson(const PropSnapshot& schema, const nlohmann::json& json) {
    if (!json.is_object()) {
        LOG_ERR("PropSnapshot::fromJson(): json must be an object");
        assert(false);
        return;
    }

    type_ = schema.type_;

    for (auto it = json.begin(); it != json.end(); ++it) {
        const std::string& name = it.key();
        if (name == "@type") {
            continue;
        }

        const nlohmann::json& jval = it.value();

        auto schit = schema.props.find(name);
        if (schit == schema.props.end()) {
            continue;
        }

        if (schit->second.get_type() == rtti::type_get<PropSnapshot>()) {
            props[name] = rtti::varying::make(PropSnapshot());
            props[name].get<PropSnapshot>()->fromJson(*schit->second.get<PropSnapshot>(), jval);
        } else {
            props[name] = rtti::varying::make(schit->second.get_type());
            props[name].from_json(jval);
        }
    }
}


}

