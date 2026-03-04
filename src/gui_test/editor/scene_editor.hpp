#pragma once

#include "editor_window.hpp"
#include "gui/elements/viewport/gui_viewport.hpp"


class GuiSceneDocument : public GuiEditorWindow {
    gpuRenderBucket render_bucket;
    gpuRenderTarget render_target;
    GameRenderInstance render_instance;

    gpuMesh mesh;
    std::unique_ptr<gpuGeometryRenderable> renderable;
    RHSHARED<gpuMaterial> material;

    gpuMesh mesh2;
    std::unique_ptr<gpuGeometryRenderable> renderable2;
    RHSHARED<gpuMaterial> material2;
public:
    GuiViewport viewport;

    GuiSceneDocument()
        : GuiEditorWindow("SceneDocument", "scene"),
        render_bucket(gpuGetPipeline(), 1000),
        render_target(800, 600) {
        
        gpuGetPipeline()->initRenderTarget(&render_target);
        render_instance.render_bucket = &render_bucket;
        render_instance.render_target = &render_target;
        render_instance.view_transform = gfxm::mat4(1.f);
        game_render_instances.insert(&render_instance);
        viewport.render_instance = &render_instance;

        addChild(&viewport);
        viewport.setOwner(this);

        {
            Mesh3d mesh_ram;
            //meshGenerateVoxelField(&mesh_ram, 0, 0, 0);
            //meshGenerateCube(&mesh_ram);
            //meshGenerateCheckerPlane(&mesh_ram);
            meshGenerateGrid(&mesh_ram, 160, 160, 160);
            mesh.setData(&mesh_ram);
            mesh.setDrawMode(MESH_DRAW_MODE::MESH_DRAW_LINES);
            material = resGet<gpuMaterial>("core/materials/gizmo_grid.mat");
            //material = resGet<gpuMaterial>("materials/default3.mat");
            renderable.reset(new gpuGeometryRenderable(material.get(), mesh.getMeshDesc(), 0, "MyCube"));
            renderable->setTransform(gfxm::mat4(1.f));
        }

        {
            Mesh3d mesh_ram;
            //meshGenerateVoxelField(&mesh_ram, 0, 0, 0);
            meshGenerateCube(&mesh_ram);
            //meshGenerateCheckerPlane(&mesh_ram);
            mesh2.setData(&mesh_ram);
            mesh2.setDrawMode(MESH_DRAW_MODE::MESH_DRAW_TRIANGLES);
            material2 = resGet<gpuMaterial>("materials/default3.mat");
            renderable2.reset(new gpuGeometryRenderable(material2.get(), mesh2.getMeshDesc(), 0, "MyCube"));
            renderable2->setTransform(gfxm::mat4(1.f));
        }
    }

    void onDraw() override {
        render_bucket.add(renderable.get());
        render_bucket.add(renderable2.get());
        viewport.render_instance->world.getRenderScene()->draw(&render_bucket);

        static float time = .0f;
        time += .01f;
        renderable2->setVec4("color", gfxm::vec4(gfxm::hsv2rgb(sinf(time * (1.f/7.f)), 1.f, 1.f), 1.f));

        GuiEditorWindow::onDraw();
    }
    bool onSaveCommand(const std::string& path) override {
        LOG_DBG("onSaveCommand");
        return true;
    }

    bool onOpenCommand(const std::string& path) override {
        LOG_DBG("onOpenCommand");
        return true;
    }
};