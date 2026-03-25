#include "m3d_project.hpp"

#include "nlohmann/json.hpp"


bool m3dpProject::initFromSource(const std::string& filepath) {
    model_source.reset(new ModelImporter);
    if (!model_source->load(filepath)) {
        LOG_ERR("m3dpProject: failed to load model source " << filepath);
        return false;
    }

    {
        std::filesystem::path path = filepath;
        path = std::filesystem::relative(path, std::filesystem::current_path());
        source_path = path.string();
    }

    //
    {
        std::filesystem::path path = source_path;
        path = std::filesystem::relative(path, std::filesystem::current_path());
        path.replace_extension();

        std::string resource_id = path.string();
        out_model_resource_id = resource_id;
        out_skeleton_resource_id = resource_id;
        /*
        std::string resource_id = path.filename().string();
        out_model_resource_id = resource_id;
        out_skeleton_resource_id = resource_id;*/
    }
    //

    return true;
}

bool m3dpProject::load(const std::string& project_path) {
    nlohmann::json json;
    std::ifstream f(project_path);
    if (!f.is_open()) {
        LOG_ERR("m3dpProject: failed to open " << project_path);
        return false;
    }
    json << f;
    f.close();

    if (!json.is_object()) {
        LOG_ERR("m3dpProject: root json must be an object");
        return false;
    }

    source_path = json.value("source", "");
    scale_factor = json.value("scale_factor", 1.f);

    initFromSource(source_path);

    out_skeleton_resource_id = json.value("skl_res_id", "");
    out_model_resource_id = json.value("m3d_res_id", "");

    import_model = json.value("import_model", true);
    import_materials = json.value("import_materials", true);
    import_animations = json.value("import_animations", true);
    external_skeleton = json.value("external_skeleton", false);

    return true;
}

void m3dpProject::save(const std::string& project_path) {
    nlohmann::json json = nlohmann::json::object();

    json["source"] = source_path;
    json["scale_factor"] = scale_factor;

    json["skl_res_id"] = out_skeleton_resource_id;
    json["m3d_res_id"] = out_model_resource_id;

    json["import_model"] = import_model;
    json["import_materials"] = import_materials;
    json["import_animations"] = import_animations;
    json["external_skeleton"] = external_skeleton;

    std::ofstream f(project_path, std::ios::binary | std::ios::trunc);
    f << json.dump(2);
}

void m3dpProject::import(m3dModel& m3d) {
    m3dData m3d_data;
    import(m3d_data);
    m3d.makeFromData(std::move(m3d_data));
}

