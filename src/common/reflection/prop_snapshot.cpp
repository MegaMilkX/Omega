#include "prop_snapshot.hpp"

#include "log/log.hpp"
#include "reflection.hpp"


namespace rtti {


void PropSnapshot::toJson(nlohmann::json& json) const {
    json = nlohmann::json::object();

    json["@type"] = type_.get_name();

    for (auto& kv : props) {
        const auto& name = kv.first;
        const auto& var = kv.second;

        nlohmann::json& jval = json[name];
        var.to_json(jval);
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

        props[name] = rtti::varying::make(schit->second.get_type());
        props[name].from_json(jval);
    }
}


}

