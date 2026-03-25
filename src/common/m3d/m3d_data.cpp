#include "m3d_data.hpp"

#include "m3d_file.hpp"
#include "resource_manager/byte_writer/vector_writer.hpp"
#include "resource_manager/byte_writer/file_writer.hpp"
#include "resource_manager/byte_reader/memory_reader.hpp"
#include "gpu/readwrite/rw_gpu_material.hpp"

constexpr uint32_t M3D_MESH_TAG = 'M' | ('3' << 8) | ('D' << 16) | ('V' << 24);
constexpr uint32_t M3D_MESH_VERSION = 1;

#pragma pack(push, 1)
struct M3D_MESH_HEAD {
	uint32_t tag = 0;
	uint32_t version = 0;

	uint32_t vertex_count = 0;
	uint32_t index_count = 0;

	uint16_t unused = 0;

	uint8_t	 n_rgba_channels = 0;
	uint8_t	 n_uv_channels = 0;

	uint32_t offs_bounds = 0;
	uint32_t offs_indices = 0;
	uint32_t offs_vertices = 0;
	uint32_t offs_normals = 0;
	uint32_t offs_tangents = 0;
	uint32_t offs_rgba = 0;
	uint32_t offs_uv = 0;
	uint32_t offs_skin = 0;
};
#pragma pack(pop)


bool m3dMeshData::read(byte_reader& in) {
	M3D_MESH_HEAD head = { 0 };
	in.read<M3D_MESH_HEAD>(&head);

	if (head.tag != M3D_MESH_TAG) {
		LOG_ERR("Not an M3D_MESH data stream");
		return false;
	}

	LOG_DBG("M3D_MESH version " << head.version);
	LOG_DBG("vertex count: " << head.vertex_count);
	LOG_DBG("index count: " << head.index_count);

	if (head.offs_bounds) {
		in.seek(head.offs_bounds, byte_reader::seek_set);

		in.read<gfxm::aabb>(&aabb);
		in.read<float>(&bounding_radius);
		in.read<gfxm::vec3>(&bounding_sphere_origin);
	}

	if (head.offs_indices) {
		in.seek(head.offs_indices, byte_reader::seek_set);

		indices.resize(head.index_count);
		in.read(indices.data(), indices.size() * sizeof(indices[0]));
	}

	in.seek(head.offs_vertices, byte_reader::seek_set);
	vertices.resize(head.vertex_count);
	in.read(vertices.data(), head.vertex_count * sizeof(vertices[0]));

	if(head.offs_normals) {
		in.seek(head.offs_normals, byte_reader::seek_set);

		normals.resize(head.vertex_count);
		in.read(normals.data(), head.vertex_count * sizeof(normals[0]));
	}

	if(head.offs_tangents) {
		in.seek(head.offs_tangents, byte_reader::seek_set);

		tangents.resize(head.vertex_count);
		in.read(tangents.data(), head.vertex_count * sizeof(tangents[0]));
	}

	if(head.offs_normals && head.offs_tangents) {
		bitangents.resize(head.vertex_count);
		for (int i = 0; i < head.vertex_count; ++i) {
			bitangents[i] = gfxm::normalize(gfxm::cross(tangents[i], normals[i]));
		}
	}

	if(head.offs_rgba) {
		in.seek(head.offs_rgba, byte_reader::seek_set);

		rgba_channels.resize(head.n_rgba_channels);
		for (int i = 0; i < head.n_rgba_channels; ++i) {
			auto& chan = rgba_channels[i];
			chan.resize(head.vertex_count);
			in.read(chan.data(), head.vertex_count * sizeof(chan[0]));
		}
	}
	if(head.offs_uv) {
		in.seek(head.offs_uv, byte_reader::seek_set);

		uv_channels.resize(head.n_uv_channels);
		for (int i = 0; i < head.n_uv_channels; ++i) {
			auto& chan = uv_channels[i];
			chan.resize(head.vertex_count);
			in.read(chan.data(), head.vertex_count * sizeof(chan[0]));
		}
	}

	if (head.offs_skin) {
		in.seek(head.offs_skin, byte_reader::seek_set);

		skin_data.reset(new m3dSkinData);
		auto& indices = skin_data->bone_indices;
		auto& weights = skin_data->bone_weights;
		auto& inv_bind = skin_data->inverse_bind_transforms;
		auto& src_indices = skin_data->bone_transform_source_indices;

		indices.resize(head.vertex_count);
		weights.resize(head.vertex_count);
		in.read(indices.data(), head.vertex_count * sizeof(indices[0]));
		in.read(weights.data(), head.vertex_count * sizeof(weights[0]));

		uint32_t local_bone_count = 0;
		in.read<uint32_t>(&local_bone_count);

		inv_bind.resize(local_bone_count);
		src_indices.resize(local_bone_count);
		in.read(inv_bind.data(), local_bone_count * sizeof(inv_bind[0]));
		in.read(src_indices.data(), local_bone_count * sizeof(src_indices[0]));
	}

	return true;
}

