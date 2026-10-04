#include "decoration_mode.hpp"

#include "../terrain_space.hpp"


TerrainDecorationMode::TerrainDecorationMode(TerrainSceneSpace* space)
    : TerrainEditMode("Decoration Mode", space)
{
    subscribe([this](const GuiEvt_MouseMove& e) {
        e.consume = false;
        guiScheduleTick(this, 0, GUI_TICK_CUSTOM);
    });
    subscribe([this](const GuiEvt_KeyDown& e) {
        switch (e.vkey) {
        case 90: // Z key
            if(cursor_hit) {
                viewport->setCameraPivot(cursor_pos);
            }
            return;
        }
        e.consume = false;
        e.invoke_next();
    });
    subscribe([this](const GuiEvt_MouseBtn& e) {
        if (e.btn == GUI_MOUSE_LEFT) {
            if (e.state == GUI_KEY_DOWN) {
                if (!cursor_hit) {
                    e.consume = false;
                    return;
                }

                state = State::Angle;
                placement_pos = cursor_pos;
            } else {
                if (state != State::Angle) {
                    return;
                }
                state = State::Hover;

                auto cell = getSpace()->getCellWorldSpace(placement_pos.x, placement_pos.z);
                if (!cell) {
                    return;
                }

                TerrainDecoration* deco = nullptr;
                for (int i = 0; i < cell->decorations.size(); ++i) {
                    TerrainDecoration* d = &cell->decorations[i];
                    if (d->model == model) {
                        deco = d;
                        break;
                    }
                }

                if (!deco) {
                    auto& d = cell->decorations.emplace_back();
                    d.model = model;
                    deco = &d;

                    {
                        deco->inst_desc.setInstanceCount(0);

                        for (int i = 0; i < deco->model->mesh_instances.size(); ++i) {
                            const auto& m3d_inst = deco->model->mesh_instances[i];
                            const auto& m3d_mesh = deco->model->meshes[m3d_inst.mesh_idx];

                            ResourceRef<gpuMaterial> material = deco->model->materials[m3d_mesh.material_idx];
                            const gpuMeshDesc* mesh_desc = m3d_mesh.mesh->getMeshDesc();

                            auto& transform_block = deco->transform_blocks.emplace_back();
                            transform_block = gpuGetDevice()->createParamBlock<gpuTransformBlock>();

                            auto& renderable = deco->renderables.emplace_back();
                            renderable = std::move(gpuRenderable(material.get(), mesh_desc, &deco->inst_desc, "decoration"));
                            renderable.attachParamBlock(transform_block);
                            renderable.compile();
                        }
                    }
                }

                deco->instances.push_back(gpuDefaultInstancingDesc::Instance{
                    .pos = gfxm::vec4(placement_pos, 1),
                    .rot = gfxm::angle_axis(placement_angle, gfxm::vec3(0, 1, 0))
                });
                LOG_DBG("angle: " << placement_angle);

                getSpace()->markCellDirtyWorldSpace(placement_pos.x, placement_pos.z);
            }
            return;
        }

        e.consume = false;
    });

    model = loadResource<m3dModel>("models/grass/grass");
    active_preview.renderables.clear();
    for (int i = 0; i < model->mesh_instances.size(); ++i) {
        const auto& m3d_inst = model->mesh_instances[i];
        const auto& m3d_mesh = model->meshes[m3d_inst.mesh_idx];

        ResourceRef<gpuMaterial> material = model->materials[m3d_mesh.material_idx];
        const gpuMeshDesc* mesh_desc = m3d_mesh.mesh->getMeshDesc();

        auto& transform_block = active_preview.transform_blocks.emplace_back();
        transform_block = gpuGetDevice()->createParamBlock<gpuTransformBlock>();

        auto& renderable = active_preview.renderables.emplace_back();
        renderable = std::move(gpuRenderable(material.get(), mesh_desc, nullptr, "preview"));
        renderable.attachParamBlock(transform_block);
        renderable.compile();
    }
}

void TerrainDecorationMode::onTick(float dt, GUI_TICK_ID id) {
    if (id != GUI_TICK_CUSTOM) {
        return;
    }

    switch (state) {
    case State::Hover:
        cursor_hit = getSpace()->hitTest(cursor_pos);
        placement_pos = cursor_pos;
        break;
    case State::Angle: {
        gfxm::vec3 ptref;
        gfxm::ray ray = viewport->makeRayFromMousePos();
        if (gfxm::intersect_ray_plane_point(ray.origin, ray.origin + ray.direction * ray.length, gfxm::vec3(0, 1, 0), placement_pos.y, ptref)) {
            gfxm::vec3 B = gfxm::normalize(ptref - placement_pos);
            gfxm::vec3 A = gfxm::vec3(1, 0, 0);
            float d = gfxm::dot(A, B);
            float dref = gfxm::dot(gfxm::vec3(0, 0, 1), B);
            placement_angle = dref < .0f ? acosf(d) : gfxm::pi2 - acosf(d);
        }
        break;
    }
    }

    for (int i = 0; i < active_preview.renderables.size(); ++i) {
        auto& rdr = active_preview.renderables[i];

        gfxm::mat4 t = gfxm::translate(gfxm::mat4(1.f), placement_pos)
            * gfxm::to_mat4(gfxm::angle_axis(placement_angle, gfxm::vec3(0, 1, 0)));
        active_preview.transform_blocks[i]->setTransform(t);
    }
}