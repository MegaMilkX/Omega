#pragma once

#include <string>
#include <vector>
#include <memory>

#include "model_importer.hpp"
#include "m3d_model.hpp"
#include "m3d_data.hpp"

#include "reflection/prop_snapshot.hpp"

/*
class m3dpNode {
    std::string name;
    m3dpNode* parent = nullptr;
    std::vector<std::unique_ptr<m3dpNode>> children;
public:

};

class m3dpSkeleton {
    std::unique_ptr<m3dpNode> root;
public:
    void initFromSource(const std::string& source_path);
};

class m3dpComponent {
    m3dpNode* node = nullptr;
public:
    virtual ~m3dpComponent() {}
};

struct m3dpMesh : public m3dpComponent {

};
struct m3dpSkin : public m3dpComponent {

};*/

class m3dpProject {
    void applyMaterialDeltas();
public:
    std::string source_path;
    float scale_factor = .01f;

    std::string out_model_resource_id;
    std::string out_skeleton_resource_id;

    bool import_model = true;
    bool import_materials = true;
    bool import_animations = true;
    bool external_skeleton = false;

    std::unique_ptr<ModelImporter> model_source;

    std::vector<ResourceRef<gpuMaterial>> materials;
    std::vector<rtti::PropSnapshot> material_snaps;
    std::vector<rtti::PropSnapshot> material_deltas;
    std::unordered_map<int, std::string> material_names;

    //std::vector<std::unique_ptr<m3dpComponent>> components;

    // TODO: Start slow
    // Do not try to implement everything at once
    // Test it out without GUI first

    bool initFromSource(const std::string& path);
    bool load(const std::string& project_path);
    void save(const std::string& project_path);
    void import(m3dModel& m3d);
    void import(m3dData& m3d);
    void save_m3d();
};