void m3dMeshData::write(byte_writer& out) {
	M3D_MESH_HEAD head = { 0 };
	head.tag = M3D_MESH_TAG;
	head.version = M3D_MESH_VERSION;

	head.vertex_count = vertices.size();
	head.index_count = indices.size();

	head.n_rgba_channels = rgba_channels.size();
	head.n_uv_channels = rgba_channels.size();

	out.write<M3D_MESH_HEAD>(head);

	head.offs_bounds = out.tell();
	out.write<gfxm::aabb>(aabb);
	out.write<float>(bounding_radius);
	out.write<gfxm::vec3>(bounding_sphere_origin);	

	head.offs_indices = out.tell();
	out.write(indices.data(), indices.size() * sizeof(indices[0]));

	head.offs_vertices = out.tell();
	out.write(vertices.data(), vertices.size() * sizeof(vertices[0]));

	head.offs_normals = out.tell();
	out.write(normals.data(), normals.size() * sizeof(normals[0]));

	head.offs_tangents = out.tell();
	out.write(tangents.data(), tangents.size() * sizeof(tangents[0]));

	head.offs_rgba = out.tell();
	for (int i = 0; i < rgba_channels.size(); ++i) {
		const auto& chan = rgba_channels[i];
		out.write(chan.data(), chan.size() * sizeof(chan[0]));
	}

	head.offs_uv = out.tell();
	for (int i = 0; i < uv_channels.size(); ++i) {
		const auto& chan = uv_channels[i];
		out.write(chan.data(), chan.size() * sizeof(chan[0]));
	}

	if(skin_data) {
		head.offs_skin = out.tell();
		
		out.write(
			skin_data->bone_indices.data(),
			skin_data->bone_indices.size() * sizeof(skin_data->bone_indices[0])
		);
		out.write(
			skin_data->bone_weights.data(),
			skin_data->bone_weights.size() * sizeof(skin_data->bone_weights[0])
		);

		uint32_t local_bone_count = skin_data->inverse_bind_transforms.size();
		out.write<uint32_t>(local_bone_count);
		
		out.write(
			skin_data->inverse_bind_transforms.data(),
			skin_data->inverse_bind_transforms.size() * sizeof(skin_data->inverse_bind_transforms[0])
		);
		out.write(
			skin_data->bone_transform_source_indices.data(),
			skin_data->bone_transform_source_indices.size() * sizeof(skin_data->bone_transform_source_indices[0])
		);
	}

	size_t rewind = out.tell();
	out.seek(0, byte_writer::seek_set);
	out.write<M3D_MESH_HEAD>(head);
	out.seek(rewind, byte_writer::seek_set); // seek back to the "end" in case we're being directly embedded
}

bool m3dMeshData::write(const std::string& path) {
	FILE* f = fopen(path.c_str(), "wb");
	if (!f) {
		LOG_ERR("m3dMeshData::write: failed to open file for writing '" << path << "'");
		return false;
	}
	file_writer fout(f);
	write(fout);

	fclose(f);
	return true;
}


