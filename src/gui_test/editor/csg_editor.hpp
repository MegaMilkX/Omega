#pragma once

#include "editor_window.hpp"

#include "gui/elements/viewport/gui_viewport.hpp"

#include "gui/elements/viewport/tools/gui_viewport_tool_csg_object_mode.hpp"
#include "gui/elements/viewport/tools/gui_viewport_tool_csg_face_mode.hpp"
#include "gui/elements/viewport/tools/gui_viewport_tool_transform.hpp"
#include "gui/elements/viewport/tools/gui_viewport_tool_csg_create_box.hpp"
#include "gui/elements/viewport/tools/gui_viewport_tool_csg_cut.hpp"

#include "xatlas.h"
#include "lightmapper/lightmapper.h"

// TODO: REMOVE THIS
extern std::set<GameRenderInstance*> game_render_instances;

class GuiCsgWindow : public GuiWindow {
    gpuRenderBucket render_bucket;
    gpuRenderTarget render_target;
    GameRenderInstance render_instance;
    GuiViewport viewport;
    GuiViewportToolCsgObjectMode tool_object_mode;
    GuiViewportToolCsgFaceMode tool_face_mode;
    GuiViewportToolCsgCreateBox tool_create_box;
    GuiViewportToolCsgCut tool_cut;

    csgScene csg_scene;
    csgMaterial* mat_floor = 0;
    csgMaterial* mat_floor2 = 0;
    csgMaterial* mat_wall = 0;
    csgMaterial* mat_wall2 = 0;
    csgMaterial* mat_ceiling = 0;
    csgMaterial* mat_planet = 0;
    csgMaterial* mat_floor_def = 0;
    csgMaterial* mat_wall_def = 0;

    //std::vector<std::unique_ptr<csgBrushShape>> shapes;
    //csgBrushShape* selected_shape = 0;
    std::vector<csgBrushShape*> selected_shapes;

    //std::map<uint64_t, csgMaterial*> materials;

    struct Mesh {
        csgMaterial* material = 0;
        gpuBuffer vertex_buffer;
        gpuBuffer normal_buffer;
        gpuBuffer tangent_buffer;
        gpuBuffer bitangent_buffer;
        gpuBuffer color_buffer;
        gpuBuffer uv_buffer;
        gpuBuffer uv_lightmap_buffer;
        gpuBuffer index_buffer;

        std::vector<gfxm::vec3> lm_vertices;
        std::vector<gfxm::vec3> lm_normals;
        std::vector<gfxm::vec2> lm_uv;
        std::vector<uint32_t> lm_indices;
        std::vector<float> lightmap_data;
        RHSHARED<gpuTexture2d> lightmap;

        std::unique_ptr<gpuMeshDesc> mesh_desc;
        gpuUniformBuffer* renderable_ubuf = 0;
        gpuRenderable renderable;
        gpuMeshShaderBinding* mesh_shader_binding;

        ~Mesh() {
            if (renderable_ubuf) {
                gpuGetPipeline()->destroyUniformBuffer(renderable_ubuf);
            }
        }
    };
    std::vector<std::unique_ptr<Mesh>> meshes;


public:
    RHSHARED<mdlSkeletalModelMaster> model;
    RHSHARED<Skeleton> skeleton;
    CollisionTriangleMesh collision_mesh;

    GuiCsgWindow()
        : GuiWindow("CSG"),
        render_bucket(gpuGetPipeline(), 1000),
        render_target(800, 600),
        tool_object_mode(&csg_scene),
        tool_create_box(&csg_scene),
        tool_cut(&csg_scene)
    {
        model.reset_acquire();
        skeleton.reset_acquire();
        model->setSkeleton(skeleton);

        tool_object_mode.setOwner(this);
        tool_face_mode.setOwner(this);
        tool_create_box.setOwner(this);
        tool_cut.setOwner(this);

        addChild(&viewport);
        viewport.setOwner(this);

        gpuGetPipeline()->initRenderTarget(&render_target);
        
        render_instance.render_bucket = &render_bucket;
        render_instance.render_target = &render_target;
        render_instance.view_transform = gfxm::mat4(1.0f);
        game_render_instances.insert(&render_instance);

        viewport.render_instance = &render_instance;
        guiDragSubscribe(&viewport);

        viewport.addTool(&tool_object_mode);

        return;
        mat_floor = csg_scene.createMaterial("materials/csg/floor.mat");
        mat_floor2 = csg_scene.createMaterial("materials/csg/floor2.mat");
        mat_wall = csg_scene.createMaterial("materials/csg/wall.mat");
        mat_wall2 = csg_scene.createMaterial("materials/csg/wall2.mat");
        mat_ceiling = csg_scene.createMaterial("materials/csg/ceiling.mat");
        mat_planet = csg_scene.createMaterial("materials/csg/planet.mat");
        mat_floor_def = csg_scene.createMaterial("materials/csg/default_floor.mat");
        mat_wall_def = csg_scene.createMaterial("materials/csg/default_wall.mat");
        mat_floor->gpu_material = resGet<gpuMaterial>("materials/csg/floor.mat");
        mat_floor2->gpu_material = resGet<gpuMaterial>("materials/csg/floor2.mat");
        mat_wall->gpu_material = resGet<gpuMaterial>("materials/csg/wall.mat");
        mat_wall2->gpu_material = resGet<gpuMaterial>("materials/csg/wall2.mat");
        mat_ceiling->gpu_material = resGet<gpuMaterial>("materials/csg/ceiling.mat");
        mat_planet->gpu_material = resGet<gpuMaterial>("materials/csg/planet.mat");
        mat_floor_def->gpu_material = resGet<gpuMaterial>("materials/csg/default_floor.mat");
        mat_wall_def->gpu_material = resGet<gpuMaterial>("materials/csg/default_wall.mat");

        csgBrushShape* shape_room = new csgBrushShape;
        csgMakeCube(shape_room, 14.f, 4.f, 14.f, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(0, 2, -2)));
        shape_room->volume_type = CSG_VOLUME_EMPTY;
        //shape_room->rgba = gfxm::make_rgba32(0.7, .4f, .6f, 1.f);
        shape_room->faces[0]->uv_scale = gfxm::vec2(2.f, 2.f);
        shape_room->faces[0]->material = mat_wall;
        shape_room->faces[1]->uv_scale = gfxm::vec2(2.f, 2.f);
        shape_room->faces[1]->material = mat_wall;
        shape_room->faces[4]->uv_scale = gfxm::vec2(2.f, 2.f);
        shape_room->faces[4]->material = mat_wall;
        shape_room->faces[5]->uv_scale = gfxm::vec2(2.f, 2.f);
        shape_room->faces[5]->material = mat_wall;
        shape_room->faces[2]->uv_scale = gfxm::vec2(5.f, 5.f);
        shape_room->faces[2]->uv_offset = gfxm::vec2(2.5f, 2.5f);
        shape_room->faces[2]->material = mat_floor;
        shape_room->faces[3]->uv_scale = gfxm::vec2(3.f, 3.f);
        shape_room->faces[3]->uv_offset = gfxm::vec2(.0f, .0f);
        shape_room->faces[3]->material = mat_ceiling;
        //shapes.push_back(std::unique_ptr<csgBrushShape>(shape_room));
        csg_scene.addShape(shape_room);

