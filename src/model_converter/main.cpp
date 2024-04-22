
#include <assert.h>
#include <fstream>
#include <string>
#include <set>
#include <vector>
#include <filesystem>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

#include <nlohmann/json.hpp>

#include "gpu/vertex_format.hpp"

#include "log/log.hpp"

#include "serialization/serialization.hpp"

#include "config.hpp"

struct MeshFileData {
    std::vector<gfxm::vec3>                 vertices;
    std::vector<gfxm::vec3>                 normals;
    std::vector<gfxm::vec3>                 tangents;
    std::vector<gfxm::vec3>                 bitangents;
    std::vector<std::vector<gfxm::vec2>>    uv_layers;
    std::vector<std::vector<uint32_t>>      rgba_layers;
    std::vector<uint32_t>                   indices;

    MeshFileData(int vertex_count = 0)
    : vertices(vertex_count, gfxm::vec3(.0f, .0f, .0f)),
    normals(vertex_count) {

    }
};
void serializeMeshFileData_v0(FILE* f, const MeshFileData& data) {
    const uint32_t tag = *(uint32_t*)"MESH";
    const uint32_t version = 0;

    const uint32_t FLAG_HAS_TANGENTS_AND_BITANGENTS = 0x0001;
    const uint32_t FLAG_HAS_UV                      = 0x0002;
    const uint32_t FLAG_HAS_RGBA                    = 0x0004;
    uint32_t flags = 0x0;
    if (!data.tangents.empty() && !data.bitangents.empty()) {
        flags |= FLAG_HAS_TANGENTS_AND_BITANGENTS;
    }
    if (!data.uv_layers.empty()) {
        flags |= FLAG_HAS_UV;
    }
    if (!data.rgba_layers.empty()) {
        flags |= FLAG_HAS_RGBA;
    }

    fwrite(&tag, sizeof(tag), 1, f);
    fwrite(&version, sizeof(version), 1, f);

    fwrite(&flags, sizeof(flags), 1, f);

    fwrite_vector(data.vertices, f);
    fwrite_vector(data.normals, f, false);
    if (flags & FLAG_HAS_TANGENTS_AND_BITANGENTS) {
        fwrite_vector(data.tangents, f, false);
        fwrite_vector(data.bitangents, f, false);
    }
    if (flags & FLAG_HAS_UV) {
        fwrite_length(data.uv_layers, f);
        for (int i = 0; i < data.uv_layers.size(); ++i) {
            auto& uvs = data.uv_layers[i];
            fwrite_vector(uvs, f, false);
        }
    }
    if (flags & FLAG_HAS_RGBA) {
        fwrite_length(data.rgba_layers, f);
        for (int i = 0; i < data.rgba_layers.size(); ++i) {
            auto& colors = data.rgba_layers[i];
            fwrite_vector(colors, f, false);
        }
    }
    fwrite_vector(data.indices, f);
}
void serializeMeshFileData(const char* path, const MeshFileData& data) {
    std::string fname = MKSTR(path << ".mesh");
    FILE* f = fopen(fname.c_str(), "wb");
    if (!f) {
        LOG_ERR("Failed to create file '" << fname << "'");
        return;
    }

    serializeMeshFileData_v0(f, data);

    fclose(f);
}

struct SkinFileData {
    MeshFileData* mesh;
    std::vector<uint32_t>       skeleton_bone_indices;
    std::vector<gfxm::mat4>     inverse_bind_transforms;
    std::vector<gfxm::ivec4>    bone_indices;
    std::vector<gfxm::vec4>     bone_weights;
};
void serializeSkinFileData_v0(FILE* f, const SkinFileData& data) {
    const uint32_t tag = *(uint32_t*)"SKIN";
    const uint32_t version = 0;

    fwrite(&tag, sizeof(tag), 1, f);
    fwrite(&version, sizeof(version), 1, f);
    
    fwrite_vector(data.skeleton_bone_indices, f);
    fwrite_vector(data.inverse_bind_transforms, f);
    fwrite_vector(data.bone_indices, f);
    fwrite_vector(data.bone_weights, f, false);

    serializeMeshFileData_v0(f, *data.mesh);
}
void serializeSkinFileData(const char* path, const SkinFileData& data) {
    std::string fname = MKSTR(path << ".skin");
    FILE* f = fopen(fname.c_str(), "wb");
    if (!f) {
        LOG_ERR("Failed to create file '" << fname << "'");
        return;
    }

    serializeSkinFileData_v0(f, data);

    fclose(f);
}

struct SkeletonAnimationFileData {
    struct KeyframeV3 {
        float time;
        gfxm::vec3 value;
    };
    struct KeyframeQ {
        float time;
        gfxm::quat value;
    };
    struct Track {
        std::string name;
        std::vector<KeyframeV3> frames_pos;
        std::vector<KeyframeQ>  frames_rot;
        std::vector<KeyframeV3> frames_scl;
    };

