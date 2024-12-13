#pragma once

#include "editor_window.hpp"

#include "gui/elements/viewport/gui_viewport.hpp"

#include "gui/elements/viewport/tools/gui_viewport_tool_csg_object_mode.hpp"
#include "gui/elements/viewport/tools/gui_viewport_tool_csg_face_mode.hpp"
#include "gui/elements/viewport/tools/gui_viewport_tool_transform.hpp"
#include "gui/elements/viewport/tools/gui_viewport_tool_csg_create_box.hpp"
#include "gui/elements/viewport/tools/gui_viewport_tool_csg_cut.hpp"
#include "gui/elements/viewport/tools/gui_viewport_tool_csg_custom_shape.hpp"
#include "gui/elements/viewport/tools/gui_viewport_tool_csg_uv_edit.hpp"

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
    GuiViewportToolCsgCreateCustomShape tool_create_custom_shape;
    GuiViewportToolCsgUVEdit tool_uv_edit;
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
    std::vector<csgObject*> selected_objects;

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
        /*
        std::vector<float> lightmap_data;
        */
        RHSHARED<gpuTexture2d> lightmap;

        RHSHARED<gpuTexture2d> albedo;

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

    RHSHARED<gpuMaterial> default_material;

    struct ReferenceImage {
        gfxm::mat4 transform;

        gpuBuffer vertex_buffer;
        gpuBuffer uv_buffer;

        std::unique_ptr<gpuMeshDesc> mesh_desc;

        HSHARED<gpuMaterial> material;
        gpuUniformBuffer* renderable_ubuf = 0;
        gpuMeshShaderBinding* mesh_shader_binding;
        gpuRenderable renderable;

        
        ~ReferenceImage() {
            if (renderable_ubuf) {
                gpuGetPipeline()->destroyUniformBuffer(renderable_ubuf);
            }
        }
    };
    std::vector<std::unique_ptr<ReferenceImage>> ref_images;

    void createReferenceImage(RHSHARED<gpuTexture2d> tex) {
        ReferenceImage* ref_image = new ReferenceImage;

        gfxm::mat4 transform
            = gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(-70.f, .0f, .0f))
            * gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(90.f), gfxm::vec3(0, 1, 0)))
            * gfxm::scale(gfxm::mat4(1.f), gfxm::vec3(12, 12, 12));

        int width = tex->getWidth();
        int height = tex->getHeight();
        float wh_ratio = width / (float)height;
        width = 10;
        height = width / wh_ratio;

        float vertices[] = {
            -width * .5f, .0f, height * .5f,
            width * .5f, .0f, height * .5f,
            width * .5f, .0f, -height * .5f,
            width * .5f, .0f, -height * .5f,
            -width * .5f, .0f, -height * .5f,
            -width * .5f, .0f, height * .5f,
        };
        float uvs[] = {
            .0f, .0f,
            1.f, .0f,
            1.f, 1.f,
            1.f, 1.f,
            .0f, 1.f,
            .0f, .0f,
        };

        ref_image->vertex_buffer.setArrayData(vertices, sizeof(vertices));
        ref_image->uv_buffer.setArrayData(uvs, sizeof(uvs));

        ref_image->mesh_desc.reset(new gpuMeshDesc);
        ref_image->mesh_desc->setAttribArray(VFMT::Position_GUID, &ref_image->vertex_buffer, 0);
        ref_image->mesh_desc->setAttribArray(VFMT::UV_GUID, &ref_image->uv_buffer, 0);
        ref_image->mesh_desc->setVertexCount(6);
        ref_image->mesh_desc->setDrawMode(MESH_DRAW_TRIANGLES);

        ref_image->material.reset_acquire();
        auto tech = ref_image->material->addTechnique("Overlay");
        auto pass = tech->addPass();
        pass->setShader(loadShaderProgram("core/shaders/editor/image_reference.glsl"));
        pass->cull_faces = false;
        ref_image->material->addSampler("texImage", tex);
        ref_image->material->compile();

        ref_image->renderable.setMaterial(ref_image->material.get());
        ref_image->renderable.setMeshDesc(ref_image->mesh_desc.get());
        ref_image->renderable_ubuf = gpuGetPipeline()->createUniformBuffer(UNIFORM_BUFFER_MODEL);
        ref_image->renderable.attachUniformBuffer(ref_image->renderable_ubuf);
        ref_image->renderable_ubuf->setMat4(ref_image->renderable_ubuf->getDesc()->getUniform(UNIFORM_MODEL_TRANSFORM), transform);
        std::string dbg_name = MKSTR("ImageRef");
        ref_image->renderable.dbg_name = dbg_name;
        ref_image->renderable.compile();        

        ref_images.push_back(std::unique_ptr<ReferenceImage>(ref_image));
    }

    void receiveDragDropMaterial() {
        csgBrushShape* shape = 0;
        gfxm::ray R = viewport.makeRayFromMousePos();
        csg_scene.pickShape(R.origin, R.origin + R.direction * R.length, &shape);
        if (shape) {
            int face_idx = csg_scene.pickShapeFace(R.origin, R.origin + R.direction * R.length, shape);

            GUI_DRAG_PAYLOAD* pld = guiDragGetPayload();
            if (pld->type != GUI_DRAG_FILE) {
                LOG_DBG("No payload");
                return;
            }
            std::string path = *(std::string*)pld->payload_ptr;
            RHSHARED<gpuMaterial> mat = resGet<gpuMaterial>(path.c_str());
            LOG_DBG(path);
            if (!mat) {
                return;
            }
                    
            auto csg_mat = csg_scene.createMaterial(mat.getReferenceName().c_str());
            csg_mat->gpu_material = mat;
                    
            if (guiIsModifierKeyPressed(GUI_KEY_SHIFT)) {
                shape->faces[face_idx]->material = csg_mat;
            } else {
                shape->material = csg_mat;
                for (auto& f : shape->faces) {
                    f->material = 0;
                }
            }
            shape->invalidate();
            csg_scene.update();
            rebuildMeshes();
        }
    }
    void receiveDragDropImage() {
        GUI_DRAG_PAYLOAD* pld = guiDragGetPayload();
        std::string str_path = *(std::string*)pld->payload_ptr;
        RHSHARED<gpuTexture2d> tex = resGet<gpuTexture2d>(str_path.c_str());
        if (!tex.isValid()) {
            return;
        }

        createReferenceImage(tex);
    }
    void receiveDragDrop() {
        GUI_DRAG_PAYLOAD* pld = guiDragGetPayload();
        if (pld->type != GUI_DRAG_FILE) {
            LOG_DBG("No payload");
            return;
        }
        std::string str_path = *(std::string*)pld->payload_ptr;
        std::filesystem::path path = str_path;
        std::string ext = path.extension().string();

        if (ext == ".mat") {
            receiveDragDropMaterial();
        } else if(ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".gif") {
            receiveDragDropImage();
        } else {
            LOG_WARN("Unknown extension: " << ext);
        }
    }

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
        tool_create_custom_shape(&csg_scene),
        tool_uv_edit(&csg_scene),
        tool_cut(&csg_scene)
    {
        model.reset_acquire();
        skeleton.reset_acquire();
        model->setSkeleton(skeleton);

        tool_object_mode.setOwner(this);
        tool_face_mode.setOwner(this);
        tool_create_box.setOwner(this);
        tool_create_custom_shape.setOwner(this);
        tool_uv_edit.setOwner(this);
        tool_cut.setOwner(this);

        addChild(&viewport);
        viewport.setOwner(this);
        //viewport.is_ortho = true;

        gpuGetPipeline()->initRenderTarget(&render_target);
        
        render_instance.render_bucket = &render_bucket;
        render_instance.render_target = &render_target;
        render_instance.view_transform = gfxm::mat4(1.0f);
        game_render_instances.insert(&render_instance);

        viewport.render_instance = &render_instance;
        guiDragSubscribe(&viewport);

        viewport.addTool(&tool_object_mode);
        
        default_material = resGet<gpuMaterial>("materials/csg/csg_default.mat");

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

            mesh_component->bone_name = "Root";
            if (mesh.material && mesh.material->gpu_material) {
                mesh_component->material = mesh.material->gpu_material;
            } else {
                mesh_component->material = material_;
            }
            mesh_component->mesh = gpu_mesh;
        }
    }

    struct GENERATE_LIGHTMAP_UV_DATA {
        const gfxm::vec3* vertices;
        int vertex_count;
        const uint32_t* indices;
        int index_count;
        std::vector<gfxm::vec2>* lightmap_uvs;
    };
    void generateLightmapUVSingleChart(GENERATE_LIGHTMAP_UV_DATA* mesh_set, int mesh_count) {
        LOG("Generating lightmap uvs...");
        xatlas::Atlas* atlas = xatlas::Create();
        assert(atlas);

        for (int i = 0; i < mesh_count; ++i) {
            const gfxm::vec3* vertices = mesh_set[i].vertices;
            int vertex_count = mesh_set[i].vertex_count;
            const uint32_t* indices = mesh_set[i].indices;
            int index_count = mesh_set[i].index_count;

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
        }

        xatlas::ComputeCharts(atlas);
        xatlas::PackCharts(atlas);

        for (int i = 0; i < atlas->meshCount; ++i) {
            const xatlas::Mesh& xmesh = atlas->meshes[i];
            std::vector<gfxm::vec2>& lightmap_uvs = *mesh_set[i].lightmap_uvs;
            int vertex_count = mesh_set[i].vertex_count;
            lightmap_uvs.resize(vertex_count);

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
    void generateLightmapUVSingleChart(csgMeshData** mesh_set, int mesh_count) {
        LOG("Generating lightmap uvs...");
        xatlas::Atlas* atlas = xatlas::Create();
        assert(atlas);

        for (int i = 0; i < mesh_count; ++i) {
            const gfxm::vec3* vertices = mesh_set[i]->vertices.data();
            int vertex_count = mesh_set[i]->vertices.size();
            const uint32_t* indices = mesh_set[i]->indices.data();
            int index_count = mesh_set[i]->indices.size();

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
        }

        xatlas::ComputeCharts(atlas);
        xatlas::PackCharts(atlas);

        for (int i = 0; i < atlas->meshCount; ++i) {
            const xatlas::Mesh& xmesh = atlas->meshes[i];
            std::vector<gfxm::vec2>& lightmap_uvs = mesh_set[i]->uvs_lightmap;
            
            lightmap_uvs.resize(xmesh.vertexCount);
            std::vector<gfxm::vec3> vertices(xmesh.vertexCount);
            std::vector<gfxm::vec3> normals(xmesh.vertexCount);
            std::vector<gfxm::vec3> tangents(xmesh.vertexCount);
            std::vector<gfxm::vec3> bitangents(xmesh.vertexCount);
            std::vector<uint32_t>   colors(xmesh.vertexCount);
            std::vector<gfxm::vec2> uvs(xmesh.vertexCount);

            for (int j = 0; j < xmesh.vertexCount; ++j) {
                const xatlas::Vertex& xvertex = xmesh.vertexArray[j];
                uint32_t orig_index = xvertex.xref;
                float xu = xvertex.uv[0] / atlas->width;
                float xv = xvertex.uv[1] / atlas->height;
                lightmap_uvs[j].x = xu;
                lightmap_uvs[j].y = xv;

                vertices[j] = mesh_set[i]->vertices[orig_index];
                normals[j] = mesh_set[i]->normals[orig_index];
                tangents[j] = mesh_set[i]->tangents[orig_index];
                bitangents[j] = mesh_set[i]->bitangents[orig_index];
                colors[j] = mesh_set[i]->colors[orig_index];
                uvs[j] = mesh_set[i]->uvs[orig_index];
            }

            mesh_set[i]->vertices = vertices;
            mesh_set[i]->normals = normals;
            mesh_set[i]->tangents = tangents;
            mesh_set[i]->bitangents = bitangents;
            mesh_set[i]->colors = colors;
            mesh_set[i]->uvs = uvs;
            mesh_set[i]->indices = std::vector<uint32_t>(xmesh.indexArray, xmesh.indexArray + xmesh.indexCount);
        }
        
        LOG("Lightmap uv generation done.");
    }

    void generateLightmapUV() {
        std::unordered_map<csgMaterial*, std::vector<csgMeshData*>> by_material;

        for (int i = 0; i < csg_scene.shapeCount(); ++i) {
            auto shape = csg_scene.getShape(i);
            for (int j = 0; j < shape->triangulated_meshes.size(); ++j) {
                by_material[shape->triangulated_meshes[j]->material].push_back(shape->triangulated_meshes[j].get());
            }
        }

        for (auto& kv : by_material) {
            auto& meshes = kv.second;
            /*
            std::vector<GENERATE_LIGHTMAP_UV_DATA> gen_data;
            gen_data.resize(meshes.size());
            for (int i = 0; i < meshes.size(); ++i) {
                csgMeshData* mesh = meshes[i];
                gen_data[i] = GENERATE_LIGHTMAP_UV_DATA{
                    .vertices = mesh->vertices.data(),
                    .vertex_count = (int)mesh->vertices.size(),
                    .indices = mesh->indices.data(),
                    .index_count = (int)mesh->indices.size(),
                    .lightmap_uvs = &mesh->uvs_lightmap
                };
            }*/
            //generateLightmapUVSingleChart(gen_data.data(), gen_data.size());
            generateLightmapUVSingleChart(meshes.data(), meshes.size());
        }

        rebuildDisplayMeshes();
    }

    void rebuildDisplayMeshes() {
        meshes.clear();
        for (int i = 0; i < csg_scene.shapeCount(); ++i) {
            auto shape = csg_scene.getShape(i);
            for (int j = 0; j < shape->triangulated_meshes.size(); ++j) {
                csgMeshData* mesh = shape->triangulated_meshes[j].get();
                auto display_mesh = new Mesh;

                display_mesh->vertex_buffer.setArrayData(mesh->vertices.data(), mesh->vertices.size() * sizeof(mesh->vertices[0]));
                display_mesh->normal_buffer.setArrayData(mesh->normals.data(), mesh->normals.size() * sizeof(mesh->normals[0]));
                display_mesh->tangent_buffer.setArrayData(mesh->tangents.data(), mesh->tangents.size() * sizeof(mesh->tangents[0]));
                display_mesh->bitangent_buffer.setArrayData(mesh->bitangents.data(), mesh->bitangents.size() * sizeof(mesh->bitangents[0]));
                display_mesh->color_buffer.setArrayData(mesh->colors.data(), mesh->colors.size() * sizeof(mesh->colors[0]));
                display_mesh->uv_buffer.setArrayData(mesh->uvs.data(), mesh->uvs.size() * sizeof(mesh->uvs[0]));
                display_mesh->uv_lightmap_buffer.setArrayData(mesh->uvs_lightmap.data(), mesh->uvs_lightmap.size() * sizeof(mesh->uvs_lightmap[0]));
                display_mesh->index_buffer.setArrayData(mesh->indices.data(), mesh->indices.size() * sizeof(mesh->indices[0]));

                display_mesh->lm_vertices = mesh->vertices;
                display_mesh->lm_normals = mesh->normals;
                display_mesh->lm_uv = mesh->uvs_lightmap;
                display_mesh->lm_indices = mesh->indices;

                display_mesh->mesh_desc.reset(new gpuMeshDesc);
                display_mesh->mesh_desc->setDrawMode(MESH_DRAW_MODE::MESH_DRAW_TRIANGLES);
                display_mesh->mesh_desc->setAttribArray(VFMT::Position_GUID, &display_mesh->vertex_buffer, 0);
                display_mesh->mesh_desc->setAttribArray(VFMT::Normal_GUID, &display_mesh->normal_buffer, 0);
                display_mesh->mesh_desc->setAttribArray(VFMT::Tangent_GUID, &display_mesh->tangent_buffer, 0);
                display_mesh->mesh_desc->setAttribArray(VFMT::Bitangent_GUID, &display_mesh->bitangent_buffer, 0);
                display_mesh->mesh_desc->setAttribArray(VFMT::ColorRGB_GUID, &display_mesh->color_buffer, 4);
                display_mesh->mesh_desc->setAttribArray(VFMT::UV_GUID, &display_mesh->uv_buffer, 0);
                display_mesh->mesh_desc->setAttribArray(VFMT::UVLightmap_GUID, &display_mesh->uv_lightmap_buffer, 0);
                display_mesh->mesh_desc->setIndexArray(&display_mesh->index_buffer);
                display_mesh->material = mesh->material;
                
                display_mesh->renderable.setMeshDesc(display_mesh->mesh_desc.get());
                if (mesh->material && mesh->material->gpu_material) {
                    display_mesh->renderable.setMaterial(mesh->material->gpu_material.get());
                } else {
                    display_mesh->renderable.setMaterial(default_material.get());
                }
                if (display_mesh->renderable.getMaterial()) {
                    display_mesh->albedo = display_mesh->renderable.getMaterial()->getSampler("texAlbedo");
                } else {
                    display_mesh->albedo = getDefaultTexture("texAlbedo");
                }
                
                display_mesh->renderable_ubuf = gpuGetPipeline()->createUniformBuffer(UNIFORM_BUFFER_MODEL);
                display_mesh->renderable.attachUniformBuffer(display_mesh->renderable_ubuf);
                display_mesh->renderable_ubuf->setMat4(display_mesh->renderable_ubuf->getDesc()->getUniform(UNIFORM_MODEL_TRANSFORM), gfxm::mat4(1.f));
                std::string dbg_name = MKSTR("csg_" << shape->uid << "_" << j);
                display_mesh->renderable.dbg_name = dbg_name;
                display_mesh->renderable.compile();

                meshes.push_back(std::unique_ptr<Mesh>(display_mesh));
            }
        }

        render_instance.render_bucket->clear();
        for (auto& mesh : meshes) {
            render_instance.render_bucket->add(&mesh->renderable);
        }
    }

    void rebuildMeshes() {
        rebuildDisplayMeshes();
    }

    void generateLightmaps() {
        LOG_DBG("Lightmap generation starts...");

        generateLightmapUV();

        HSHARED<gpuShaderProgram> prog = loadShaderProgramForLightmapSampling("shaders/default_lightmap_sample.glsl");
        HSHARED<gpuShaderProgram> prog_uv_wire = loadShaderProgramForLightmapSampling("shaders/uv_wire.glsl");

        lm_context* ctx = lmCreate(
            512,
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
        const int image_width = 512;
        const int image_height = 512;
        const int image_bpp = 3;

        struct LIGHTMAP_DATA {
            std::vector<float> lightmap_data;
            RHSHARED<gpuTexture2d> lightmap;
            std::vector<Mesh*> meshes;

            std::vector<uint32_t> uv_wire_data;
            std::unique_ptr<gpuTexture2d> tex_uv_wire;
            GLuint fbo_wire;
            std::vector<gpuMeshShaderBinding*> wire_bindings;
        };
        std::vector<LIGHTMAP_DATA> lm_groups;

        {
            std::unordered_map<csgMaterial*, std::vector<Mesh*>> by_material;
            for (int i = 0; i < meshes.size(); ++i) {
                auto mesh = meshes[i].get();
                by_material[mesh->material].push_back(mesh);
            }
            for (auto& kv : by_material) {
                auto& group = lm_groups.emplace_back();
                group.meshes = kv.second;
                group.lightmap_data.resize(image_width * image_height * image_bpp);
                memset(group.lightmap_data.data(), 0, sizeof(group.lightmap_data[0]) * image_width * image_height * image_bpp);
                group.lightmap.reset_acquire();
                group.lightmap->setData(group.lightmap_data.data(), image_width, image_height, image_bpp, IMAGE_CHANNEL_FLOAT, false);
                for (int i = 0; i < group.meshes.size(); ++i) {
                    group.meshes[i]->lightmap = group.lightmap;
                    group.meshes[i]->renderable.addSamplerOverride("texLightmap", group.meshes[i]->lightmap);
                    group.meshes[i]->renderable.compile();
                }
                
                group.uv_wire_data.resize(image_width * image_height * image_bpp);
                memset(group.uv_wire_data.data(), 0, sizeof(group.uv_wire_data[0])* image_width* image_height* image_bpp);
            }
        }

        // TODO: Bindings are not freed
        std::vector<gpuMeshShaderBinding*> bindings;
        std::vector<gfxm::mat4> transforms;
        for (int j = 0; j < meshes.size(); ++j) {
            Mesh* mesh = meshes[j].get();
            /*
            mesh->lightmap_data.resize(image_width * image_height * image_bpp);
            memset(mesh->lightmap_data.data(), 0, sizeof(mesh->lightmap_data[0]) * image_width * image_height * image_bpp);
            mesh->lightmap.reset_acquire();
            mesh->lightmap->setData(mesh->lightmap_data.data(), image_width, image_height, image_bpp, IMAGE_CHANNEL_FLOAT, false);
            */
            mesh->mesh_shader_binding = gpuCreateMeshShaderBinding(prog.get(), mesh->mesh_desc.get());

            if (mesh->material && mesh->material->gpu_material.getReferenceName() == "materials/csg/default_hole.mat") {
                continue;
            }
            bindings.push_back(mesh->mesh_shader_binding);
        }

        GL_CHECK(;);

        for (int j = 0; j < lm_groups.size(); ++j) {
            auto& group = lm_groups[j];
            group.tex_uv_wire.reset(new gpuTexture2d(GL_RGBA, image_width, image_height, 4));
            
            glGenFramebuffers(1, &group.fbo_wire);
            glBindFramebuffer(GL_FRAMEBUFFER, group.fbo_wire);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 0, GL_TEXTURE_2D, group.tex_uv_wire->getId(), 0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        for (int j = 0; j < lm_groups.size(); ++j) {
            auto& group = lm_groups[j];

            std::vector<gpuMeshShaderBinding*> wire_bindings;
            for (int k = 0; k < group.meshes.size(); ++k) {
                auto mesh = group.meshes[k];
                auto binding = gpuCreateMeshShaderBinding(prog_uv_wire.get(), mesh->mesh_desc.get());
                wire_bindings.push_back(binding);
            }

            glBindFramebuffer(GL_FRAMEBUFFER, group.fbo_wire);
            GLenum draw_buffers[] = {
                GL_COLOR_ATTACHMENT0 + 0
            };
            glDrawBuffers(1, draw_buffers);
            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

            DRAW_PARAMS params = {
                .view = gfxm::mat4(1.f),
                .projection = gfxm::ortho(.0f, 1.f, .0f, 1.f, .0f, 1.f),
                .viewport_x = 0,
                .viewport_y = 0,
                .viewport_width = image_width,
                .viewport_height = image_height
            };
            glUseProgram(prog_uv_wire->getId());
            gpuDrawUVWires(wire_bindings.data(), wire_bindings.size(), params);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            glDeleteFramebuffers(1, &group.fbo_wire);
            for (auto b : wire_bindings) {
                delete b;
            }
            //Sleep(3000);
            //platformSwapBuffers();
        }

        // Read uv wire pixels
        GLuint fb_read = 0;
        glGenFramebuffers(1, &fb_read);
        for (int j = 0; j < lm_groups.size(); ++j) {
            auto& group = lm_groups[j];
            //glActiveTexture(GL_TEXTURE0 + 0);
            //glBindTexture(GL_TEXTURE_2D, group.tex_uv_wire->getId());
            glBindFramebuffer(GL_FRAMEBUFFER, fb_read);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, group.tex_uv_wire->getId(), 0);
            //glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, image_width, image_height);
            glReadPixels(0, 0, image_width, image_height, GL_RGBA, GL_UNSIGNED_BYTE, group.uv_wire_data.data());
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            GL_CHECK(;);
        }
        glDeleteFramebuffers(1, &fb_read);

        for (int i = 0; i < bounce_count; ++i) {
            for (int j = 0; j < lm_groups.size(); ++j) {
                auto& group = lm_groups[j];

                memset(group.lightmap_data.data(), 0, sizeof(group.lightmap_data[0]) * image_width * image_height * image_bpp);
                for (int k = 0; k < group.meshes.size(); ++k) {
                    Mesh* mesh = group.meshes[k];

                    gfxm::mat4 transform = gfxm::mat4(1.f);

                    lmSetTargetLightmap(ctx, group.lightmap_data.data(), image_width, image_width, image_bpp);
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
                        /*
                        glActiveTexture(GL_TEXTURE0 + 0);
                        glBindTexture(GL_TEXTURE_2D, mesh->albedo);*/
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
                    LOG("Lightmapping bounce: " << i + 1 << "/" << bounce_count
                        << ", map: " << j + 1 << "/" << lm_groups.size()
                        << ", mesh: " << k + 1 << "/" << group.meshes.size()
                    );
                }
            }

            std::vector<float> temp(image_width* image_height* image_bpp);
            for (int i = 0; i < lm_groups.size(); ++i) {
                auto& group = lm_groups[i];

                lmImageDilate(group.lightmap_data.data(), temp.data(), image_width, image_height, image_bpp);
                lmImageDilate(temp.data(), group.lightmap_data.data(), image_width, image_height, image_bpp);

                group.lightmap->setData(group.lightmap_data.data(), image_width, image_height, image_bpp, IMAGE_CHANNEL_FLOAT, false);
                group.lightmap->setFilter(GPU_TEXTURE_FILTER_NEAREST);
                //group.lightmap->generateMipmaps();
            }
            /*
            for (int i = 0; i < meshes.size(); ++i) {
                Mesh* mesh = meshes[i].get();

                lmImageDilate(mesh->lightmap_data.data(), temp.data(), image_width, image_height, image_bpp);
                lmImageDilate(temp.data(), mesh->lightmap_data.data(), image_width, image_height, image_bpp);

                mesh->lightmap.reset_acquire();
                mesh->lightmap->setData(mesh->lightmap_data.data(), image_width, image_height, image_bpp, IMAGE_CHANNEL_FLOAT, false);

                mesh->renderable.addSamplerOverride("texLightmap", mesh->lightmap);
                mesh->renderable.compile();
            }*/
        }
        glUseProgram(0);

        lmDestroy(ctx);

        // gamma correct and save lightmaps to disk
        for (int i = 0; i < lm_groups.size(); ++i) {
            auto& group = lm_groups[i];
            //lmImagePower(group.lightmap_data.data(), image_width, image_height, image_bpp, 1.0f / 2.2f);
            lmImageSaveTGAf(MKSTR("lightmap_test/lm" << i << ".tga").c_str(), group.lightmap_data.data(), image_width, image_height, image_bpp);
            lmImageSaveTGAub(MKSTR("lightmap_test/uv" << i << ".tga").c_str(), (unsigned char*)group.uv_wire_data.data(), image_width, image_height, 4);
        }
        /*
        for (int i = 0; i < meshes.size(); i++) {
            Mesh* mesh = meshes[i].get();
            lmImagePower(mesh->lightmap_data.data(), image_width, image_height, image_bpp, 1.0f / 2.2f);
            lmImageSaveTGAf(MKSTR("lightmap_test/lm" << i << ".tga").c_str(), mesh->lightmap_data.data(), image_width, image_height, image_bpp);
        }*/

        LOG_DBG("Lightmap generation done.");
    }

    void buildSkeletalModel() {
        auto material_ = resGet<gpuMaterial>("materials/csg/csg_default.mat");

        {
            model.reset_acquire();
            skeleton.reset_acquire();
            model->setSkeleton(skeleton);
        }

        std::map<csgMaterial*, std::vector<csgMeshData*>> by_material;
        for (int i = 0; i < csg_scene.shapeCount(); ++i) {
            csgBrushShape* shape = csg_scene.getShape(i);
            for (int j = 0; j < shape->triangulated_meshes.size(); ++j) {
                csgMeshData* mesh = shape->triangulated_meshes[j].get();
                by_material[mesh->material].push_back(mesh);
            }
        }

        for (auto& kv : by_material) {
            auto& meshes = kv.second;
            csgMaterial* material = kv.first;

            std::vector<gfxm::vec3> vertices;
            std::vector<gfxm::vec3> normals;
            std::vector<gfxm::vec3> tangents;
            std::vector<gfxm::vec3> bitangents;
            std::vector<uint32_t>   colors;
            std::vector<gfxm::vec2> uvs;
            std::vector<gfxm::vec2> uvs_lightmap;
            std::vector<uint32_t>   indices;

            int base_index = 0;
            for (int i = 0; i < meshes.size(); ++i) {
                vertices.insert(vertices.end(), meshes[i]->vertices.begin(), meshes[i]->vertices.end());
                normals.insert(normals.end(), meshes[i]->normals.begin(), meshes[i]->normals.end());
                tangents.insert(tangents.end(), meshes[i]->tangents.begin(), meshes[i]->tangents.end());
                bitangents.insert(bitangents.end(), meshes[i]->bitangents.begin(), meshes[i]->bitangents.end());
                colors.insert(colors.end(), meshes[i]->colors.begin(), meshes[i]->colors.end());
                uvs.insert(uvs.end(), meshes[i]->uvs.begin(), meshes[i]->uvs.end());
                uvs_lightmap.insert(uvs_lightmap.end(), meshes[i]->uvs_lightmap.begin(), meshes[i]->uvs_lightmap.end());

                for (int j = 0; j < meshes[i]->indices.size(); ++j) {
                    indices.push_back(base_index + meshes[i]->indices[j]);
                }

                base_index = vertices.size();
            }

            Mesh3d cpu_mesh;
            cpu_mesh.clear();
            cpu_mesh.setAttribArray(VFMT::Position_GUID, vertices.data(), vertices.size() * sizeof(vertices[0]));
            cpu_mesh.setAttribArray(VFMT::Normal_GUID, normals.data(), normals.size() * sizeof(normals[0]));
            cpu_mesh.setAttribArray(VFMT::Tangent_GUID, tangents.data(), tangents.size() * sizeof(tangents[0]));
            cpu_mesh.setAttribArray(VFMT::Bitangent_GUID, bitangents.data(), bitangents.size() * sizeof(bitangents[0]));
            cpu_mesh.setAttribArray(VFMT::ColorRGB_GUID, colors.data(), colors.size() * sizeof(colors[0]));
            cpu_mesh.setAttribArray(VFMT::UV_GUID, uvs.data(), uvs.size() * sizeof(uvs[0]));
            cpu_mesh.setAttribArray(VFMT::UVLightmap_GUID, uvs_lightmap.data(), uvs_lightmap.size() * sizeof(uvs_lightmap[0]));
            cpu_mesh.setIndexArray(indices.data(), indices.size() * sizeof(indices[0]));

            RHSHARED<gpuMesh> gpu_mesh;
            gpu_mesh.reset_acquire();
            gpu_mesh->setData(&cpu_mesh);
            gpu_mesh->setDrawMode(MESH_DRAW_MODE::MESH_DRAW_TRIANGLES);

            auto mesh_component = model->addComponent<sklmMeshComponent>(MKSTR("csg_" << (int)kv.first).c_str());
            mesh_component->bone_name = "Root";
            if (material && material->gpu_material) {
                mesh_component->material = material->gpu_material;
            } else {
                mesh_component->material = material_;
            }
            mesh_component->mesh = gpu_mesh;
        }

        /*
        for (int i = 0; i < csg_scene.shapeCount(); ++i) {
            csgBrushShape* shape = csg_scene.getShape(i);
            for (int j = 0; j < shape->triangulated_meshes.size(); ++j) {
                csgMeshData* mesh = shape->triangulated_meshes[j].get();

                Mesh3d cpu_mesh;
                cpu_mesh.clear();
                cpu_mesh.setAttribArray(VFMT::Position_GUID, mesh->vertices.data(), mesh->vertices.size() * sizeof(mesh->vertices[0]));
                cpu_mesh.setAttribArray(VFMT::Normal_GUID, mesh->normals.data(), mesh->normals.size() * sizeof(mesh->normals[0]));
                cpu_mesh.setAttribArray(VFMT::Tangent_GUID, mesh->tangents.data(), mesh->tangents.size() * sizeof(mesh->tangents[0]));
                cpu_mesh.setAttribArray(VFMT::Bitangent_GUID, mesh->bitangents.data(), mesh->bitangents.size() * sizeof(mesh->bitangents[0]));
                cpu_mesh.setAttribArray(VFMT::ColorRGB_GUID, mesh->colors.data(), mesh->colors.size() * sizeof(mesh->colors[0]));
                cpu_mesh.setAttribArray(VFMT::UV_GUID, mesh->uvs.data(), mesh->uvs.size() * sizeof(mesh->uvs[0]));
                cpu_mesh.setAttribArray(VFMT::UVLightmap_GUID, mesh->uvs_lightmap.data(), mesh->uvs_lightmap.size() * sizeof(mesh->uvs_lightmap[0]));
                cpu_mesh.setIndexArray(mesh->indices.data(), mesh->indices.size() * sizeof(mesh->indices[0]));

                RHSHARED<gpuMesh> gpu_mesh;
                gpu_mesh.reset_acquire();
                gpu_mesh->setData(&cpu_mesh);
                gpu_mesh->setDrawMode(MESH_DRAW_MODE::MESH_DRAW_TRIANGLES);
                

                auto mesh_component = model->addComponent<sklmMeshComponent>(MKSTR("csg" << i << "_" << j).c_str());
                mesh_component->bone_name = "Root";
                if (mesh->material && mesh->material->gpu_material) {
                    mesh_component->material = mesh->material->gpu_material;
                } else {
                    mesh_component->material = material_;
                }
                mesh_component->mesh = gpu_mesh;
            }
        }*/
    }

    void buildCollisionData() {
        uint32_t merged_base_index = 0;

        std::vector<gfxm::vec3> merged_vertices;
        std::vector<uint32_t> merged_indices;

        for (int i = 0; i < csg_scene.shapeCount(); ++i) {
            csgBrushShape* shape = csg_scene.getShape(i);
            for (int j = 0; j < shape->triangulated_meshes.size(); ++j) {
                csgMeshData* mesh = shape->triangulated_meshes[j].get();

                merged_vertices.insert(merged_vertices.end(), mesh->vertices.begin(), mesh->vertices.end());
                for (auto& idx : mesh->indices) {
                    merged_indices.push_back(idx + merged_base_index);
                }
                merged_base_index = merged_vertices.size();
            }
        }

        collision_mesh.setData(merged_vertices.data(), merged_vertices.size(), merged_indices.data(), merged_indices.size());
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::KEYDOWN:
            switch (params.getA<uint16_t>()) {
            case 0x43: // C key
                viewport.clearTools();
                viewport.addTool(&tool_create_box);
                return true;
            case 0x4E: // N
                viewport.clearTools();
                viewport.addTool(&tool_create_custom_shape);
                return true;
            case 0x56: // V - cut
                if (!selected_objects.empty()) {
                    csgBrushShape* shape = dynamic_cast<csgBrushShape*>(selected_objects.back());
                    if (shape) {
                        viewport.clearTools();
                        viewport.addTool(&tool_cut);
                        tool_cut.setData(shape);
                    }
                }
                return true;
            case 0x55: // U - edit texture coordinates
                if (!selected_objects.empty()) {
                    csgBrushShape* shape = dynamic_cast<csgBrushShape*>(selected_objects.back());
                    if (shape) {
                        viewport.clearTools();
                        viewport.addTool(&tool_uv_edit);
                        // TODO:
                        //tool_uv_edit.setData(selected_shapes.back());
                    }
                }
                return true;
            case 0x31: // 1
                viewport.clearTools();
                viewport.addTool(&tool_object_mode);
                return true;
            case 0x32: { // 2
                if (!tool_object_mode.selected_objects.empty()) {
                    csgBrushShape* shape = dynamic_cast<csgBrushShape*>(tool_object_mode.selected_objects.back());
                    if (shape) {
                        viewport.clearTools();
                        viewport.addTool(&tool_face_mode);
                        tool_face_mode.setShapeData(&csg_scene, shape);
                    }
                }
                return true;
            }
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
                receiveDragDrop();
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
                selected_objects.clear();
                selected_objects.push_back(params.getB<csgObject*>());
                return true;
            case GUI_NOTIFY::CSG_SHAPE_CREATED: {
                auto ptr = params.getB<csgObject*>();

                selected_objects.push_back(ptr);
                tool_object_mode.selectObject(ptr, true);

                csgBrushShape* shape = dynamic_cast<csgBrushShape*>(ptr);
                if(shape) {
                    //shapes.push_back(std::unique_ptr<csgBrushShape>(ptr));
                    for (int i = 0; i < shape->faces.size(); ++i) {
                        auto& face = shape->faces[i];
                        gfxm::vec3& N = face->N;
                        if (fabsf(gfxm::dot(gfxm::vec3(0, 1, 0), N)) < .707f) {
                            face->material = mat_wall_def;
                        } else {
                            face->material = mat_floor_def;
                        }
                    }
                    csg_scene.addShape(shape);
                }
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
                auto obj = params.getB<csgObject*>();
                assert(obj);
                csg_scene.removeObject(obj);
                selected_objects.clear();
                csg_scene.update();
                rebuildMeshes();/*
                auto shape = dynamic_cast<csgBrushShape*>(obj);
                if (shape) {
                    csg_scene.removeShape(shape);
                    selected_objects.clear();
                    csg_scene.update();
                    rebuildMeshes();
                }*/
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
                //viewport.pivot_reset_point = hit;
            } else if(gfxm::intersect_line_plane_point(R.origin, R.direction, gfxm::vec3(0, 1, 0), .0f, hit)) {
                //viewport.pivot_reset_point = hit;
            }            
        }

        render_instance.render_bucket->clear();
        for (auto& mesh : meshes) {
            render_instance.render_bucket->add(&mesh->renderable);
        }
        for (auto& ri : ref_images) {
            render_instance.render_bucket->add(&ri->renderable);
        }
    }
    void onDraw() override {
        GuiWindow::onDraw();
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
            csg_viewport.buildSkeletalModel();

            nlohmann::json j;
            type_get<mdlSkeletalModelMaster>().serialize_json(j, csg_viewport.model.get());
            std::ofstream f(path + ".skeletal_model");
            if (!f.is_open()) {
                return false;
            }
            f << j.dump(4);
        }
        {
            csg_viewport.buildCollisionData();

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
