#pragma once

#include <stdint.h>

constexpr uint32_t M3D_TAG = 'M' | ('3' << 8) | ('D' << 16) | ('\0' << 24);
constexpr uint32_t M3D_VERSION = 1;

#pragma pack(push, 1)
struct M3D_HEAD {
	uint32_t tag = 0;
	uint32_t version = 0;

	uint32_t offs_bounds = 0;
	uint32_t offs_skeleton = 0;
	uint32_t offs_materials = 0;
	uint32_t offs_meshes = 0;
	uint32_t offs_mesh_instances = 0;
	uint32_t offs_effects = 0;
	uint32_t offs_animations = 0;
};
#pragma pack(pop)

