#pragma once

#include <map>
#include <vector>
#include "nlohmann/json.hpp"
#include "reflection/reflection.hpp"
#include "resource_manager/loadable.hpp"

[[cppi_decl, no_reflect]];
struct ActorPrefab;

class Actor;
struct ActorPrefab : public ILoadable {
    struct ComponentBlueprint {
        std::map<property, varying> properties;
    };
    struct DriverBlueprint {
        std::map<property, varying> properties;
    };
    struct NodeBlueprint {
        type t;
        std::vector<NodeBlueprint> children;
        std::map<property, varying> properties;
        void clear() {
            t = type(0);
            children.clear();
            properties.clear();
        }
    };

    std::map<type, ComponentBlueprint> components;
    std::map<type, DriverBlueprint> drivers;
    NodeBlueprint root_node;

    Actor* instantiate() const;

    void nodeToJson(nlohmann::json& j, const NodeBlueprint& node) {
        j["@type"] = node.t.get_name();
        auto& props = node.properties;
        nlohmann::json& jprops = j["@props"];
        jprops = nlohmann::json::object();
        for (auto& kv : props) {
            nlohmann::json& jprop = jprops[kv.first.get_name()];
            assert(kv.second.get_type().is_valid());
            kv.second.to_json(jprop);
        }
        nlohmann::json& jchildren = j["@children"];
        jchildren = nlohmann::json::array();
        for (int i = 0; i < node.children.size(); ++i) {
            auto& ch = node.children[i];
            nlohmann::json& jchild = jchildren.emplace_back();
            nodeToJson(jchild, ch);
        }
    }
    void toJson(nlohmann::json& j) {
        j = nlohmann::json::object();
        
        nlohmann::json& jcomponent_array = j["components"];
        for (auto& kv : components) {
            nlohmann::json& jcomponent = jcomponent_array.emplace_back();
            jcomponent["@type"] = kv.first.get_name();
            
            auto& props = kv.second.properties;
            nlohmann::json& jprops = jcomponent["@props"];
            jprops = nlohmann::json::object();
            for (auto& kv : props) {
                nlohmann::json& jprop = jprops[kv.first.get_name()];
                kv.second.to_json(jprop);
            }
        }

        nlohmann::json& jdriver_array = j["drivers"];
        for (auto& kv : drivers) {
            nlohmann::json& jdriver = jdriver_array.emplace_back();
            jdriver["@type"] = kv.first.get_name();

            auto& props = kv.second.properties;
            nlohmann::json& jprops = jdriver["@props"];
            jprops = nlohmann::json::object();
            for (auto& kv : props) {
                nlohmann::json& jprop = jprops[kv.first.get_name()];
                kv.second.to_json(jprop);
            }
        }

        nlohmann::json& jnode = j["root"];
        nodeToJson(jnode, root_node);
    }

    template<typename T>
    T json_get(const nlohmann::json& j, const char* key, const T& default_value = T()) {
        T out = default_value;
        auto it = j.find(key);
        if(it == j.end()) return out;
        try {
            out = it->get<T>();
        } catch (...) {
            // TODO:
        }
        return out;
    }

    void nodeFromJson(const nlohmann::json& jnode, NodeBlueprint& node) {
        std::string stype = json_get<std::string>(jnode, "@type", "");
        LOG("type: " << stype);
        type t = type_get(stype.c_str());
        
        node.t = t;

        const auto& it_props = jnode.find("@props");
        if(it_props != jnode.end()) {
            const nlohmann::json& jprops = it_props.value();
            propsFromJson(jprops, t, node.properties);
        }

        const auto& it_children = jnode.find("@children");
        if (it_children != jnode.end()) {
            const nlohmann::json& jchildren = it_children.value();
            assert(jchildren.is_array());
            for (int i = 0; i < jchildren.size(); ++i) {
                const nlohmann::json& jchild = jchildren[i];
                auto& child = node.children.emplace_back();
                nodeFromJson(jchild, child);
            }
        }
    }

    void propsFromJson(const nlohmann::json& jprops, type t, std::map<property, varying>& props) {
        /*const auto& parent_types = t.get_desc()->parent_types;
        for (const auto& parent_info : parent_types) {
            propsFromJson(jprops, parent_info.parent_type, props);
        }*/

        for (int i = 0; i < t.prop_count(); ++i) {
            property prop = t.get_property(i);
            const auto& it_prop = jprops.find(prop.get_name());
            if (it_prop != jprops.end()) {
                const nlohmann::json& jprop = it_prop.value();
                auto& var = props[prop];
                var = varying::make(prop.get_type());
                var.from_json(jprop);

                {
                    nlohmann::json j;
                    var.to_json(j);
                    LOG_DBG(prop.get_name() << ": " << j.dump(-1));
                }
            }
        }
    }

    DEFINE_EXTENSIONS(e_apf);
    bool load(byte_reader& reader) override {
        components.clear();
        drivers.clear();
        root_node.clear();

        LOG("Loading an actor prefab");
        auto view = reader.try_slurp();
        if (!view) {
            return false;
        }
        std::string str_json(view.data, view.data + view.size);
        nlohmann::json json = nlohmann::json::parse(str_json);
        if (!json.is_object()) {
            return false;
        }
        
        auto it_components = json.find("components");
        if (it_components != json.end()) {
            LOG("Components");
            const nlohmann::json& jcomponents = it_components.value();
            assert(jcomponents.is_array() || jcomponents.is_null());
            for (const nlohmann::json& jcomponent : jcomponents) {
                assert(jcomponent.is_object());
                std::string stype = json_get<std::string>(jcomponent, "@type", "");
                LOG("type: " << stype);
                type t = type_get(stype.c_str());
                ComponentBlueprint& comp_blp = components[t];

                const auto& it_props = jcomponent.find("@props");
                if(it_props != jcomponent.end()) {
                    const nlohmann::json& jprops = it_props.value();
                    propsFromJson(jprops, t, comp_blp.properties);
                }
            }
        }

        auto it_drivers = json.find("drivers");
        if (it_drivers != json.end()) {
            LOG("Drivers");
            const nlohmann::json& jdrivers = it_drivers.value();
            assert(jdrivers.is_array());
            assert(jdrivers.is_array());
            for (const nlohmann::json& jdriver : jdrivers) {
                assert(jdriver.is_object());
                std::string stype = json_get<std::string>(jdriver, "@type", "");
                LOG("type: " << stype);
                type t = type_get(stype.c_str());
                DriverBlueprint& drv_blp = drivers[t];
                
                const auto& it_props = jdriver.find("@props");
                if(it_props != jdriver.end()) {
                    const nlohmann::json& jprops = it_props.value();
                    propsFromJson(jprops, t, drv_blp.properties);
                }
            }
        }

        auto it_root = json.find("root");
        if (it_root != json.end()) {
            LOG("Nodes");
            const nlohmann::json& jroot = it_root.value();
            assert(jroot.is_object());
            nodeFromJson(jroot, root_node);
        }

        return true;
    }
};

