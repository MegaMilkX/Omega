#pragma once

#include "importer.hpp"


class gpuTexture2dImporter : public Importer {
public:
    TYPE_ENABLE();
    std::string target;
    int channels = 4;
};
REFLECT(gpuTexture2dImporter) {
    type_register<gpuTexture2dImporter>("gpuTexture2dImporter")
        .parent<Importer>()
        //.attrib(SourceExtensions({ EXT_PNG, EXT_JPG, EXT_JPEG, EXT_TGA }))
        .prop("target", &gpuTexture2dImporter::target)
        .prop("channels", &gpuTexture2dImporter::channels);
}