        // Ceiling arch X axis, A
        {
            csgBrushShape* shape = new csgBrushShape;
            csgMakeCylinder(shape, 14, 2.f, 16,
                gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(7, 4, 2.9))
                * gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(90.f), gfxm::vec3(0, 0, 1)))
            );
            shape->material = mat_wall;
            shape->volume_type = CSG_VOLUME_EMPTY;
            //shapes.push_back(std::unique_ptr<csgBrushShape>(shape));
            csg_scene.addShape(shape);
        }
        // Ceiling arch X axis, B
        {
            csgBrushShape* shape = new csgBrushShape;
            csgMakeCylinder(shape, 14, 2.f, 16,
                gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(7, 4, -2))
                * gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(90.f), gfxm::vec3(0, 0, 1)))
            );
            shape->material = mat_wall;
            shape->volume_type = CSG_VOLUME_EMPTY;
            //shapes.push_back(std::unique_ptr<csgBrushShape>(shape));
            csg_scene.addShape(shape);
        }
        // Ceiling arch X axis, C
        {
            csgBrushShape* shape = new csgBrushShape;
            csgMakeCylinder(shape, 14, 2.f, 16,
                gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(7, 4, -6.9))
                * gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(90.f), gfxm::vec3(0, 0, 1)))
            );
            shape->material = mat_wall;
            shape->volume_type = CSG_VOLUME_EMPTY;
            //shapes.push_back(std::unique_ptr<csgBrushShape>(shape));
            csg_scene.addShape(shape);
        }
        // Ceiling arch Z axis, A
        {
            csgBrushShape* shape = new csgBrushShape;
            csgMakeCylinder(shape, 14, 2.f, 16,
                gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(0, 4, -9))
                * gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(90.f), gfxm::vec3(1, 0, 0)))
            );
            shape->material = mat_wall;
            shape->volume_type = CSG_VOLUME_EMPTY;
            //shapes.push_back(std::unique_ptr<csgBrushShape>(shape));
            csg_scene.addShape(shape);
        }
        // Ceiling arch Z axis, B
        {
            csgBrushShape* shape = new csgBrushShape;
            csgMakeCylinder(shape, 14, 2.f, 16,
                gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(4.9, 4, -9))
                * gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(90.f), gfxm::vec3(1, 0, 0)))
            );
            shape->material = mat_wall;
            shape->volume_type = CSG_VOLUME_EMPTY;
            //shapes.push_back(std::unique_ptr<csgBrushShape>(shape));
            csg_scene.addShape(shape);
        }
        // Ceiling arch Z axis, C
        {
            csgBrushShape* shape = new csgBrushShape;
            csgMakeCylinder(shape, 14, 2.f, 16,
                gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(-4.9, 4, -9))
                * gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(90.f), gfxm::vec3(1, 0, 0)))
            );
            shape->material = mat_wall;
            shape->volume_type = CSG_VOLUME_EMPTY;
            //shapes.push_back(std::unique_ptr<csgBrushShape>(shape));
            csg_scene.addShape(shape);
        }

        // Pillar A
        {
            csgBrushShape* shape = new csgBrushShape;
            csgMakeCube(shape, .9f, .25f, .9f, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(-2.5f, .125f, .5f)));
            shape->material = mat_wall;
            //shapes.push_back(std::unique_ptr<csgBrushShape>(shape));
            csg_scene.addShape(shape);

            csgBrushShape* shape_pillar = new csgBrushShape;
            csgMakeCylinder(shape_pillar, 4.f, .3f, 16, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(-2.5f, .25f, .5f)));
            shape_pillar->volume_type = CSG_VOLUME_SOLID;
            //shape_pillar->rgba = gfxm::make_rgba32(.5f, .7f, .0f, 1.f);
            shape_pillar->material = mat_wall2;
            //shapes.push_back(std::unique_ptr<csgBrushShape>(shape_pillar));
            csg_scene.addShape(shape_pillar);
        }
        // Pillar B
        {
            csgBrushShape* shape = new csgBrushShape;
            csgMakeCube(shape, .9f, .25f, .9f, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(2.5f, .125f, .5f)));
            shape->material = mat_wall;
            //shapes.push_back(std::unique_ptr<csgBrushShape>(shape));
            csg_scene.addShape(shape);

            csgBrushShape* shape_pillar = new csgBrushShape;
            csgMakeCylinder(shape_pillar, 4.f, .3f, 16, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(2.5f, .25f, .5f)));
            shape_pillar->volume_type = CSG_VOLUME_SOLID;
            //shape_pillar->rgba = gfxm::make_rgba32(.5f, .7f, .0f, 1.f);
            shape_pillar->material = mat_wall2;
            //shapes.push_back(std::unique_ptr<csgBrushShape>(shape_pillar));
            csg_scene.addShape(shape_pillar);
        }
        // Pillar C
        {
            csgBrushShape* shape = new csgBrushShape;
            csgMakeCube(shape, .9f, .25f, .9f, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(-2.5f, .125f, -4.5f)));
            shape->material = mat_wall;
            //shapes.push_back(std::unique_ptr<csgBrushShape>(shape));
            csg_scene.addShape(shape);

            csgBrushShape* shape_pillar = new csgBrushShape;
            csgMakeCylinder(shape_pillar, 4.f, .3f, 16, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(-2.5f, .25f, -4.5f)));
            shape_pillar->volume_type = CSG_VOLUME_SOLID;
            //shape_pillar->rgba = gfxm::make_rgba32(.5f, .7f, .0f, 1.f);
            shape_pillar->material = mat_wall2;
            //shapes.push_back(std::unique_ptr<csgBrushShape>(shape_pillar));
            csg_scene.addShape(shape_pillar);
        }
        // Pillar D
        {
            csgBrushShape* shape = new csgBrushShape;
            csgMakeCube(shape, .9f, .25f, .9f, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(2.5f, .125f, -4.5f)));
            shape->material = mat_wall;
            //shapes.push_back(std::unique_ptr<csgBrushShape>(shape));
            csg_scene.addShape(shape);

            csgBrushShape* shape_pillar = new csgBrushShape;
            csgMakeCylinder(shape_pillar, 4.f, .3f, 16, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(2.5f, .25f, -4.5f)));
            shape_pillar->volume_type = CSG_VOLUME_SOLID;
            //shape_pillar->rgba = gfxm::make_rgba32(.5f, .7f, .0f, 1.f);
            shape_pillar->material = mat_wall2;
            //shapes.push_back(std::unique_ptr<csgBrushShape>(shape_pillar));
            csg_scene.addShape(shape_pillar);
        }

        {
            csgBrushShape* shape_doorway = new csgBrushShape;
            csgMakeCube(shape_doorway, 3.0, 3.5, .25f, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(0, 1.75, 5.125)));
            shape_doorway->volume_type = CSG_VOLUME_EMPTY;
            //shape_doorway->rgba = gfxm::make_rgba32(.0f, .5f, 1.f, 1.f);
            shape_doorway->material = mat_wall;
            //shapes.push_back(std::unique_ptr<csgBrushShape>(shape_doorway));
            csg_scene.addShape(shape_doorway);

            csgBrushShape* shape_arch_part = new csgBrushShape;
            csgMakeCylinder(shape_arch_part, .25f, 1.5, 16,
                gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(0, 3.5, 5.25))
                * gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(-90), gfxm::vec3(1, 0, 0)))
            );
            shape_arch_part->volume_type = CSG_VOLUME_EMPTY;
            //shape_arch_part->rgba = gfxm::make_rgba32(.5f, 1.f, .0f, 1.f);
            shape_arch_part->material = mat_wall;
            //shapes.push_back(std::unique_ptr<csgBrushShape>(shape_arch_part));
            csg_scene.addShape(shape_arch_part);
        }
        {
            csgBrushShape* shape_doorway = new csgBrushShape;
            csgMakeCube(shape_doorway, 2, 3.5, 1, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(0, 1.75, 5.5)));
            shape_doorway->volume_type = CSG_VOLUME_EMPTY;
            //shape_doorway->rgba = gfxm::make_rgba32(.0f, .5f, 1.f, 1.f);
            shape_doorway->material = mat_wall;
            //shapes.push_back(std::unique_ptr<csgBrushShape>(shape_doorway));
            csg_scene.addShape(shape_doorway);
            /*
            csgBrushShape* shape_arch_part = new csgBrushShape;
            csgMakeCylinder(shape_arch_part, 1, 1, 16,
                gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(0, 2.5, 6))
                * gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(-90), gfxm::vec3(1, 0, 0)))
            );
            shape_arch_part->volume_type = CSG_VOLUME_EMPTY;
            shape_arch_part->rgba = gfxm::make_rgba32(.5f, 1.f, .0f, 1.f);
            shape_arch_part->material = &mat_wall;
            shapes.push_back(std::unique_ptr<csgBrushShape>(shape_arch_part));
            csg_scene.addShape(shape_arch_part);*/
        }
        csgBrushShape* shape_room2 = new csgBrushShape;
        csgBrushShape* shape_window = new csgBrushShape;
        csgMakeCube(shape_room2, 10.f, 4.f, 10.f, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(-1.5, 2, 11.)));
        csgMakeCube(shape_window, 2.5, 2.5, 1, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(-3.5, 2., 5.5)));





        shape_room2->volume_type = CSG_VOLUME_EMPTY;
        //shape_room2->rgba = gfxm::make_rgba32(.0f, 1.f, .5f, 1.f);
        shape_room2->material = mat_floor2;
        shape_room2->faces[2]->uv_scale = gfxm::vec2(5, 5);

        shape_window->volume_type = CSG_VOLUME_EMPTY;
        //shape_window->rgba = gfxm::make_rgba32(.5f, 1.f, .0f, 1.f);
        shape_window->material = mat_wall2;

        auto sphere = new csgBrushShape;
        csgMakeSphere(sphere, 32, 1.f, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(0,4.5,-2)) * gfxm::scale(gfxm::mat4(1.f), gfxm::vec3(1, 1, 1)));
        sphere->volume_type = CSG_VOLUME_SOLID;
        //sphere->rgba = gfxm::make_rgba32(1,1,1,1);
        sphere->material = mat_planet;

        //shapes.push_back(std::unique_ptr<csgBrushShape>(shape_room2));
        //shapes.push_back(std::unique_ptr<csgBrushShape>(shape_window));
        //shapes.push_back(std::unique_ptr<csgBrushShape>(sphere));
        
        csg_scene.addShape(shape_room2);
        csg_scene.addShape(shape_window);
        csg_scene.addShape(sphere);
        csg_scene.update();

        rebuildMeshes();

    }
    ~GuiCsgWindow() {
        game_render_instances.erase(&render_instance);
    }

    csgScene& getScene() {
        return csg_scene;
    }

    void updateGpuMesh(int mesh_idx, const csgMeshData& mesh, const Mesh3d& cpu_mesh) {
        auto material_ = resGet<gpuMaterial>("materials/csg/csg_default.mat");

        RHSHARED<gpuMesh> gpu_mesh;
        gpu_mesh.reset_acquire();
        gpu_mesh->setData(&cpu_mesh);
        gpu_mesh->setDrawMode(MESH_DRAW_MODE::MESH_DRAW_TRIANGLES);

        {
            auto mesh_component = model->addComponent<sklmMeshComponent>(MKSTR(mesh_idx).c_str());
            ++mesh_idx;
            mesh_component->bone_name = "Root";
            if (mesh.material && mesh.material->gpu_material) {
                mesh_component->material = mesh.material->gpu_material;
            } else {
                mesh_component->material = material_;
            }
            mesh_component->mesh = gpu_mesh;
        }
    }    
    void generateLightmapUV(
        const gfxm::vec3* vertices, int vertex_count,
        const uint32_t* indices, int index_count,
        std::vector<gfxm::vec2>& lightmap_uvs
    ) {
        LOG("Generating lightmap uvs...");
        xatlas::Atlas* atlas = xatlas::Create();
        assert(atlas);

        xatlas::MeshDecl meshDecl = { 0 };
        meshDecl.vertexCount = vertex_count;
        meshDecl.indexCount = index_count;
        meshDecl.vertexPositionData = vertices;
        meshDecl.vertexPositionStride = sizeof(vertices[0]);
        meshDecl.indexData = indices;
        meshDecl.indexFormat = xatlas::IndexFormat::UInt32;
        auto ret = xatlas::AddMesh(atlas, meshDecl);
        if (ret != xatlas::AddMeshError::Success) {
            LOG_ERR("xatlas error: %i", ret);
            return;
        }

        xatlas::ComputeCharts(atlas);
        xatlas::PackCharts(atlas);

        lightmap_uvs.resize(vertex_count);
        for (int i = 0; i < atlas->meshCount; ++i) {
            const xatlas::Mesh& xmesh = atlas->meshes[i];
            for (int j = 0; j < xmesh.vertexCount; ++j) {
                const xatlas::Vertex& xvertex = xmesh.vertexArray[j];
                uint32_t orig_index = xvertex.xref;
                float xu = xvertex.uv[0] / atlas->width;
                float xv = xvertex.uv[1] / atlas->height;
                lightmap_uvs[orig_index].x = xu;
                lightmap_uvs[orig_index].y = xv;
            }
        }
        LOG("Lightmap uv generation done.");
    }

    void rebuildMeshes() {
        meshes.clear();

        std::unordered_map<csgMaterial*, csgMeshData> mesh_data;
        for (int i = 0; i < csg_scene.shapeCount(); ++i) {
            csgMakeShapeTriangles(csg_scene.getShape(i), mesh_data);
        }
        
        {
            model.reset_acquire();
            skeleton.reset_acquire();
            model->setSkeleton(skeleton);
        }

        int mesh_idx = 0;

        std::vector<gfxm::vec3> unified_vertices;
        std::vector<uint32_t> unified_indices;
        uint32_t unified_base_index = 0;
        for (auto& kv : mesh_data) {
            auto material = kv.first;
            auto& mesh = kv.second;

            if (mesh.indices.size() == 0) {
                continue;
            }
            if (mesh.vertices.size() == 0) {
                continue;
            }

            generateLightmapUV(
                mesh.vertices.data(), mesh.vertices.size(),
                mesh.indices.data(), mesh.indices.size(),
                mesh.uvs_lightmap
            );
            
            Mesh3d cpu_mesh;
            cpu_mesh.clear();
            cpu_mesh.setAttribArray(VFMT::Position_GUID, mesh.vertices.data(), mesh.vertices.size() * sizeof(mesh.vertices[0]));
            cpu_mesh.setAttribArray(VFMT::Normal_GUID, mesh.normals.data(), mesh.normals.size() * sizeof(mesh.normals[0]));
            cpu_mesh.setAttribArray(VFMT::Tangent_GUID, mesh.tangents.data(), mesh.tangents.size() * sizeof(mesh.tangents[0]));
            cpu_mesh.setAttribArray(VFMT::Bitangent_GUID, mesh.bitangents.data(), mesh.bitangents.size() * sizeof(mesh.bitangents[0]));
            cpu_mesh.setAttribArray(VFMT::ColorRGB_GUID, mesh.colors.data(), mesh.colors.size() * sizeof(mesh.colors[0]));
            cpu_mesh.setAttribArray(VFMT::UV_GUID, mesh.uvs.data(), mesh.uvs.size() * sizeof(mesh.uvs[0]));
            cpu_mesh.setAttribArray(VFMT::UVLightmap_GUID, mesh.uvs_lightmap.data(), mesh.uvs_lightmap.size() * sizeof(mesh.uvs_lightmap[0]));
            cpu_mesh.setIndexArray(mesh.indices.data(), mesh.indices.size() * sizeof(mesh.indices[0]));

            updateGpuMesh(mesh_idx, mesh, cpu_mesh);
            
            {
                unified_vertices.insert(unified_vertices.end(), mesh.vertices.begin(), mesh.vertices.end());
                for (auto& idx : mesh.indices) {
                    unified_indices.push_back(idx + unified_base_index);
                }
                unified_base_index = unified_vertices.size();
            }


            auto material_ = resGet<gpuMaterial>("materials/csg/csg_default.mat");
            auto ptr = new Mesh;
            ptr->vertex_buffer.setArrayData(mesh.vertices.data(), mesh.vertices.size() * sizeof(mesh.vertices[0]));
            ptr->normal_buffer.setArrayData(mesh.normals.data(), mesh.normals.size() * sizeof(mesh.normals[0]));
            ptr->tangent_buffer.setArrayData(mesh.tangents.data(), mesh.tangents.size() * sizeof(mesh.tangents[0]));
            ptr->bitangent_buffer.setArrayData(mesh.bitangents.data(), mesh.bitangents.size() * sizeof(mesh.bitangents[0]));
            ptr->color_buffer.setArrayData(mesh.colors.data(), mesh.colors.size() * sizeof(mesh.colors[0]));
            ptr->uv_buffer.setArrayData(mesh.uvs.data(), mesh.uvs.size() * sizeof(mesh.uvs[0]));
            ptr->uv_lightmap_buffer.setArrayData(mesh.uvs_lightmap.data(), mesh.uvs_lightmap.size() * sizeof(mesh.uvs_lightmap[0]));
            ptr->index_buffer.setArrayData(mesh.indices.data(), mesh.indices.size() * sizeof(mesh.indices[0]));

            ptr->lm_vertices = mesh.vertices;
            ptr->lm_normals = mesh.normals;
            ptr->lm_uv = mesh.uvs_lightmap;
            ptr->lm_indices = mesh.indices;

            ptr->mesh_desc.reset(new gpuMeshDesc);
            ptr->mesh_desc->setDrawMode(MESH_DRAW_MODE::MESH_DRAW_TRIANGLES);
            ptr->mesh_desc->setAttribArray(VFMT::Position_GUID, &ptr->vertex_buffer, 0);
            ptr->mesh_desc->setAttribArray(VFMT::Normal_GUID, &ptr->normal_buffer, 0);
            ptr->mesh_desc->setAttribArray(VFMT::Tangent_GUID, &ptr->tangent_buffer, 0);
            ptr->mesh_desc->setAttribArray(VFMT::Bitangent_GUID, &ptr->bitangent_buffer, 0);
            ptr->mesh_desc->setAttribArray(VFMT::ColorRGB_GUID, &ptr->color_buffer, 4);
            ptr->mesh_desc->setAttribArray(VFMT::UV_GUID, &ptr->uv_buffer, 0);
            ptr->mesh_desc->setAttribArray(VFMT::UVLightmap_GUID, &ptr->uv_lightmap_buffer, 0);
            ptr->mesh_desc->setIndexArray(&ptr->index_buffer);
            ptr->material = mesh.material;

            ptr->renderable.setMeshDesc(ptr->mesh_desc.get());
            if (mesh.material && mesh.material->gpu_material) {
                ptr->renderable.setMaterial(mesh.material->gpu_material.get());
            } else {
                ptr->renderable.setMaterial(material_.get());
            }
            ptr->renderable_ubuf = gpuGetPipeline()->createUniformBuffer(UNIFORM_BUFFER_MODEL);
            ptr->renderable.attachUniformBuffer(ptr->renderable_ubuf);
            ptr->renderable_ubuf->setMat4(ptr->renderable_ubuf->getDesc()->getUniform(UNIFORM_MODEL_TRANSFORM), gfxm::mat4(1.f));
            std::string dbg_name = MKSTR("csg_" << (int)ptr->material);
            ptr->renderable.dbg_name = dbg_name;
            ptr->renderable.compile();

            meshes.push_back(std::unique_ptr<Mesh>(ptr));
        }
        collision_mesh.setData(unified_vertices.data(), unified_vertices.size(), unified_indices.data(), unified_indices.size());

        render_instance.render_bucket->clear();
    }

    void generateLightmaps() {
        LOG_DBG("Lightmap generation starts...");

        HSHARED<gpuShaderProgram> prog = loadShaderProgramForLightmapSampling("shaders/default_lightmap_sample.glsl");        

        lm_context* ctx = lmCreate(
            64,
            .001f, 100.f,
            1.0f, 1.0f, 1.0f,
            2, .01f,
            .0f
        );
        assert(ctx);

        auto ubuf_model = gpuGetPipeline()->createUniformBuffer(UNIFORM_BUFFER_MODEL);
        auto loc_transform = ubuf_model->getDesc()->getUniform(UNIFORM_MODEL_TRANSFORM);
        GLint gl_id = ubuf_model->gpu_buf.getId();

        const int bounce_count = 4;
        const int image_width = 1024;
        const int image_height = 1024;
        const int image_bpp = 3;

        std::vector<float> temp(image_width * image_height * image_bpp);

        std::vector<gpuMeshShaderBinding*> bindings;
        std::vector<gfxm::mat4> transforms;
        for (int j = 0; j < meshes.size(); ++j) {
            Mesh* mesh = meshes[j].get();
            mesh->lightmap_data.resize(image_width * image_height * image_bpp);
            memset(mesh->lightmap_data.data(), 0, sizeof(mesh->lightmap_data[0]) * image_width * image_height * image_bpp);
            mesh->lightmap.reset_acquire();
            mesh->lightmap->setData(mesh->lightmap_data.data(), image_width, image_height, image_bpp, IMAGE_CHANNEL_FLOAT, false);

            mesh->mesh_shader_binding = gpuCreateMeshShaderBinding(prog.get(), mesh->mesh_desc.get());

            if (mesh->material && mesh->material->gpu_material.getReferenceName() == "materials/csg/default_hole.mat") {
                continue;
            }
            bindings.push_back(mesh->mesh_shader_binding);
        }

        for (int i = 0; i < bounce_count; ++i) {
            for (int j = 0; j < meshes.size(); ++j) {
                Mesh* mesh = meshes[j].get();

                gfxm::mat4 transform = gfxm::mat4(1.f);

                memset(mesh->lightmap_data.data(), 0, sizeof(mesh->lightmap_data[0]) * image_width * image_height * image_bpp);
                lmSetTargetLightmap(ctx, mesh->lightmap_data.data(), image_width, image_width, image_bpp);
                lmSetGeometry(
                    ctx, (float*)&transform,
                    LM_FLOAT, mesh->lm_vertices.data(), sizeof(float) * 3,   // Positions
                    LM_FLOAT, mesh->lm_normals.data(), sizeof(float) * 3,   // Normals
                    LM_FLOAT, mesh->lm_uv.data(), sizeof(float) * 2,   // Lightmap UV
                    mesh->lm_indices.size(), LM_UNSIGNED_INT, mesh->lm_indices.data()             // Indices
                );

                int vp[4];
                gfxm::mat4 view = gfxm::mat4(1.f);
                gfxm::mat4 proj = gfxm::mat4(1.f);
                float progress;
                while (lmBegin(ctx, vp, (float*)&view, (float*)&proj)) {
                    DRAW_PARAMS params = {
                        .view = view,
                        .projection = proj,
                        .viewport_x = vp[0],
                        .viewport_y = vp[1],
                        .viewport_width = vp[2],
                        .viewport_height = vp[3]
                    };
                    glActiveTexture(GL_TEXTURE0 + 1);
                    glBindTexture(GL_TEXTURE_2D, mesh->lightmap);

                    glBindBufferBase(GL_UNIFORM_BUFFER, ubuf_model->getDesc()->id, gl_id);
                    ubuf_model->setMat4(loc_transform, gfxm::mat4(1.f));

                    glUseProgram(prog->getId());
                    gpuDrawLightmapSample(bindings.data(), bindings.size(), params);
                    //platformSwapBuffers();
                    //gpuDraw(&render_bucket, &render_target, params);
                    //gpuDrawTextureToFramebuffer(render_target.getTexture("Final"), framebuffer, vp);
                    progress = lmProgress(ctx);

                    lmEnd(ctx);
                }
                LOG("Lightmapping bounce: " << i + 1 << "/" << bounce_count << ", mesh: " << j + 1 << "/" << meshes.size());
            }

            for (int i = 0; i < meshes.size(); ++i) {
                Mesh* mesh = meshes[i].get();

                lmImageDilate(mesh->lightmap_data.data(), temp.data(), image_width, image_height, image_bpp);
                lmImageDilate(temp.data(), mesh->lightmap_data.data(), image_width, image_height, image_bpp);

                mesh->lightmap.reset_acquire();
                mesh->lightmap->setData(mesh->lightmap_data.data(), image_width, image_height, image_bpp, IMAGE_CHANNEL_FLOAT, false);

                mesh->renderable.addSamplerOverride("texLightmap", mesh->lightmap);
                mesh->renderable.compile();
            }
        }
        glUseProgram(0);

        lmDestroy(ctx);

        // gamma correct and save lightmaps to disk
        for (int i = 0; i < meshes.size(); i++) {
            Mesh* mesh = meshes[i].get();
            lmImagePower(mesh->lightmap_data.data(), image_width, image_height, image_bpp, 1.0f / 2.2f);
            lmImageSaveTGAf(MKSTR("lightmap_test/lm" << i << ".tga").c_str(), mesh->lightmap_data.data(), image_width, image_height, image_bpp);
        }

        LOG_DBG("Lightmap generation done.");
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::KEYDOWN:
            switch (params.getA<uint16_t>()) {
            case 0x43: // C key
                viewport.clearTools();
                viewport.addTool(&tool_create_box);
                return true;
            case 0x51: // Q
                if (!selected_shapes.empty()) {
                    for (int i = 0; i < selected_shapes.size(); ++i) {
                        selected_shapes[i]->volume_type = (selected_shapes[i]->volume_type == CSG_VOLUME_SOLID) ? CSG_VOLUME_EMPTY : CSG_VOLUME_SOLID;
                        csg_scene.invalidateShape(selected_shapes[i]);
                    }
                    csg_scene.update();
                    rebuildMeshes();
                }
                return true;
            case 0x56: // V - cut
                if (!selected_shapes.empty()) {
                    viewport.clearTools();
                    viewport.addTool(&tool_cut);
                    tool_cut.setData(selected_shapes.back());
                }
                return true;
            case 0x31: // 1
                viewport.clearTools();
                viewport.addTool(&tool_object_mode);
                return true;
            case 0x32: // 2
                if (!tool_object_mode.selected_shapes.empty()) {
                    viewport.clearTools();
                    viewport.addTool(&tool_face_mode);
                    tool_face_mode.setShapeData(&csg_scene, tool_object_mode.selected_shapes.back());
                }
                return true;
            case 0x4C: // L
                generateLightmaps();
                return true;
            }
            break;
        case GUI_MSG::NOTIFY:
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::VIEWPORT_DRAG_DROP_HOVER: {
                return true;
            }
            case GUI_NOTIFY::VIEWPORT_DRAG_DROP: {
                csgBrushShape* shape = 0;
                gfxm::ray R = viewport.makeRayFromMousePos();
                csg_scene.pickShape(R.origin, R.origin + R.direction * R.length, &shape);
                if (shape) {
                    GUI_DRAG_PAYLOAD* pld = guiDragGetPayload();
                    if (pld->type != GUI_DRAG_FILE) {
                        LOG_DBG("No payload");
                        return true;
                    }
                    std::string path = *(std::string*)pld->payload_ptr;
                    RHSHARED<gpuMaterial> mat = resGet<gpuMaterial>(path.c_str());
                    LOG_DBG(path);
                    if (!mat) {
                        return true;
                    }
                    auto csg_mat = csg_scene.getMaterial(mat.getReferenceName().c_str());
                    if (!csg_mat) {
                        csg_mat = csg_scene.createMaterial(mat.getReferenceName().c_str());
                        csg_mat->gpu_material = mat;
                    }
                    shape->material = csg_mat;
                    for (auto& f : shape->faces) {
                        f->material = 0;
                    }
                    rebuildMeshes();
                }
                return true;
            }
            case GUI_NOTIFY::VIEWPORT_TOOL_DONE: {
                viewport.clearTools();
                viewport.addTool(&tool_object_mode);
                return true;
            }
            case GUI_NOTIFY::CSG_REBUILD:
                rebuildMeshes();
                return true;
            case GUI_NOTIFY::CSG_SHAPE_SELECTED:
                selected_shapes.clear();
                selected_shapes.push_back(params.getB<csgBrushShape*>());
                return true;
            case GUI_NOTIFY::CSG_SHAPE_CREATED: {
                auto ptr = params.getB<csgBrushShape*>();
                selected_shapes.clear();
                selected_shapes.push_back(ptr);
                tool_object_mode.selectShape(ptr, false);

                //shapes.push_back(std::unique_ptr<csgBrushShape>(ptr));
                for (int i = 0; i < ptr->planes.size(); ++i) {
                    auto& face = ptr->faces[i];
                    gfxm::vec3& N = face->N;
                    if (fabsf(gfxm::dot(gfxm::vec3(0, 1, 0), N)) < .707f) {
                        face->material = mat_wall_def;
                    } else {
                        face->material = mat_floor_def;
                    }
                }
                //selected_shape = ptr;
                csg_scene.addShape(ptr);
                csg_scene.update();
                rebuildMeshes();
                return true;
            }
            case GUI_NOTIFY::CSG_SHAPE_CHANGED: {
                csg_scene.update();
                rebuildMeshes();
                return true;
            }
            case GUI_NOTIFY::CSG_SHAPE_DELETE: {
                auto shape = params.getB<csgBrushShape*>();
                if (shape) {
                    csg_scene.removeShape(shape);
                    selected_shapes.clear();
                    csg_scene.update();
                    rebuildMeshes();
                }
                return true;
            }
            }
            break;
        }
        return GuiWindow::onMessage(msg, params);
    }
    void onHitTest(GuiHitResult& hit, int x, int y) override {
        GuiWindow::onHitTest(hit, x, y);
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        GuiWindow::onLayout(rc, flags);

        {
            gfxm::ray R = viewport.makeRayFromMousePos();
            gfxm::vec3 hit;
            gfxm::vec3 N;
            gfxm::vec3 plane_origin;
            gfxm::mat3 orient;
            if (csg_scene.castRay(R.origin, R.origin + R.direction * R.length, hit, N, plane_origin, orient)) {
                viewport.pivot_reset_point = hit;
            } else if(gfxm::intersect_line_plane_point(R.origin, R.direction, gfxm::vec3(0, 1, 0), .0f, hit)) {
                viewport.pivot_reset_point = hit;
            }            
        }
        for (auto& mesh : meshes) {
            render_instance.render_bucket->add(&mesh->renderable);
        }
    }
    void onDraw() override {
        GuiWindow::onDraw();

        /*
        auto proj = gfxm::perspective(gfxm::radian(65.0f),
            render_instance.render_target->getWidth() / (float)render_instance.render_target->getHeight(), 0.01f, 1000.0f);
        guiPushViewportRect(viewport.getClientArea()); // TODO: Do this automatically
        guiPushProjection(proj);
        guiPushViewTransform(render_instance.view_transform);
        if (selected_shape) {
            for (auto shape : selected_shape->intersecting_shapes) {
                guiDrawAABB(shape->aabb, gfxm::mat4(1.f), 0xFFCCCCCC);
            }
        } else {
            for (auto& shape : shapes) {
                guiDrawAABB(shape->aabb, gfxm::mat4(1.f), 0xFFCCCCCC);
            }
        }
        guiPopViewTransform();
        guiPopProjection();
        guiPopViewportRect();*/

        auto proj = gfxm::perspective(gfxm::radian(65.0f),
            render_instance.render_target->getWidth() / (float)render_instance.render_target->getHeight(), 0.01f, 1000.0f);
        guiPushViewportRect(viewport.getClientArea()); // TODO: Do this automatically
        guiPushProjection(proj);
        guiPushViewTransform(render_instance.view_transform);

        if (!selected_shapes.empty()) {
            for (int i = 0; i < selected_shapes.size(); ++i) {
                for (auto& face : selected_shapes[i]->faces) {/*
                    for (auto& frag : face->fragments) {
                        for (int i = 0; i < frag.vertices.size(); ++i) {
                            int j = (i + 1) % frag.vertices.size();
                            guiDrawLine3(frag.vertices[i].position, frag.vertices[j].position, 0xFFFFFFFF);

                            guiDrawLine3(frag.vertices[i].position, frag.vertices[i].position + frag.vertices[i].normal * .2f, 0xFFFF0000);
                        }
                    }*/

                    /*
                    for (int i = 0; i < face->control_points.size(); ++i) {
                        gfxm::vec3 a = selected_shape->world_space_vertices[face->control_points[i]->index];
                        gfxm::vec3 b = a + face->normals[i] * .2f;
                        guiDrawLine3(a, b, 0xFFFF0000);
                    }*/
                }
            }
        }

        guiPopViewTransform();
        guiPopProjection();
        guiPopViewportRect();
    }
};
class GuiCsgDocument : public GuiEditorWindow {
    GuiDockSpace dock_space;
    GuiCsgWindow csg_viewport;
public:
    GuiCsgDocument()
        : GuiEditorWindow("CsgDocument", "csg") {
        dock_space.setDockGroup(this);
        csg_viewport.setDockGroup(this);

        addChild(&dock_space);
        csg_viewport.setOwner(this);
        dock_space.getRoot()->addWindow(&csg_viewport);
    }

    bool onSaveCommand(const std::string& path) override {
        nlohmann::json j;
        std::ofstream f(path);
        if (!f.is_open()) {
            return false;
        }
        auto& scene = csg_viewport.getScene();
        scene.serializeJson(j);
        f << j.dump(4);

        {
            nlohmann::json j;
            type_get<mdlSkeletalModelMaster>().serialize_json(j, csg_viewport.model.get());
            std::ofstream f(path + ".skeletal_model");
            if (!f.is_open()) {
                return false;
            }
            f << j.dump(4);
        }
        {
            std::vector<uint8_t> bytes;
            csg_viewport.collision_mesh.serialize(bytes);
            FILE* f = fopen((path + ".collision_mesh").c_str(), "wb");
            if (!f) {
                return false;
            }
            fwrite(bytes.data(), bytes.size(), 1, f);
            fclose(f);
        }

        return true;
    }
    bool onOpenCommand(const std::string& path) override {
        std::ifstream f(path);
        if (!f.is_open()) {
            return false;
        }
        nlohmann::json j;
        j << f;

        if (!csg_viewport.getScene().deserializeJson(j)) {
            return false;
        }
        csg_viewport.getScene().update();
        csg_viewport.rebuildMeshes();

        return true;
    }
};
