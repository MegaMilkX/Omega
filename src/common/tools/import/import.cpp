#include "import.hpp"


void importInit() {
    reflect<Importer>();
    reflect<gpuTexture2dImporter>();
    reflect<ModelImporter>();

    // TODO: make a map EXTENSION-TO-IMPORTER_TYPE_LIST
}