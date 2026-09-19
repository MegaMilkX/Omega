#pragma once

#include <map>
#include <vector>
#include "nlohmann/json.hpp"
#include "reflection/reflection.hpp"
#include "resource_manager/resource.hpp"
#include "resource_manager/loadable.hpp"

[[cppi_decl, no_reflect]];
struct ActorPrefab;

class Actor;
struct ActorPrefab
: public Resource
, public ILoadable {
    struct NodeBlueprint {
        rtti::PropSnapshot snap;
        std::vector<NodeBlueprint> children;
        void clear() {
            children.clear();
            snap.clear();
        }
    };

    rtti::PropSnapshot snapshot;
    std::vector<rtti::PropSnapshot> drivers;
    NodeBlueprint root_node;

    Actor* instantiate() const;

    void nodeToJson(nlohmann::json& j, const NodeBlueprint& node) {        
        node.snap.toJson(j);

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

        snapshot.toJson(j);

        nlohmann::json& jdriver_array = j["@drivers"];
        for (int i = 0; i < drivers.size(); ++i) {
            auto& snap = drivers[i];

            nlohmann::json& jdriver = jdriver_array.emplace_back();
            snap.toJson(jdriver);
        }

        nlohmann::json& jnode = j["@root"];
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
        rtti::read_snapshot(jnode, node.snap);

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

    void propsFromJson(const nlohmann::json& jprops, rtti::type t, std::map<rtti::property, rtti::varying>& props) {
        /*const auto& parent_types = t.get_desc()->parent_types;
        for (const auto& parent_info : parent_types) {
            propsFromJson(jprops, parent_info.parent_type, props);
        }*/

        for (int i = 0; i < t.prop_count(); ++i) {
            rtti::property prop = t.get_property(i);
            const auto& it_prop = jprops.find(prop.get_name());
            if (it_prop != jprops.end()) {
                const nlohmann::json& jprop = it_prop.value();
                auto& var = props[prop];
                var = rtti::varying::make(prop.get_type());
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

        rtti::read_snapshot(json, snapshot);

        auto it_drivers = json.find("@drivers");
        if (it_drivers != json.end()) {
            LOG("Drivers");
            const nlohmann::json& jdrivers = it_drivers.value();
            assert(jdrivers.is_array());
            for (const nlohmann::json& jdriver : jdrivers) {
                rtti::PropSnapshot snap;
                if (!rtti::read_snapshot(jdriver, snap)) {
                    continue;
                }
                drivers.emplace_back(std::move(snap));
            }
        }

        auto it_root = json.find("@root");
        if (it_root != json.end()) {
            LOG("Nodes");
            const nlohmann::json& jroot = it_root.value();
            assert(jroot.is_object());
            nodeFromJson(jroot, root_node);
        }

        return true;
    }
};

