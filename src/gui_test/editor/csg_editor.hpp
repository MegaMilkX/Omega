#pragma once

#include "editor_window.hpp"

#include "gui/elements/viewport/gui_viewport.hpp"

#include "gui/elements/viewport/tools/gui_viewport_tool_csg_object_mode.hpp"
#include "gui/elements/viewport/tools/gui_viewport_tool_csg_face_mode.hpp"
#include "gui/elements/viewport/tools/gui_viewport_tool_transform.hpp"
#include "gui/elements/viewport/tools/gui_viewport_tool_csg_create_box.hpp"
#include "gui/elements/viewport/tools/gui_viewport_tool_csg_cut.hpp"

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
    csgMaterial mat_floor;
    csgMaterial mat_floor2;
    csgMaterial mat_wall;
    csgMaterial mat_wall2;
    csgMaterial mat_ceiling;
    csgMaterial mat_planet;
    csgMaterial mat_floor_def;
    csgMaterial mat_wall_def;

    std::vector<std::unique_ptr<csgBrushShape>> shapes;
    csgBrushShape* selected_shape = 0;

    std::map<uint64_t, std::unique_ptr<csgMaterial>> materials;

    struct Mesh {
        csgMaterial* material = 0;
        gpuBuffer vertex_buffer;
        gpuBuffer normal_buffer;
        gpuBuffer color_buffer;
        gpuBuffer uv_buffer;
        gpuBuffer index_buffer;
        std::unique_ptr<gpuMeshDesc> mesh_desc;
        gpuUniformBuffer* renderable_ubuf = 0;
        gpuRenderable renderable;

        ~Mesh() {
            if (renderable_ubuf) {
                gpuGetPipeline()->destroyUniformBuffer(renderable_ubuf);
            }
        }
    };
    std::vector<std::unique_ptr<Mesh>> meshes;
