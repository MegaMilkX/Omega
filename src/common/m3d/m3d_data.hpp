#pragma once

#include <stdint.h>
#include <string>
#include "resource_manager/byte_reader/byte_reader.hpp"
#include "resource_manager/byte_writer/byte_writer.hpp"
#include "math/gfxm.hpp"
#include "skeleton/skeleton_editable.hpp"
#include "gpu/gpu_material.hpp"
#include "animation/animation.hpp"


struct m3dSkinData {
    std::vector<gfxm::ivec4>    bone_indices;
    std::vector<gfxm::vec4>     bone_weights;
    std::vector<gfxm::mat4>     inverse_bind_transforms;
    std::vector<uint32_t>       bone_transform_source_indices;
};

struct m3dMeshData {
    int                         material_idx = 0;
    gfxm::aabb                  aabb;
    gfxm::vec3                  bounding_sphere_origin;
    float                       bounding_radius;

    std::vector<uint32_t>                   indices;

    std::vector<gfxm::vec3>                 vertices;
    std::vector<std::vector<uint32_t>>      rgba_channels;
    std::vector<std::vector<gfxm::vec2>>    uv_channels;
    std::vector<gfxm::vec3>                 normals;
    std::vector<gfxm::vec3>                 tangents;
    std::vector<gfxm::vec3>                 bitangents;
    std::unique_ptr<m3dSkinData>            skin_data;

    bool read(byte_reader& in);
    void write(byte_writer& out);
    bool write(const std::string& path);
};

struct m3dMeshInstanceData {
    std::string bone_name;
    int mesh_idx = -1;
};

struct m3dData {
	ResourceRef<Skeleton>				    skeleton;
	std::vector<ResourceRef<gpuMaterial>>	materials;
	std::vector<m3dMeshData>			    meshes;
    std::vector<m3dMeshInstanceData>        mesh_instances;
	std::vector<ResourceRef<Animation>>	    animations;
	gfxm::aabb							    aabb;
	gfxm::vec3							    bounding_sphere_origin;
	float								    bounding_radius = .0f;

    bool read(byte_reader& in);
    void write(byte_writer& out);
    bool write(const std::string& path);
};