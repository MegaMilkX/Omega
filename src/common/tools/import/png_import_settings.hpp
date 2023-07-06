#pragma once

#include "import_settings.hpp"


class PNG_ImportSettings : public ImportSettings {
    TYPE_ENABLE();
public:
    std::string source_path;
    std::string target_path;
    int channels = 4;
    gfxm::vec2 target_size;
};
REFLECT(PNG_ImportSettings) {
    type_register<PNG_ImportSettings>("PNG_ImportSettings")
        .prop("source_path", &PNG_ImportSettings::source_path)
        .prop("target_path", &PNG_ImportSettings::target_path)
        .prop("channels", &PNG_ImportSettings::channels)
        .prop("target_size", &PNG_ImportSettings::target_size);
}