bool m3dData::read(byte_reader& in) {
	M3D_HEAD head;
	in.read<M3D_HEAD>(&head);

	if (head.tag != M3D_TAG) {
		LOG_ERR("Not an m3d file");
		return false;
	}

	LOG_DBG("M3D version " << head.version);

	if (head.offs_bounds) {
		in.seek(head.offs_bounds, byte_reader::seek_set);

		in.read<gfxm::aabb>(&aabb);
		in.read<float>(&bounding_radius);
		in.read<gfxm::vec3>(&bounding_sphere_origin);
	}

	if(head.offs_skeleton) {
		in.seek(head.offs_skeleton, byte_reader::seek_set);

		uint64_t skl_fingerprint = 0;
		in.read<uint64_t>(&skl_fingerprint);
		uint8_t embedded = 0;
		in.read<uint8_t>(&embedded);
		if (embedded) {
			uint32_t skl_size = 0;
			in.read<uint32_t>(&skl_size);
			std::vector<unsigned char> buf(skl_size);
			in.read(buf.data(), skl_size);
			memory_reader mem_reader(buf.data(), buf.size());
			skeleton = ResourceManager::get()->create<Skeleton>("");
			skeleton->load(mem_reader);
		} else {
			std::string res_id;
			in.read_string(&res_id);
			skeleton = loadResource<Skeleton>(res_id);
		}

		if (skl_fingerprint != skeleton->getFingerprint()) {
			LOG_WARN("M3D: Skeleton fingerprint mismatch");
			assert(false);
		}
	}

	if (head.offs_materials) {
		in.seek(head.offs_materials, byte_reader::seek_set);
		uint32_t mat_count = 0;
		in.read<uint32_t>(&mat_count);
		materials.resize(mat_count);
		for (int i = 0; i < mat_count; ++i) {
			uint8_t embedded = 0;
			in.read<uint8_t>(&embedded);
			if (embedded) {
				uint32_t nbytes = 0;
				in.read<uint32_t>(&nbytes);
				std::vector<unsigned char> bytes(nbytes);
				in.read(bytes.data(), bytes.size());

				std::string jstr(bytes.data(), bytes.data() + bytes.size());
				nlohmann::json json = nlohmann::json::parse(jstr);

				auto& mat = materials[i];
				mat = ResourceManager::get()->create<gpuMaterial>("");
				if (!readGpuMaterialJson(json, mat.get())) {
					assert(false);
					LOG_ERR("M3D: Failed to read embedded material");
				}
			} else {
				assert(false);
				LOG_ERR("M3D: External materials not implemented yet");
				return false;
			}
		}
	}

	if (head.offs_meshes) {
		in.seek(head.offs_meshes, byte_reader::seek_set);
		uint32_t mesh_count = 0;
		in.read<uint32_t>(&mesh_count);
		meshes.resize(mesh_count);
		for (int i = 0; i < mesh_count; ++i) {
			uint32_t material_idx = 0;
			in.read<uint32_t>(&material_idx);

			uint8_t embedded = 0;
			in.read<uint8_t>(&embedded);
			if (embedded) {
				uint32_t nbytes = 0;
				in.read<uint32_t>(&nbytes);
				std::vector<unsigned char> bytes(nbytes);
				in.read(bytes.data(), bytes.size());

				memory_reader reader(bytes.data(), bytes.size());
				if (!meshes[i].read(reader)) {
					assert(false);
					LOG_ERR("M3D: Failed to read embedded mesh " << i);
					return false;
				}

				meshes[i].material_idx = material_idx;
			} else {
				assert(false);
				LOG_ERR("M3D: External meshes not implemented");
				return false;
			}
		}
	}

	if (head.offs_mesh_instances) {
		in.seek(head.offs_mesh_instances, byte_reader::seek_set);
		
		uint32_t inst_count = 0;
		in.read<uint32_t>(&inst_count);
		mesh_instances.resize(inst_count);
		for (int i = 0; i < inst_count; ++i) {
			uint32_t mesh_idx = 0;
			std::string bone_name;
			in.read<uint32_t>(&mesh_idx);
			in.read_string(&bone_name);

			auto& inst = mesh_instances[i];
			inst.mesh_idx = mesh_idx;
			inst.bone_name = bone_name;
		}
	}

	if (head.offs_animations) {
		in.seek(head.offs_animations, byte_reader::seek_set);
		
		uint32_t anim_count = 0;
		in.read<uint32_t>(&anim_count);
		animations.resize(anim_count);
		for (int i = 0; i < anim_count; ++i) {
			uint8_t embedded = 0;
			in.read<uint8_t>(&embedded);
			if (embedded) {
				uint32_t nbytes = 0;
				in.read<uint32_t>(&nbytes);
				std::vector<unsigned char> bytes(nbytes);
				in.read(bytes.data(), bytes.size());

				memory_reader reader(bytes.data(), bytes.size());
				animations[i] = ResourceManager::get()->create<Animation>("");
				if (!animations[i]->load(reader)) {
					assert(false);
					LOG_ERR("M3D: Failed to read embedded animation " << i);
					return false;
				}
			} else {
				assert(false);
				LOG_ERR("M3D: External animations not implemented yet");
				return false;
			}
		}
	}

	return true;
}

