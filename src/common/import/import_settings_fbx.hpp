#pragma once

#include "import_settings.hpp"
#include <string>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include "assimp_load_skeletal_model.hpp"
#include "animation/readwrite/rw_animation.hpp"
#include "filesystem/filesystem.hpp"

struct ImportSettingsFbx : public ImportSettings {
    struct AnimationTrack {
        std::string output_path;
        std::string name;
        std::string source_track_name;
        gfxm::ivec2 range;

        struct {
            bool enabled;
            std::string bone_name;
        } root_motion;
    };
    struct Material {
        std::string name;
        std::string output_path;
        bool overwrite;
    };

    float scale_factor = .01f;
    std::string source_path;

    bool import_skeletal_model = false;
    bool import_static_model = false;

    std::string model_path;
    std::string skeleton_path;
    bool overwrite_skeleton;
    bool import_model;
    bool import_materials;
    bool import_animations;
    std::vector<AnimationTrack> tracks;
    std::vector<Material> materials;
    //
    std::string static_model_path;

    bool from_source(const std::string& spath) override {
        LOG("ImportSettingsFbx: generating from file...");
        std::vector<char> bytes;
        {
            std::ifstream file(spath, std::ios::binary | std::ios::ate);
            if (!file.is_open()) {
                LOG_ERR("Failed to open file: " << spath);
                return false;
            }
            std::streamsize file_size = file.tellg();

            file.seekg(0, std::ios::beg);
            bytes.resize(file_size);
            file.read(&bytes[0], file_size);
        }
        
        fs_path current_dir = fsGetCurrentDirectory();

        source_path = spath;
        std::filesystem::path path = spath;
        std::filesystem::path target_path = path;
        target_path.remove_filename();
        target_path /= path.stem();
        std::string target_directory = target_path.string();
        model_path = target_directory + "\\" + path.stem().string() + ".skeletal_model";
        skeleton_path = target_directory + "\\" + path.stem().string() + ".skeleton";
        import_file_path = path.string() + ".import";

        static_model_path = target_directory + "\\" + path.stem().string() + ".static_model";

        source_path = fsMakeRelativePath(current_dir, source_path).string();
        import_file_path = fsMakeRelativePath(current_dir, import_file_path).string();
        model_path = fsMakeRelativePath(current_dir, model_path).string();
        skeleton_path = fsMakeRelativePath(current_dir, skeleton_path).string();

        static_model_path = fsMakeRelativePath(current_dir, static_model_path).string();

        overwrite_skeleton = true;
        import_model = true;
        import_materials = true;
        import_animations = true;

        {
            LOG("aiImportFileFromMemory()...");
            const aiScene* ai_scene = aiImportFileFromMemory(
                &bytes[0], bytes.size(),
                aiProcess_GenSmoothNormals |
                aiProcess_GenUVCoords |
                aiProcess_Triangulate |
                aiProcess_JoinIdenticalVertices |
                aiProcess_LimitBoneWeights |
                aiProcess_GlobalScale,
                source_path.c_str()
            );
            if (!ai_scene) {
                LOG_ERR("aiImportFileFromMemory() failed");
                return false;
            }

            LOG("aiApplyPostProcessing()...");
            ai_scene = aiApplyPostProcessing(ai_scene, aiProcess_CalcTangentSpace);
            if (!ai_scene) {
                LOG_ERR("aiApplyPostProcessing() failed");
                return false;
            }

            double fbxScaleFactor = 1.0;
            if (ai_scene->mMetaData && ai_scene->mMetaData->Get("UnitScaleFactor", fbxScaleFactor)) {
                if (fbxScaleFactor == .0) fbxScaleFactor = 1.0;
                fbxScaleFactor *= .01;
            }
            LOG("fbx scale factor: " << fbxScaleFactor);
            scale_factor = float(fbxScaleFactor);

            LOG("Animations");
            for (int i = 0; i < ai_scene->mNumAnimations; ++i) {
                aiAnimation* ai_anim = ai_scene->mAnimations[i];
                LOG("\tAnim " << i << ": " << ai_anim->mName.C_Str());

                ImportSettingsFbx::AnimationTrack track;        

                track.output_path = target_directory + "\\" + ai_anim->mName.C_Str() + ".anim";
                track.output_path = fsMakeRelativePath(current_dir, track.output_path).string();
                track.name = ai_anim->mName.C_Str();
                track.source_track_name = ai_anim->mName.C_Str();
                track.range.x = 0;
                track.range.y = ai_anim->mDuration - 1; // TODO: Might fuck up the last frame due to 999.999999999999998
                track.root_motion.enabled = false;
                track.root_motion.bone_name = "";

                tracks.push_back(track);
            }
            LOG("Materials");
            for (int i = 0; i < ai_scene->mNumMaterials; ++i) {
                aiMaterial* ai_mat = ai_scene->mMaterials[i];
                LOG("\tMaterial " << i << ": " << ai_mat->GetName().C_Str());

                std::string name = ai_mat->GetName().C_Str();
                if (name.empty()) {
                    name = std::string("Unnamed") + std::to_string(i);
                }

                ImportSettingsFbx::Material mat;
                mat.name = name;
                mat.output_path = target_directory + "\\" + ai_mat->GetName().C_Str() + ".material";
                mat.output_path = fsMakeRelativePath(current_dir, mat.output_path).string();
                mat.overwrite = false;
                materials.push_back(mat);
            }
        }
        return true;
    }
    void to_json(nlohmann::json& j_) override {
        j_["type"] = "3DModel";

        if (import_skeletal_model) {
            auto& j = j_["SkeletalModel"];

            j["type"] = "SkeletalModel";
            j["source_path"] = source_path;
            j["model_path"] = model_path;
            j["scale_factor"] = scale_factor;
            if (skeleton_path.empty()) {
                j["skeleton_path"] = nullptr;
            }
            else {
                j["skeleton_path"] = skeleton_path;
            }

            j["overwrite_skeleton"] = overwrite_skeleton;
            j["import_model"] = import_model;
            j["import_materials"] = import_materials;
            j["import_animations"] = import_animations;

            nlohmann::json janim;
            for (int i = 0; i < tracks.size(); ++i) {
                auto& track = tracks[i];
                nlohmann::json jtrack;
                jtrack["output_path"] = track.output_path;
                jtrack["source_track_name"] = track.source_track_name;
                jtrack["from"] = track.range.x;
                jtrack["to"] = track.range.y;
                nlohmann::json& jroot_motion = jtrack["root_motion"];
                jroot_motion["enabled"] = track.root_motion.enabled;
                jroot_motion["bone_name"] = track.root_motion.bone_name;
                janim[track.name] = jtrack;
            }
            j["animation_tracks"] = janim;
        }

        if (import_static_model) {
            auto& j = j_["StaticModel"];

            j["model_path"] = static_model_path;
        }

        nlohmann::json jmaterials;
        for (int i = 0; i < materials.size(); ++i) {
            auto& mat = materials[i];
            nlohmann::json jmat;
            jmat["output_path"] = mat.output_path;
            jmat["overwrite"] = mat.overwrite;
            jmaterials[mat.name] = jmat;
        }
        j_["materials"] = jmaterials;
    }
    void from_json(const nlohmann::json& j_) override {
        auto it_skeletal = j_.find("SkeletalModel");
        if(it_skeletal != j_.end()) {
            auto& j = it_skeletal.value();
            if (!j.is_object()) {
                assert(false);
                return;
            }
            import_skeletal_model = true;

            source_path = j["source_path"];
            {
                auto it = j.find("scale_factor");
                if (it != j.end() && it.value().is_number()) {
                    scale_factor = it.value().get<float>();
                }
            }
            if (!j["model_path"].is_null()) {
                model_path = j["model_path"].get<std::string>();
            }
            if (!j["skeleton_path"].is_null()) {
                skeleton_path = j["skeleton_path"].get<std::string>();
            }

            overwrite_skeleton = j["overwrite_skeleton"];
            import_model = j["import_model"];
            import_materials = j["import_materials"];
            import_animations = j["import_animations"];

            const nlohmann::json& janim = j["animation_tracks"];
            if (janim.is_object()) {
                for (auto it = janim.begin(); it != janim.end(); ++it) {
                    ImportSettingsFbx::AnimationTrack track;
                    track.name = it.key();
                
                    const nlohmann::json& jtrack = it.value();
                    track.output_path = jtrack["output_path"].get<std::string>();
                    track.source_track_name = jtrack["source_track_name"].get<std::string>();
                    track.range.x = jtrack["from"].get<int>();
                    track.range.y = jtrack["to"].get<int>();
                    track.root_motion.enabled = jtrack["root_motion"]["enabled"].get<bool>();
                    track.root_motion.bone_name = jtrack["root_motion"]["bone_name"].get<std::string>();

                    tracks.push_back(track);
                }
            }
        }
        
        auto it_static = j_.find("StaticModel");
        if (it_static != j_.end()) {
            auto& j = it_skeletal.value();
            if (!j.is_object()) {
                assert(false);
                return;
            }
            import_static_model = true;

            if (!j["model_path"].is_null()) {
                static_model_path = j["model_path"].get<std::string>();
            }
        }

        const nlohmann::json& jmaterials = j_["materials"];
        if (jmaterials.is_object()) {
            for (auto it = jmaterials.begin(); it != jmaterials.end(); ++it) {
                ImportSettingsFbx::Material mat;
                mat.name = it.key();

                const nlohmann::json& jmat = it.value();
                mat.output_path = jmat["output_path"].get<std::string>();
                mat.overwrite = jmat["overwrite"].get<bool>();

                materials.push_back(mat);
            }
        }
    }
    bool do_import() override {
        assimpLoadedResources resources;
        RHSHARED<mdlSkeletalModelMaster> model(HANDLE_MGR<mdlSkeletalModelMaster>::acquire());
        assimpImporter importer;
        importer.loadFile(source_path.c_str(), scale_factor);
        importer.loadSkeletalModel(model.get(), &resources);
        
        if (import_skeletal_model) {
            if (import_animations) {
                for (int i = 0; i < tracks.size(); ++i) {
                    auto& track = tracks[i];
                    std::filesystem::path anim_path = track.output_path;
                    fsCreateDirRecursive(anim_path.parent_path().string());

                    RHSHARED<Animation> anim(HANDLE_MGR<Animation>::acquire());
                    importer.loadAnimation(
                        anim.get(), track.source_track_name.c_str(), track.range.x, track.range.y,
                        track.root_motion.enabled ? track.root_motion.bone_name.c_str() : 0
                    );

                    AnimationUAF::saveFile(anim_path.string(), anim.get(), 0, 0);

                    anim.setReferenceName(anim_path.string().c_str());
                }
            }

            for (int i = 0; i < materials.size(); ++i) {
                auto& mat = materials[i];
                std::filesystem::path path = mat.output_path;

                bool exists = file_exists(mat.output_path);
                if (import_materials && (!exists || mat.overwrite)) {
                    fsCreateDirRecursive(path.parent_path().string());
                    resources.materials[i].serializeJson(mat.output_path.c_str(), true);
                    exists = true;
                }

                if (exists) {
                    resources.materials[i].setReferenceName(mat.output_path.c_str());
                }
            }

            model->getSkeleton().setReferenceName(skeleton_path.c_str());

            bool skeleton_exists = file_exists(skeleton_path);
            if (!skeleton_exists || overwrite_skeleton) {
                std::filesystem::path path = skeleton_path;
                fsCreateDirRecursive(path.parent_path().string());
                model->getSkeleton().serializeJson(skeleton_path.c_str(), true);
            }
            if (import_model) {
                std::filesystem::path path = model_path;
                fsCreateDirRecursive(path.parent_path().string());
                model.serializeJson(model_path.c_str(), true);
            }
        }

        if (import_static_model) {
            RHSHARED<StaticModel> model;
            model.reset_acquire();
            importer.loadStaticModel(model.get());

            for (int i = 0; i < materials.size(); ++i) {
                auto& mat = materials[i];
                std::filesystem::path path = mat.output_path;

                bool exists = file_exists(mat.output_path);
                if (import_materials && (!exists || mat.overwrite)) {
                    fsCreateDirRecursive(path.parent_path().string());
                    resources.materials[i].serializeJson(mat.output_path.c_str(), true);
                    exists = true;
                }

                if (exists) {
                    resources.materials[i].setReferenceName(mat.output_path.c_str());
                }
            }

            std::filesystem::path path = static_model_path;
            fsCreateDirRecursive(path.parent_path().string());
            model.serializeJson(static_model_path.c_str(), true);
        }

        LOG(skeleton_path);
        LOG(model_path);
        return true;
    }
};
