#include "m3d_model.hpp"

#include "resource_manager/byte_writer/file_writer.hpp"
#include "resource_manager/byte_writer/vector_writer.hpp"
#include "resource_manager/byte_reader/memory_reader.hpp"
#include "m3d/m3d_data.hpp"


void m3dModel::clear() {
	skeleton.reset();
	materials.clear();
	meshes.clear();
	effects.reset();
	animations.clear();
}

void m3dModel::makeFromData(m3dData&& dat) {
	aabb = dat.aabb;
	bounding_sphere_origin = dat.bounding_sphere_origin;
	bounding_radius = dat.bounding_radius;

	skeleton = std::move(dat.skeleton);
	materials = std::move(dat.materials);
	// TODO: effects
	animations = std::move(dat.animations);

	// meshes
	meshes.resize(dat.meshes.size());
	for (int i = 0; i < dat.meshes.size(); ++i) {
		auto m3d_md = &dat.meshes[i];
		auto m3d_mesh = &meshes[i];

		m3d_mesh->material_idx = m3d_md->material_idx;
		m3d_mesh->bounding_sphere_origin = m3d_md->bounding_sphere_origin;
		m3d_mesh->bounding_radius = m3d_md->bounding_radius;

		Mesh3d mesh3d;
		mesh3d.setAttribArray(VFMT::Position_GUID, m3d_md->vertices);
		std::vector<uint8_t> rgb;
		rgb.resize(mesh3d.getVertexCount() * 3);
		for (int j = 0; j < mesh3d.getVertexCount(); ++j) {
			uint32_t rgba = m3d_md->rgba_channels[0][j];
			rgb[j * 3] = rgba & 0xFF;
			rgb[j * 3 + 1] = (rgba & 0xFF00) >> 8;
			rgb[j * 3 + 2] = (rgba & 0xFF0000) >> 16;
		}
		mesh3d.setAttribArray(VFMT::ColorRGB_GUID, rgb);
		mesh3d.setAttribArray(VFMT::UV_GUID, m3d_md->uv_channels[0]);
		mesh3d.setAttribArray(VFMT::Normal_GUID, m3d_md->normals);
		mesh3d.setAttribArray(VFMT::Tangent_GUID, m3d_md->tangents);
		mesh3d.setAttribArray(VFMT::Bitangent_GUID, m3d_md->bitangents);
		mesh3d.setIndexArray(m3d_md->indices.data(), m3d_md->indices.size() * sizeof(m3d_md->indices[0]));

		if (m3d_md->skin_data) {
			auto m3d_sd = m3d_md->skin_data.get();
			m3d_mesh->skin.reset(new m3dSkin);
			auto m3d_skin = m3d_mesh->skin.get();

			mesh3d.setAttribArray(VFMT::BoneIndex4_GUID, m3d_sd->bone_indices);
			mesh3d.setAttribArray(VFMT::BoneWeight4_GUID, m3d_sd->bone_weights);

			m3d_skin->inv_bind_transforms = std::move(m3d_sd->inverse_bind_transforms);

			assert(skeleton);
			m3d_skin->bone_list.resize(m3d_sd->bone_transform_source_indices.size());
			for (int j = 0; j < m3d_sd->bone_transform_source_indices.size(); ++j) {
				auto skl_bone = skeleton->getBone(m3d_sd->bone_transform_source_indices[j]);
				m3d_skin->bone_list[j] = skl_bone->getName();
			}
		}

		m3d_mesh->mesh.reset_acquire();
		m3d_mesh->mesh->setData(&mesh3d);
	}

	// mesh instances
	mesh_instances.resize(dat.mesh_instances.size());
	for (int i = 0; i < dat.mesh_instances.size(); ++i) {
		auto dat_inst = &dat.mesh_instances[i];
		auto m3d_inst = &mesh_instances[i];
		m3d_inst->bone_name = dat_inst->bone_name;
		m3d_inst->mesh_idx = dat_inst->mesh_idx;
	}
}

bool m3dModel::load(byte_reader& in) {
	m3dData m3d_data;
	if (!m3d_data.read(in)) {
		return false;
	}

	makeFromData(std::move(m3d_data));

	return true;
}

