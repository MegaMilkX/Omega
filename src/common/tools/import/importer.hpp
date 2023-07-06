#pragma once

#include "reflect.hpp"

class Importer {
public:
    TYPE_ENABLE_BASE();

    std::wstring source_path;

    virtual ~Importer() {}
};
REFLECT(Importer) {
    type_register<Importer>("Importer");
        //.prop("source_path", &Importer::source_path);
}