void m3dpProject::import(m3dData& m3d) {
    // TODO: Builds from ModelImporter directly for now,
    // but should build from internal augmentable representation

    // bounds
    m3d.aabb = model_source->getBoundingBox();
    m3d.bounding_sphere_origin = model_source->getBoundingSphereOrigin();
    m3d.bounding_radius = model_source->getBoundingRadius();
    
    // skeleton
    m3d.skeleton = ResourceManager::get()->create<Skeleton>("");
    {
        Skeleton* skeleton = m3d.skeleton.get();
        const mimpNode* src_node = model_source->getRoot();
        skeleton->setRootName(src_node->name.c_str());
        sklBone* skl_bone = skeleton->getRoot();
        std::queue<const mimpNode*> src_node_q;
        std::queue<sklBone*> skl_bone_q;
        while (src_node) {
            for (int i = 0; i < src_node->children.size(); ++i) {
                const mimpNode* src_child = src_node->children[i];
                src_node_q.push(src_child);
                sklBone* skl_child = skl_bone->createChild(src_child->name.c_str());
                skl_bone_q.push(skl_child);
            }

            skl_bone->setTranslation(src_node->translation);
            skl_bone->setRotation(src_node->rotation);
            skl_bone->setScale(src_node->scale);

            if (src_node_q.empty()) {
                src_node = 0;
            } else {
                src_node = src_node_q.front();
                src_node_q.pop();
                skl_bone = skl_bone_q.front();
                skl_bone_q.pop();
            }
        }
        sklBone* skl_root = skeleton->getRoot();
        skl_root->setTranslation(gfxm::vec3(0, 0, 0));
        skl_root->setRotation(gfxm::quat(0, 0, 0, 1));
        skl_root->setScale(gfxm::vec3(1, 1, 1));
    }

    std::set<std::string> missing_bones;

    // meshes
    LOG_DBG("m3dpProject: " << model_source->meshCount() << " meshes");
    for (int i = 0; i < model_source->meshCount(); ++i) {
        auto mimp_mesh = model_source->getMesh(i);
        auto& m3d_md = m3d.meshes.emplace_back();

        assert(mimp_mesh->material);
        m3d_md.material_idx = 0;
        if(mimp_mesh->material) {
            m3d_md.material_idx = mimp_mesh->material->index;
        }

        m3d_md.aabb = mimp_mesh->aabb;
        m3d_md.bounding_sphere_origin = mimp_mesh->bounding_sphere_pos;
        m3d_md.bounding_radius = mimp_mesh->bounding_radius;
        
        m3d_md.indices = mimp_mesh->indices;
        m3d_md.vertices = mimp_mesh->vertices;
        m3d_md.rgba_channels = mimp_mesh->rgba_channels;
        m3d_md.uv_channels = mimp_mesh->uv_channels;
        m3d_md.normals = mimp_mesh->normals;
        m3d_md.tangents = mimp_mesh->tangents;
        m3d_md.bitangents = mimp_mesh->bitangents;

        if (mimp_mesh->skin) {
            auto mimp_skin = mimp_mesh->skin.get();
            m3d_md.skin_data.reset(new m3dSkinData);
            auto m3d_sd = m3d_md.skin_data.get();
            m3d_sd->bone_indices = mimp_skin->bone_indices;
            m3d_sd->bone_weights = mimp_skin->bone_weights;
            m3d_sd->inverse_bind_transforms = mimp_skin->inverse_bind_transforms;

            // m3d_sd must reference the SKL bone indices, while mimp_skin references the source's node indices
            m3d_sd->bone_transform_source_indices.resize(mimp_skin->bone_transform_source_indices.size());
            for (int j = 0; j < mimp_skin->bone_transform_source_indices.size(); ++j) {
                auto src_idx = mimp_skin->bone_transform_source_indices[j];
                auto bone_name = model_source->getNode(src_idx)->name;
                auto skl_bone = m3d.skeleton->findBone(bone_name.c_str());
                int m3d_bone_idx = 0;
                if (skl_bone) {
                    m3d_bone_idx = skl_bone->getIndex();
                } else {
                    missing_bones.insert(bone_name);
                }
                m3d_sd->bone_transform_source_indices[j] = m3d_bone_idx;
            }
        }
    }

    // mesh instances
    LOG_DBG("m3dpProject: " << model_source->meshInstanceCount() << " mesh instances");
    m3d.mesh_instances.resize(model_source->meshInstanceCount());
    for (int i = 0; i < model_source->meshInstanceCount(); ++i) {
        auto src_inst = model_source->getMeshInstance(i);
        auto& m3d_mesh_inst = m3d.mesh_instances[i];

        m3d_mesh_inst.mesh_idx = src_inst->mesh->index;

        auto skl_bone = m3d.skeleton->findBone(src_inst->node->name.c_str());
        std::string bone_name = "";
        if (skl_bone) {
            bone_name = skl_bone->getName();
        } else {
            missing_bones.insert(src_inst->node->name);
        }
        m3d_mesh_inst.bone_name = bone_name;
    }

    if (!missing_bones.empty()) {
        LOG_WARN("m3dpProject::import: missing bones encountered: ");
        for (auto it : missing_bones) {
            LOG_WARN("\t" << it);
        }
    }
    
    // materials
    m3d.materials.clear();
    for (int i = 0; i < model_source->materialCount(); ++i) {
        auto mimp_mat = model_source->getMaterial(i);
        ResourceRef<gpuMaterial>& mat = m3d.materials.emplace_back();
        mat = ResourceManager::get()->create<gpuMaterial>("");
        if(mimp_mat->albedo) {
            mat->addSampler("texAlbedo", mimp_mat->albedo);
        }
        if(mimp_mat->normalmap) {
            mat->addSampler("texNormal", mimp_mat->normalmap);
        }
        if(mimp_mat->roughness) {
            mat->addSampler("texRoughness", mimp_mat->roughness);
        }
        if(mimp_mat->metallic) {
            mat->addSampler("texMetallic", mimp_mat->metallic);
        }
        if(mimp_mat->emission) {
            mat->addSampler("texEmission", mimp_mat->emission);
        }
        mat->setFragmentExtension(loadResource<gpuShaderSet>("core/shaders/modular/basic.frag"));
        mat->compile();
    }
    
    // animations
    for (int i = 0; i < model_source->animCount(); ++i) {
        auto anim = model_source->getAnimation(i);
        m3d.animations.push_back(anim->clip);
    }
}

void m3dpProject::save_m3d() {
    m3dData m3d_out;
    import(m3d_out);

    if(m3d_out.skeleton && !out_skeleton_resource_id.empty()) {
        m3d_out.skeleton._setResourceId(out_skeleton_resource_id);
        std::filesystem::path path = out_skeleton_resource_id;
        path.replace_extension(".skl");
        m3d_out.skeleton->write(path.string());
    }

    {
        std::filesystem::path path = out_model_resource_id;
        path.replace_extension(".m3d");
        m3d_out.write(path.string());
    }

    // TODO: separate materials, animations
}

