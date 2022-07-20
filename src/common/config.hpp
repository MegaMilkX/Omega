#pragma once

/*

    Common parameters shared between the game and tools

*/

#include "gpu/pipeline/gpu_pipeline_default.hpp"

namespace build_config {

    // Must use the same rendering pipeline between game and tools
    using gpuPipelineCommon = gpuPipelineDefault;

    const char* const default_import_shader = "shaders/vertex_color.glsl";

};