    float length;
    float fps;
    std::vector<Track> tracks;
};
void serializeSkeletonAnimationFileData_v0(FILE* f, const SkeletonAnimationFileData& data) {
    const uint32_t tag = *(uint32_t*)"SANM";
    const uint32_t version = 0;

    fwrite(&tag, sizeof(tag), 1, f);
    fwrite(&version, sizeof(version), 1, f);

    fwrite(&data.length, sizeof(data.length), 1, f);
    fwrite(&data.fps, sizeof(data.fps), 1, f);
    
    fwrite_length(data.tracks, f);
    for (int i = 0; i < data.tracks.size(); ++i) {
        auto& track = data.tracks[i];
        fwrite_string(track.name, f);
        fwrite_vector(track.frames_pos, f);
        fwrite_vector(track.frames_rot, f);
        fwrite_vector(track.frames_scl, f);
    }
}
void serializeSkeletonAnimationFileData(const char* path, const SkeletonAnimationFileData& data) {
    std::string fname = MKSTR(path << ".sanim");
    FILE* f = fopen(fname.c_str(), "wb");
    if (!f) {
        LOG_ERR("Failed to create file '" << fname << "'");
        return;
    }

    serializeSkeletonAnimationFileData_v0(f, data);

    fclose(f);
}

struct SkeletonFileData {
    std::vector<uint32_t>           parents;
    std::vector<gfxm::mat4>         default_pose;    
    std::map<std::string, uint32_t> node_name_to_index;

    int findBone(const char* bone_name) const {
        auto it = node_name_to_index.find(bone_name);
        if (it == node_name_to_index.end()) {
            return -1;
        }
        return it->second;
    }
};
void serializeSkeletonFileData_v0(FILE* f, const SkeletonFileData& data) {
    const uint32_t tag = *(uint32_t*)"SKEL";
    const uint32_t version = 0;

    fwrite(&tag, sizeof(tag), 1, f);
    fwrite(&version, sizeof(version), 1, f);

    fwrite_vector(data.parents, f);
    fwrite_vector(data.default_pose, f, false);
    for (auto& kv : data.node_name_to_index) {
        fwrite_string(kv.first, f);
        fwrite(&kv.second, sizeof(kv.second), 1, f);
    }
}
void serializeSkeletonFileData(const char* path, const SkeletonFileData& data) {
    std::string fname = MKSTR(path << ".skeleton");
    FILE* f = fopen(fname.c_str(), "wb");
    if (!f) {
        LOG_ERR("Failed to create file '" << fname << "'");
        return;
    }

    serializeSkeletonFileData_v0(f, data);

    fclose(f);
}


bool loadFile(const char* path, std::vector<char>& bytes) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        LOG_ERR("Failed to open file: " << path);
        return false;
    }
    std::streamsize file_size = file.tellg();

    file.seekg(0, std::ios::beg);
    bytes.resize(file_size);
    file.read(&bytes[0], file_size);

    return true;
}

#include "import/import_settings_fbx.hpp"

#include "filesystem/filesystem.hpp"

#include "platform/platform.hpp"
#include "resource/resource.hpp"
#include "gpu/gpu.hpp"

#include "gpu/pipeline/gpu_pipeline_default.hpp"

#include "gpu/readwrite/rw_gpu_material.hpp"
#include "gpu/readwrite/rw_gpu_mesh.hpp"
#include "gpu/readwrite/rw_gpu_texture_2d.hpp"
#include "animation/readwrite/rw_animation.hpp"

#include "skeleton/skeleton_editable.hpp"
#include "skeletal_model/skeletal_model.hpp"
#include "import/assimp_load_skeletal_model.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Not enough arguments");
        assert(false);
        return -1;
    }
    for (int i = 0; i < argc; ++i) {
        LOG(argv[i]);
    }

    std::string model_path = argv[1];
    
    std::filesystem::path path(model_path);
    if (!path.has_extension()) {
        LOG_ERR("Path has no extension: " << model_path);
    }

    if (path.extension() == ".import") {
        platformInit(false);
        resInit();
        std::unique_ptr<build_config::gpuPipelineCommon> pipeline(new build_config::gpuPipelineCommon);
        gpuInit();

        // Do import
        LOG("Importing...");
        nlohmann::json j;
        std::ifstream f(path);
        j << f;
        f.close();
        ImportSettingsFbx settings;
        settings.from_json(j);

        settings.do_import();

        gpuCleanup();
        resCleanup();
        platformCleanup();

        LOG_DBG("Import process completed. Press Enter...");
        getchar();
    } else {
        // Make an import file
        LOG("Making an import file...");
        ImportSettingsFbx settings;
        if (!settings.from_source(path.string())) {
            return -2;
        }

        nlohmann::json j;
        settings.to_json(j);
        path.replace_extension("import");
        std::ofstream f(path);
        f << j.dump(4);
        f.close();
    }

    return 0;
}