#pragma once

#include <fstream>
#include "log/log.hpp"
#include "nlohmann/json.hpp"


struct ImportSettings {
    std::string import_file_path;

    virtual ~ImportSettings() {}

    bool file_exists(const std::string& path) {
        std::ifstream f(path);
        return f.good();
    }

    virtual bool from_source(const std::string& spath) = 0;
    virtual void to_json(nlohmann::json& j) = 0;
    virtual void from_json(const nlohmann::json& j) = 0;

    bool write_import_file() {
        nlohmann::json j;
        to_json(j);
        std::ofstream f(import_file_path);
        if (!f.is_open()) {
            LOG_ERR("Failed to open " << import_file_path);
            return false;
        }
        f << j.dump(4);
        f.close();
        LOG("Import file '" << import_file_path << "' written");
        return true;
    }
    bool read_import_file(const std::string& spath) {
        nlohmann::json j;
        std::ifstream f(spath);
        if (!f.is_open()) {
            LOG_ERR("Failed to open " << spath);
            return false;
        }
        j << f;
        f.close();
        from_json(j);
        import_file_path = spath;
        return true;
    }
    virtual bool do_import() = 0;
};