public:
    GuiCsgWindow()
        : GuiWindow("CSG"),
        render_bucket(gpuGetPipeline(), 1000),
        render_target(800, 600),
        tool_object_mode(&csg_scene),
        tool_create_box(&csg_scene),
        tool_cut(&csg_scene)
    {
        tool_object_mode.setOwner(this);
        tool_face_mode.setOwner(this);
        tool_create_box.setOwner(this);
        tool_cut.setOwner(this);

        padding = gfxm::rect(1, 1, 1, 1);
        addChild(&viewport);
        viewport.setOwner(this);

        gpuGetPipeline()->initRenderTarget(&render_target);
        
        render_instance.render_bucket = &render_bucket;
        render_instance.render_target = &render_target;
        render_instance.view_transform = gfxm::mat4(1.0f);
        game_render_instances.insert(&render_instance);

        viewport.render_instance = &render_instance;
        guiDragSubscribe(&viewport);

        mat_floor.gpu_material = resGet<gpuMaterial>("materials/csg/floor.mat");
        mat_floor2.gpu_material = resGet<gpuMaterial>("materials/csg/floor2.mat");
        mat_wall.gpu_material = resGet<gpuMaterial>("materials/csg/wall.mat");
        mat_wall2.gpu_material = resGet<gpuMaterial>("materials/csg/wall2.mat");
        mat_ceiling.gpu_material = resGet<gpuMaterial>("materials/csg/ceiling.mat");
        mat_planet.gpu_material = resGet<gpuMaterial>("materials/csg/planet.mat");
        mat_floor_def.gpu_material = resGet<gpuMaterial>("materials/csg/default_floor.mat");
        mat_wall_def.gpu_material = resGet<gpuMaterial>("materials/csg/default_wall.mat");

        csgBrushShape* shape_room = new csgBrushShape;
        csgMakeCube(shape_room, 14.f, 4.f, 14.f, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(0, 2, -2)));
        shape_room->volume_type = VOLUME_EMPTY;
        shape_room->rgba = gfxm::make_rgba32(0.7, .4f, .6f, 1.f);
        shape_room->faces[0]->uv_scale = gfxm::vec2(2.f, 2.f);
        shape_room->faces[0]->material = &mat_wall;
        shape_room->faces[1]->uv_scale = gfxm::vec2(2.f, 2.f);
        shape_room->faces[1]->material = &mat_wall;
        shape_room->faces[4]->uv_scale = gfxm::vec2(2.f, 2.f);
        shape_room->faces[4]->material = &mat_wall;
        shape_room->faces[5]->uv_scale = gfxm::vec2(2.f, 2.f);
        shape_room->faces[5]->material = &mat_wall;
        shape_room->faces[2]->uv_scale = gfxm::vec2(5.f, 5.f);
        shape_room->faces[2]->uv_offset = gfxm::vec2(2.5f, 2.5f);
        shape_room->faces[2]->material = &mat_floor;
        shape_room->faces[3]->uv_scale = gfxm::vec2(3.f, 3.f);
        shape_room->faces[3]->uv_offset = gfxm::vec2(.0f, .0f);
        shape_room->faces[3]->material = &mat_ceiling;
        shapes.push_back(std::unique_ptr<csgBrushShape>(shape_room));
        csg_scene.addShape(shape_room);

        // Ceiling arch X axis, A
        {
            csgBrushShape* shape = new csgBrushShape;
            csgMakeCylinder(shape, 14, 2.f, 16,
                gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(7, 4, 2.9))
                * gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(90.f), gfxm::vec3(0, 0, 1)))
            );
            shape->material = &mat_wall;
            shape->volume_type = VOLUME_EMPTY;
            shapes.push_back(std::unique_ptr<csgBrushShape>(shape));
            csg_scene.addShape(shape);
        }
        // Ceiling arch X axis, B
        {
            csgBrushShape* shape = new csgBrushShape;
            csgMakeCylinder(shape, 14, 2.f, 16,
                gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(7, 4, -2))
                * gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(90.f), gfxm::vec3(0, 0, 1)))
            );
            shape->material = &mat_wall;
            shape->volume_type = VOLUME_EMPTY;
            shapes.push_back(std::unique_ptr<csgBrushShape>(shape));
            csg_scene.addShape(shape);
        }
        // Ceiling arch X axis, C
        {
            csgBrushShape* shape = new csgBrushShape;
            csgMakeCylinder(shape, 14, 2.f, 16,
                gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(7, 4, -6.9))
                * gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(90.f), gfxm::vec3(0, 0, 1)))
            );
            shape->material = &mat_wall;
            shape->volume_type = VOLUME_EMPTY;
            shapes.push_back(std::unique_ptr<csgBrushShape>(shape));
            csg_scene.addShape(shape);
        }
        // Ceiling arch Z axis, A
        {
            csgBrushShape* shape = new csgBrushShape;
            csgMakeCylinder(shape, 14, 2.f, 16,
                gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(0, 4, -9))
                * gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(90.f), gfxm::vec3(1, 0, 0)))
            );
            shape->material = &mat_wall;
            shape->volume_type = VOLUME_EMPTY;
            shapes.push_back(std::unique_ptr<csgBrushShape>(shape));
            csg_scene.addShape(shape);
        }
        // Ceiling arch Z axis, B
        {
            csgBrushShape* shape = new csgBrushShape;
            csgMakeCylinder(shape, 14, 2.f, 16,
                gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(4.9, 4, -9))
                * gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(90.f), gfxm::vec3(1, 0, 0)))
            );
            shape->material = &mat_wall;
            shape->volume_type = VOLUME_EMPTY;
            shapes.push_back(std::unique_ptr<csgBrushShape>(shape));
            csg_scene.addShape(shape);
        }
        // Ceiling arch Z axis, C
        {
            csgBrushShape* shape = new csgBrushShape;
            csgMakeCylinder(shape, 14, 2.f, 16,
                gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(-4.9, 4, -9))
                * gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(90.f), gfxm::vec3(1, 0, 0)))
            );
            shape->material = &mat_wall;
            shape->volume_type = VOLUME_EMPTY;
            shapes.push_back(std::unique_ptr<csgBrushShape>(shape));
            csg_scene.addShape(shape);
        }

        // Pillar A
        {
            csgBrushShape* shape = new csgBrushShape;
            csgMakeCube(shape, .9f, .25f, .9f, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(-2.5f, .125f, .5f)));
            shape->material = &mat_wall;
            shapes.push_back(std::unique_ptr<csgBrushShape>(shape));
            csg_scene.addShape(shape);

            csgBrushShape* shape_pillar = new csgBrushShape;
            csgMakeCylinder(shape_pillar, 4.f, .3f, 16, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(-2.5f, .25f, .5f)));
            shape_pillar->volume_type = VOLUME_SOLID;
            shape_pillar->rgba = gfxm::make_rgba32(.5f, .7f, .0f, 1.f);
            shape_pillar->material = &mat_wall2;
            shapes.push_back(std::unique_ptr<csgBrushShape>(shape_pillar));
            csg_scene.addShape(shape_pillar);
        }
        // Pillar B
        {
            csgBrushShape* shape = new csgBrushShape;
            csgMakeCube(shape, .9f, .25f, .9f, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(2.5f, .125f, .5f)));
            shape->material = &mat_wall;
            shapes.push_back(std::unique_ptr<csgBrushShape>(shape));
            csg_scene.addShape(shape);

            csgBrushShape* shape_pillar = new csgBrushShape;
            csgMakeCylinder(shape_pillar, 4.f, .3f, 16, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(2.5f, .25f, .5f)));
            shape_pillar->volume_type = VOLUME_SOLID;
            shape_pillar->rgba = gfxm::make_rgba32(.5f, .7f, .0f, 1.f);
            shape_pillar->material = &mat_wall2;
            shapes.push_back(std::unique_ptr<csgBrushShape>(shape_pillar));
            csg_scene.addShape(shape_pillar);
        }
        // Pillar C
        {
            csgBrushShape* shape = new csgBrushShape;
            csgMakeCube(shape, .9f, .25f, .9f, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(-2.5f, .125f, -4.5f)));
            shape->material = &mat_wall;
            shapes.push_back(std::unique_ptr<csgBrushShape>(shape));
            csg_scene.addShape(shape);

            csgBrushShape* shape_pillar = new csgBrushShape;
            csgMakeCylinder(shape_pillar, 4.f, .3f, 16, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(-2.5f, .25f, -4.5f)));
            shape_pillar->volume_type = VOLUME_SOLID;
            shape_pillar->rgba = gfxm::make_rgba32(.5f, .7f, .0f, 1.f);
            shape_pillar->material = &mat_wall2;
            shapes.push_back(std::unique_ptr<csgBrushShape>(shape_pillar));
            csg_scene.addShape(shape_pillar);
        }
        // Pillar D
        {
            csgBrushShape* shape = new csgBrushShape;
            csgMakeCube(shape, .9f, .25f, .9f, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(2.5f, .125f, -4.5f)));
            shape->material = &mat_wall;
            shapes.push_back(std::unique_ptr<csgBrushShape>(shape));
            csg_scene.addShape(shape);

            csgBrushShape* shape_pillar = new csgBrushShape;
            csgMakeCylinder(shape_pillar, 4.f, .3f, 16, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(2.5f, .25f, -4.5f)));
            shape_pillar->volume_type = VOLUME_SOLID;
            shape_pillar->rgba = gfxm::make_rgba32(.5f, .7f, .0f, 1.f);
            shape_pillar->material = &mat_wall2;
            shapes.push_back(std::unique_ptr<csgBrushShape>(shape_pillar));
            csg_scene.addShape(shape_pillar);
        }

        {
            csgBrushShape* shape_doorway = new csgBrushShape;
            csgMakeCube(shape_doorway, 3.0, 3.5, .25f, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(0, 1.75, 5.125)));
            shape_doorway->volume_type = VOLUME_EMPTY;
            shape_doorway->rgba = gfxm::make_rgba32(.0f, .5f, 1.f, 1.f);
            shape_doorway->material = &mat_wall;
            shapes.push_back(std::unique_ptr<csgBrushShape>(shape_doorway));
            csg_scene.addShape(shape_doorway);

            csgBrushShape* shape_arch_part = new csgBrushShape;
            csgMakeCylinder(shape_arch_part, .25f, 1.5, 16,
                gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(0, 3.5, 5.25))
                * gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(-90), gfxm::vec3(1, 0, 0)))
            );
            shape_arch_part->volume_type = VOLUME_EMPTY;
            shape_arch_part->rgba = gfxm::make_rgba32(.5f, 1.f, .0f, 1.f);
            shape_arch_part->material = &mat_wall;
            shapes.push_back(std::unique_ptr<csgBrushShape>(shape_arch_part));
            csg_scene.addShape(shape_arch_part);
        }
        {
            csgBrushShape* shape_doorway = new csgBrushShape;
            csgMakeCube(shape_doorway, 2, 3.5, 1, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(0, 1.75, 5.5)));
            shape_doorway->volume_type = VOLUME_EMPTY;
            shape_doorway->rgba = gfxm::make_rgba32(.0f, .5f, 1.f, 1.f);
            shape_doorway->material = &mat_wall;
            shapes.push_back(std::unique_ptr<csgBrushShape>(shape_doorway));
            csg_scene.addShape(shape_doorway);
            /*
            csgBrushShape* shape_arch_part = new csgBrushShape;
            csgMakeCylinder(shape_arch_part, 1, 1, 16,
                gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(0, 2.5, 6))
                * gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(-90), gfxm::vec3(1, 0, 0)))
            );
            shape_arch_part->volume_type = VOLUME_EMPTY;
            shape_arch_part->rgba = gfxm::make_rgba32(.5f, 1.f, .0f, 1.f);
            shape_arch_part->material = &mat_wall;
            shapes.push_back(std::unique_ptr<csgBrushShape>(shape_arch_part));
            csg_scene.addShape(shape_arch_part);*/
        }
        csgBrushShape* shape_room2 = new csgBrushShape;
        csgBrushShape* shape_window = new csgBrushShape;
        csgMakeCube(shape_room2, 10.f, 4.f, 10.f, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(-1.5, 2, 11.)));
        csgMakeCube(shape_window, 2.5, 2.5, 1, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(-3.5, 2., 5.5)));





        shape_room2->volume_type = VOLUME_EMPTY;
        shape_room2->rgba = gfxm::make_rgba32(.0f, 1.f, .5f, 1.f);
        shape_room2->material = &mat_floor2;
        shape_room2->faces[2]->uv_scale = gfxm::vec2(5, 5);

        shape_window->volume_type = VOLUME_EMPTY;
        shape_window->rgba = gfxm::make_rgba32(.5f, 1.f, .0f, 1.f);
        shape_window->material = &mat_wall2;

        auto sphere = new csgBrushShape;
        csgMakeSphere(sphere, 32, 1.f, gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(0,4.5,-2)) * gfxm::scale(gfxm::mat4(1.f), gfxm::vec3(1, 1, 1)));
        sphere->volume_type = VOLUME_SOLID;
        sphere->rgba = gfxm::make_rgba32(1,1,1,1);
        sphere->material = &mat_planet;

        shapes.push_back(std::unique_ptr<csgBrushShape>(shape_room2));
        shapes.push_back(std::unique_ptr<csgBrushShape>(shape_window));
        shapes.push_back(std::unique_ptr<csgBrushShape>(sphere));
        
        csg_scene.addShape(shape_room2);
        csg_scene.addShape(shape_window);
        csg_scene.addShape(sphere);
        csg_scene.update();

        rebuildMeshes();

        viewport.addTool(&tool_object_mode);
    }
    ~GuiCsgWindow() {
        game_render_instances.erase(&render_instance);
    }
    void rebuildMeshes() {
        meshes.clear();

        std::unordered_map<csgMaterial*, csgMeshData> mesh_data;
        for (int i = 0; i < shapes.size(); ++i) {
            csgMakeShapeTriangles(shapes[i].get(), mesh_data);
        }
        
        auto material_ = resGet<gpuMaterial>("materials/csg/csg_default.mat");
        for (auto& kv : mesh_data) {
            auto material = kv.first;
            auto& mesh = kv.second;

            if (mesh.indices.size() == 0) {
                continue;
            }
            if (mesh.vertices.size() == 0) {
                continue;
            }

            auto ptr = new Mesh;
            ptr->vertex_buffer.setArrayData(mesh.vertices.data(), mesh.vertices.size() * sizeof(mesh.vertices[0]));
            ptr->normal_buffer.setArrayData(mesh.normals.data(), mesh.normals.size() * sizeof(mesh.normals[0]));
            ptr->color_buffer.setArrayData(mesh.colors.data(), mesh.colors.size() * sizeof(mesh.colors[0]));
            ptr->uv_buffer.setArrayData(mesh.uvs.data(), mesh.uvs.size() * sizeof(mesh.uvs[0]));
            ptr->index_buffer.setArrayData(mesh.indices.data(), mesh.indices.size() * sizeof(mesh.indices[0]));
            ptr->mesh_desc.reset(new gpuMeshDesc);
            ptr->mesh_desc->setDrawMode(MESH_DRAW_MODE::MESH_DRAW_TRIANGLES);
            ptr->mesh_desc->setAttribArray(VFMT::Position_GUID, &ptr->vertex_buffer, 0);
            ptr->mesh_desc->setAttribArray(VFMT::Normal_GUID, &ptr->normal_buffer, 0);
            ptr->mesh_desc->setAttribArray(VFMT::ColorRGB_GUID, &ptr->color_buffer, 4);
            ptr->mesh_desc->setAttribArray(VFMT::UV_GUID, &ptr->uv_buffer, 0);
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
        render_instance.render_bucket->clear();
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
                if (selected_shape) {
                    selected_shape->volume_type = (selected_shape->volume_type == VOLUME_SOLID) ? VOLUME_EMPTY : VOLUME_SOLID;
                    csg_scene.invalidateShape(selected_shape);
                    csg_scene.update();
                    rebuildMeshes();
                }
                return true;
            case 0x56: // V - cut
                if (selected_shape) {
                    viewport.clearTools();
                    viewport.addTool(&tool_cut);
                    tool_cut.setData(selected_shape);
                }
                return true;
            case 0x31: // 1
                viewport.clearTools();
                viewport.addTool(&tool_object_mode);
                return true;
            case 0x32: // 2
                if (tool_object_mode.selected_shape) {
                    viewport.clearTools();
                    viewport.addTool(&tool_face_mode);
                    tool_face_mode.setShapeData(&csg_scene, tool_object_mode.selected_shape);
                }
                return true;
            }
            break;
            return true;
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
                    auto it = materials.find(mat.getHandle().handle);
                    if (it == materials.end()) {
                        it = materials.insert(std::make_pair(mat.getHandle().handle, std::unique_ptr<csgMaterial>(new csgMaterial))).first;
                        it->second->gpu_material = mat;
                    }
                    shape->material = it->second.get();
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
            case GUI_NOTIFY::CSG_SHAPE_SELECTED:
                selected_shape = params.getB<csgBrushShape*>();
                return true;
            case GUI_NOTIFY::CSG_SHAPE_CREATED: {
                auto ptr = params.getB<csgBrushShape*>();
                selected_shape = ptr;
                tool_object_mode.selectShape(ptr);

                shapes.push_back(std::unique_ptr<csgBrushShape>(ptr));
                for (int i = 0; i < ptr->planes.size(); ++i) {
                    auto& face = ptr->faces[i];
                    gfxm::vec3& N = face->N;
                    if (fabsf(gfxm::dot(gfxm::vec3(0, 1, 0), N)) < .707f) {
                        face->material = &mat_wall_def;
                    } else {
                        face->material = &mat_floor_def;
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
                    for (auto it = shapes.begin(); it != shapes.end(); ++it) {
                        if (it->get() == shape) {
                            shapes.erase(it);
                            break;
                        }
                    }
                    selected_shape = 0;
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
            if (csg_scene.castRay(R.origin, R.origin + R.direction * R.length, hit, N, plane_origin)) {
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

        if (selected_shape) {
            for (auto& face : selected_shape->faces) {/*
                for (auto& frag : face.fragments) {
                    for (int i = 0; i < frag.vertices.size(); ++i) {
                        int j = (i + 1) % frag.vertices.size();
                        guiDrawLine3(frag.vertices[i].position, frag.vertices[j].position, 0xFFFFFFFF);

                        guiDrawLine3(frag.vertices[i].position, frag.vertices[i].position + frag.vertices[i].normal * .2f, 0xFFFF0000);
                    }
                }*/

                /*
                for (int i = 0; i < face.indices.size(); ++i) {
                    gfxm::vec3 a = selected_shape->world_space_vertices[face.indices[i]];
                    gfxm::vec3 b = a + face.normals[i] * .2f;
                    guiDrawLine3(a, b, 0xFFFF0000);
                }*/
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
        : GuiEditorWindow("CsgDocument") {
        padding = gfxm::rect(0, 0, 0, 0);

        dock_space.setDockGroup(this);
        csg_viewport.setDockGroup(this);

        addChild(&dock_space);
        csg_viewport.setOwner(this);
        dock_space.getRoot()->addWindow(&csg_viewport);
    }
};
