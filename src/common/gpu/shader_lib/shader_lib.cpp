#include "shader_lib.hpp"

#include <map>
#include <unordered_map>
#include "gpu/gpu_shader_program.hpp"
#include "filesystem/filesystem.hpp"

enum SHADER_LIB_EDGE_TYPE {
    SHADER_LIB_EDGE_HOST_VERT,
    SHADER_LIB_EDGE_HOST_FRAG,
    SHADER_LIB_EDGE_VERT,
    //SHADER_LIB_EDGE_GEOM,
    SHADER_LIB_EDGE_FRAG,
    //SHADER_LIB_EDGE_FLAGS,

    SHADER_LIB_EDGE_COUNT
};

struct ShaderSource {
    SHADER_TYPE type;
    std::string source;
    std::string path;
};
static std::map<std::string, std::unique_ptr<ShaderSource>> s_shader_sources;

struct ShaderLibNode {
    std::unordered_map<uint64_t, std::unique_ptr<ShaderLibNode>> children;
    RHSHARED<gpuShaderProgram> program;
};
static std::unique_ptr<ShaderLibNode> s_root;

static ShaderSource* loadShaderSourceImpl(const char* path) {
    std::string src;
    if (!fsSlurpTextFile(path, src)) {
        return 0;
    }
    ShaderSource* shader_source = new ShaderSource;
    shader_source->type = SHADER_UNKNOWN; // TODO
    shader_source->source = src;
    shader_source->path = path;
    return shader_source;
}

static ShaderSource* loadShaderSource(const char* path) {
    if (path == nullptr) {
        return 0;
    }

    auto it = s_shader_sources.find(path);
    if (it == s_shader_sources.end()) {
        auto shader = loadShaderSourceImpl(path);
        if (!shader) {
            LOG_ERR("Failed to load shader source");
            assert(false);
            return 0;
        }
        it = s_shader_sources.insert(
            std::make_pair(
                std::string(path),
                std::unique_ptr<ShaderSource>(shader)
            )
        ).first;
    }
    return it->second.get();
}

struct SHADER_LIB_LOAD_CONTEXT {
    ShaderSource* host_vertex;
    ShaderSource* host_fragment;
    ShaderSource* vertex;
    ShaderSource* fragment;
};

static ShaderLibNode* insertShaderSource(ShaderLibNode* node, const char* path, ShaderSource** out) {
    ShaderSource* shader = loadShaderSource(path);
    auto it = node->children.find((uint64_t)shader);
    if (it == node->children.end()) {
        it = node->children.insert(
            std::make_pair(
                (uint64_t)shader,
                std::unique_ptr<ShaderLibNode>(new ShaderLibNode)
            )
        ).first;
    }
    *out = shader;
    return it->second.get();
}

ShaderLibNode* shaderLibTraverseTree(
    SHADER_LIB_EDGE_TYPE edge_type, 
    const SHADER_LIB_LOAD_PARAMS& params,
    SHADER_LIB_LOAD_CONTEXT* ctx,
    ShaderLibNode* node
) {
    ShaderLibNode* ret = 0;

    switch (edge_type) {
    case SHADER_LIB_EDGE_HOST_VERT: {
        ret = insertShaderSource(node, params.host_vertex_path, &ctx->host_vertex);
        break;
    }
    case SHADER_LIB_EDGE_HOST_FRAG: {
        ret = insertShaderSource(node, params.host_fragment_path, &ctx->host_fragment);
        break;
    }
    case SHADER_LIB_EDGE_VERT: {
        ret = insertShaderSource(node, params.vertex_path, &ctx->vertex);
        break;
    }
    case SHADER_LIB_EDGE_FRAG: {
        ret = insertShaderSource(node, params.fragment_path, &ctx->fragment);
        break;
    }
    default: {
        LOG_ERR("Unimplemented shader lib edge");
        assert(false);
        return 0;
    }
    }

    return ret;
}

RHSHARED<gpuShaderProgram> shaderLibLoadProgram(const SHADER_LIB_LOAD_PARAMS& params) {
#define CSTRLOG(CSTR) (CSTR ? CSTR : "null")
    LOG("shaderLibLoadProgram" <<
        "\n\thost_vertex_path: " << CSTRLOG(params.host_vertex_path)
        << "\n\thost_fragment_path: " << CSTRLOG(params.host_fragment_path)
        << "\n\tvertex_path: " << CSTRLOG(params.vertex_path)
        << "\n\tgeometry_path: " << CSTRLOG(params.geometry_path)
        << "\n\tfragment_path: " << CSTRLOG(params.fragment_path)
        << "\n\tflags: " << params.flags
    )
#undef CSTRLOG

    if (!s_root) {
        s_root.reset(new ShaderLibNode);
    }

    SHADER_LIB_LOAD_CONTEXT ctx = { 0 };
    ShaderLibNode* node = s_root.get();
    for (int i = 0; i < SHADER_LIB_EDGE_COUNT; ++i) {
        node = shaderLibTraverseTree((SHADER_LIB_EDGE_TYPE)i, params, &ctx, node);
        if (!node) {
            LOG_ERR("ShaderLibNode* can not be null");
        }
        assert(node);
    }

    if (node->program) {
        LOG("Found existing program");
        return node->program;
    }

    LOG("Building a new program");

    std::vector<std::string> paths;
    std::vector<const char*> cpaths;
    std::string program_source;
    if (ctx.host_vertex) {
        program_source += "#vertex\n";
        program_source += ctx.host_vertex->source;

        std::filesystem::path file_dir = ctx.host_vertex->path;
        file_dir = file_dir.parent_path();
        std::string str_file_dir = file_dir.string();
        paths.push_back(str_file_dir);
    }
    if (ctx.host_fragment) {
        program_source += "#fragment\n";
        program_source += ctx.host_fragment->source;

        std::filesystem::path file_dir = ctx.host_fragment->path;
        file_dir = file_dir.parent_path();
        std::string str_file_dir = file_dir.string();
        paths.push_back(str_file_dir);
    }
    if (ctx.vertex) {
        program_source += "#vertex\n";
        program_source += ctx.vertex->source;

        std::filesystem::path file_dir = ctx.vertex->path;
        file_dir = file_dir.parent_path();
        std::string str_file_dir = file_dir.string();
        paths.push_back(str_file_dir);
    }
    if (ctx.fragment) {
        program_source += "#fragment\n";
        program_source += ctx.fragment->source;

        std::filesystem::path file_dir = ctx.fragment->path;
        file_dir = file_dir.parent_path();
        std::string str_file_dir = file_dir.string();
        paths.push_back(str_file_dir);
    }

    cpaths.resize(paths.size());
    for (int i = 0; i < paths.size(); ++i) {
        cpaths[i] = paths[i].c_str();
    }

    GLX_PP_CONTEXT pp_ctx = { 0 };
    pp_ctx.include_paths = cpaths.data();
    pp_ctx.n_include_paths = cpaths.size();
    auto program_handle = createProgram(pp_ctx, program_source.c_str(), program_source.size());
    program_handle->init();

    node->program.reset(program_handle);

    {
        const std::string dir = "./shader_program_cache/";
        fsCreateDirRecursive(dir);
        FILE* f = fopen((dir + "/test.glsl").c_str(), "wb");
        if(f) {
            fwrite(program_source.data(), program_source.size(), 1, f);
            fclose(f);
        } else {
            LOG_ERR("Failed to open file");
        }
    }

    return node->program;
}

