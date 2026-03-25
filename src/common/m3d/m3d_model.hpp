#pragma once

#include "resource_manager/loadable.hpp"
#include "skeleton/skeleton_editable.hpp"
#include "gpu/gpu_material.hpp"
#include "gpu/gpu_mesh.hpp"
#include "animation/animation.hpp"

#include "resource_manager/byte_writer/byte_writer.hpp"

#include "m3d_data.hpp"


struct m3dSkin {
	std::vector<std::string> bone_list;
	std::vector<gfxm::mat4> inv_bind_transforms;
};

struct m3dMesh {
	HSHARED<gpuMesh> mesh;
	int material_idx = -1;

	gfxm::vec3 bounding_sphere_origin;
	float bounding_radius = .0f;

	std::unique_ptr<m3dSkin> skin;
};

struct m3dMeshInstance {
	std::string bone_name;
	int mesh_idx = -1;
};

struct m3dDecal {
	std::string bone;
	gfxm::vec3 extents;
	uint32_t color = 0xFFFFFFFF;
	int material_idx = -1;
};

struct m3dEffects {
	std::vector<m3dDecal> decals;
	// particle emitters
	// lights
};


struct m3dModel : public ILoadable {
	ResourceRef<Skeleton>					skeleton;
	std::vector<ResourceRef<gpuMaterial>>	materials;
	std::vector<m3dMesh>					meshes;
	std::vector<m3dMeshInstance>			mesh_instances;
	std::unique_ptr<m3dEffects>				effects;
	std::vector<ResourceRef<Animation>>		animations;
	gfxm::aabb								aabb;
	gfxm::vec3								bounding_sphere_origin;
	float									bounding_radius = .0f;

	m3dModel() {}
	m3dModel(const m3dModel&) = delete;
	m3dModel& operator=(const m3dModel&) = delete;

	void clear();

	void makeFromData(m3dData&& dat);

	DEFINE_EXTENSIONS(e_m3d);
	bool load(byte_reader& reader) override;
};

