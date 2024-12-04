#include "json.hpp"

#include <fstream>
#include "log/log.hpp"

nlohmann::json jsonLoad(const char* path) {
    std::ifstream f(path);
    if (!f) {
        LOG_ERR("jsonLoad: json file not found '" << path << "'");
        assert(false);
        return nullptr;
    }
    nlohmann::json json;
    f >> json;
    return json;
}

nlohmann::json jsonLoadWithExtensions(const char* path) {
    nlohmann::json j = jsonLoad(path);
    if (j.is_null()) {
        return nullptr;
    }
    return jsonPreprocessExtensions(j);
}

nlohmann::json jsonPreprocessExtensions(const nlohmann::json& json) {
    if (!json.is_object()) {
        LOG_ERR("jsonPreprocessExtensions: json expected to be an object");
        assert(false);
        return json;
    }

    auto j = json.find("$extends");
    if (j == json.end()) {
        return json;
    }

    auto jextends = j.value();
    if (!jextends.is_string()) {
        LOG_ERR("$extends expected to be a string");
        assert(false);
        return json;
    }

    std::string path = jextends.get<std::string>();
    nlohmann::json parent_json = jsonLoad(path.c_str());
    if (parent_json.is_null()) {
        return json;
    }

    parent_json = jsonPreprocessExtensions(parent_json);
    parent_json.merge_patch(json);
    return parent_json;
}
