#pragma once

// Unified asset file

#include <string>
#include <type_traits>
#include "log/log.hpp"
#include "filesystem/filesystem.hpp"
#include "nlohmann/json.hpp"

class IImportedAsset {
public:
    virtual ~IImportedAsset() {}

    virtual void serializeJson(nlohmann::json& json) const = 0;
    virtual bool deserializeJson(const nlohmann::json& json) = 0;
};
class IDesignAsset {
public:
    virtual ~IDesignAsset() {}

    virtual void serializeJson(nlohmann::json& json) const = 0;
    virtual bool deserializeJson(const nlohmann::json& json) = 0;
};
class IRuntimeAsset {
public:
    virtual ~IRuntimeAsset() {}

    virtual void serializeJson(nlohmann::json& json) const = 0;
    virtual bool deserializeJson(const nlohmann::json& json) = 0;
};


class UAF {
public:
    virtual ~UAF() {}
    static bool loadJson(const std::string& path, nlohmann::json& json) {
        std::vector<uint8_t> bytes;
        if (fsSlurpFile(path, bytes)) {
            try {
                json = nlohmann::json::parse(bytes);
            } catch(std::exception& ex) {
                LOG_ERR("Failed to read uaf file: " << ex.what());
                return false;
            }
        }
        return true;
    }
};

template<typename IMPORT_T, typename DESIGN_T, typename RUNTIME_T>
class UAF_T : public UAF {
    static_assert(std::is_base_of<IImportedAsset, IMPORT_T>::value, "IMPORT_T must derive from IImportedAsset");
    static_assert(std::is_base_of<IDesignAsset, DESIGN_T>::value, "DESIGN_T must derive from IDesignAsset");
    static_assert(std::is_base_of<IRuntimeAsset, RUNTIME_T>::value, "RUNTIME_T must derive from IRuntimeAsset");
public:
    typedef IMPORT_T import_t;
    typedef DESIGN_T design_t;
    typedef RUNTIME_T runtime_t;

    template<typename T>
    static bool load(const std::string& path, T* out);

    static bool loadImported(const std::string& path, IMPORT_T* imported);
    static bool loadDesign(const std::string& path, DESIGN_T* design);
    static bool loadRuntime(const std::string& path, RUNTIME_T* runtime);

    bool buildRuntime(RUNTIME_T* runtime);

    static bool saveFile(const std::string& path, IMPORT_T* imported, DESIGN_T* design, RUNTIME_T* runtime);
};


template<typename IMPORT_T, typename DESIGN_T, typename RUNTIME_T>
template<typename T>
bool UAF_T<IMPORT_T, DESIGN_T, RUNTIME_T>::load(const std::string& path, T* out) {
    static_assert(std::is_same<T, IMPORT_T>::value || std::is_same<T, DESIGN_T>::value || std::is_same<T, RUNTIME_T>::value, "T must be one of IMPORT_T, DESIGN_T or RUNTIME_T");
    // NOTE: Since import, design and runtime are allowed to be of the same type,
    // we check if it matches the RUNTIME_T first,
    // since if you would have the same type as runtime and imported,
    // you would probably want the runtime version if you use this function
    if (std::is_same<T, RUNTIME_T>::value) {
        if (loadRuntime(path, reinterpret_cast<RUNTIME_T*>(out))) {
            return true;
        }
    }
    if (std::is_same<T, IMPORT_T>::value) {
        if (loadImported(path, reinterpret_cast<IMPORT_T*>(out))) {
            return true;
        }
    }
    if(std::is_same<T, DESIGN_T>::value) {
        if (loadDesign(path, reinterpret_cast<DESIGN_T*>(out))) {
            return true;
        }
    }
    // NOTE: Unreachable
    return false;
}


template<typename IMPORT_T, typename DESIGN_T, typename RUNTIME_T>
bool UAF_T<IMPORT_T, DESIGN_T, RUNTIME_T>::loadImported(const std::string& path, IMPORT_T* imported) {
    nlohmann::json json;
    if (!loadJson(path, json)) {
        return false;
    }
    return imported->deserializeJson(json["imported"]);
}
template<typename IMPORT_T, typename DESIGN_T, typename RUNTIME_T>
bool UAF_T<IMPORT_T, DESIGN_T, RUNTIME_T>::loadDesign(const std::string& path, DESIGN_T* design) {
    nlohmann::json json;
    if (!loadJson(path, json)) {
        return false;
    }
    return design->deserializeJson(json["design"]);
}
template<typename IMPORT_T, typename DESIGN_T, typename RUNTIME_T>
bool UAF_T<IMPORT_T, DESIGN_T, RUNTIME_T>::loadRuntime(const std::string& path, RUNTIME_T* runtime) {
    nlohmann::json json;
    if (!loadJson(path, json)) {
        return false;
    }
    return runtime->deserializeJson(json["runtime"]);
}

template<typename IMPORT_T, typename DESIGN_T, typename RUNTIME_T>
bool UAF_T<IMPORT_T, DESIGN_T, RUNTIME_T>::saveFile(const std::string& path, IMPORT_T* imported, DESIGN_T* design, RUNTIME_T* runtime) {
    nlohmann::json j = nlohmann::json::object();

    {
        std::vector<uint8_t> bytes;
        if (fsSlurpFile(path, bytes)) {
            try {
                j = nlohmann::json::parse(bytes);
            } catch(std::exception& ex) {
                LOG_ERR("Failed to read uaf file: " << ex.what());
                return false;
            }
        }
    }

    nlohmann::json& jimported = j["imported"];
    nlohmann::json& jdesign = j["design"];
    nlohmann::json& jruntime = j["runtime"];
    if (imported) {
        jimported.clear();
        imported->serializeJson(jimported);
    }
    if (design) {
        jdesign.clear();
        design->serializeJson(jdesign);
    }
    if (runtime) {
        jruntime.clear();
        runtime->serializeJson(jruntime);
    }

    std::string dump = j.dump(4);
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) {
        assert(false);
        return false;
    }
    size_t nwritten = fwrite(dump.data(), 1, dump.size(), f);
    fclose(f);

    return nwritten == dump.size();
}