void m3dData::write(byte_writer& out) {    
	M3D_HEAD head;
	head.tag = M3D_TAG;
	head.version = M3D_VERSION;

	out.write<M3D_HEAD>(head);

	// bounds
	{
		head.offs_bounds = out.tell();
		out.write<gfxm::aabb>(aabb);
		out.write<float>(bounding_radius);
		out.write<gfxm::vec3>(bounding_sphere_origin);
	}

	// skeleton
	{
		head.offs_skeleton = out.tell();
		// TODO: HANDLE THE NULL SKELETON CASE

		uint64_t skl_fingerprint = skeleton->getFingerprint();
		out.write<uint64_t>(skl_fingerprint);
		uint8_t embedded = skeleton.getResourceId().empty() ? 1 : 0;
		out.write<uint8_t>(embedded);
		if (embedded) {
			static_assert(std::is_base_of_v<IWritable, std::remove_cvref<decltype(skeleton)>::type::resource_type>);

			std::vector<unsigned char> bytes;
			vector_writer vout(bytes);
			skeleton->write(vout);

			out.write<uint32_t>(bytes.size());
			out.write(bytes.data(), bytes.size());
		} else {
			std::string res_id = skeleton.getResourceId();
			out.write_string(res_id);
		}
	}

	// materials
	{
		head.offs_materials = out.tell();
		out.write<uint32_t>(materials.size());
		for (int i = 0; i < materials.size(); ++i) {
			ResourceRef<gpuMaterial>& mat = materials[i];
			const std::string& res_id = mat.getResourceId();

			uint8_t embedded = res_id.empty() ? 1 : 0;
			out.write<uint8_t>(embedded);
			if (embedded) {
				static_assert(std::is_base_of_v<IWritable, std::remove_cvref<decltype(mat)>::type::resource_type>);

				std::vector<unsigned char> bytes;
				vector_writer vout(bytes);
				mat->write(vout);

				out.write<uint32_t>(bytes.size());
				out.write(bytes.data(), bytes.size());
			} else {
				out.write_string(res_id);
			}
		}
	}

	// meshes
	{
		head.offs_meshes = out.tell();
		out.write<uint32_t>(meshes.size());
		for (int i = 0; i < meshes.size(); ++i) {
			auto mesh = &meshes[i];

			out.write<uint32_t>(mesh->material_idx);

			uint8_t embedded = 1; // always embed for now
			out.write<uint8_t>(embedded);

			std::vector<unsigned char> bytes;
			vector_writer vout(bytes);
			mesh->write(vout);
			out.write<uint32_t>(bytes.size());
			out.write(bytes.data(), bytes.size());
		}
	}

	// mesh instances
	{
		head.offs_mesh_instances = out.tell();
		out.write<uint32_t>(mesh_instances.size());
		for (int i = 0; i < mesh_instances.size(); ++i) {
			auto mesh_inst = &mesh_instances[i];
			out.write<uint32_t>(mesh_inst->mesh_idx);
			out.write_string(mesh_inst->bone_name);
		}
	}

	// effects
	{
		//
	}

	// animations
	{
		head.offs_animations = out.tell();
		out.write<uint32_t>(animations.size());
		for (int i = 0; i < animations.size(); ++i) {
			auto& anim = animations[i];
			const std::string& res_id = anim.getResourceId();

			uint8_t embedded = res_id.empty() ? 1 : 0;
			out.write<uint8_t>(embedded);
			if (embedded) {
				static_assert(std::is_base_of_v<IWritable, std::remove_cvref<decltype(anim)>::type::resource_type>);

				std::vector<unsigned char> bytes;
				vector_writer vout(bytes);
				anim->write(vout);

				out.write<uint32_t>(bytes.size());
				out.write(bytes.data(), bytes.size());
			} else {
				out.write_string(res_id);
			}
		}
	}

	size_t rewind = out.tell();
	out.seek(0, byte_writer::seek_set);
	out.write<M3D_HEAD>(head);
	out.seek(rewind, byte_writer::seek_set); // seek back to the "end" in case we're being directly embedded
}

bool m3dData::write(const std::string& path) {
	FILE* f = fopen(path.c_str(), "wb");
	if (!f) {
		LOG_ERR("m3dData::write: failed to open file for writing '" << path << "'");
		return false;
	}
	file_writer fout(f);
	write(fout);

	fclose(f);
	return true;
}

