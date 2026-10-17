#pragma once

#include "editor_window.hpp"
#include "gui/elements/viewport/gui_viewport.hpp"
#include "gui/elements/viewport/tools/gui_viewport_tool_transform.hpp"
#include "gui/elements/dock_space.hpp"
#include "gui_engine/actor_inspector.hpp"

#include "scene_editor/terrain/terrain_space.hpp"

#include "scene_editor/scene_data.hpp"


struct GuiEvt_SpaceSelected : public GuiEvent {
    GuiEvt_SpaceSelected(SceneSpace* s) : space(s) {}
    SceneSpace* space = nullptr;
};
struct GuiEvt_EntrySelected : public GuiEvent {
    GuiEvt_EntrySelected(SceneEntry_OLD* e) : entry(e) {}
    SceneEntry_OLD* entry = nullptr;
};

class GuiSceneOutliner : public GuiElement {
    SceneData* scene_data = nullptr;

    GuiTreeView* list = nullptr;
    std::vector<GuiTreeItem*> items; // TODO: Should just access items through list
public:
    GuiSceneOutliner(SceneData* scn)
    : scene_data(scn) {
        setSize(200, 400);
        list = guiCreate<GuiTreeView>();
        list->setSize(gui::fill(), gui::fill());
        pushBack(list);
        updateView();
    }

    void setSelected(SceneEntry_OLD* e) {
        for (int i = 0; i < items.size(); ++i) {
            auto item = items[i];
            if (item->user_ptr != e) {
                continue;
            }
            item->invokeBubble(GuiEvt_Selected{item});
            list->scrollTo(item);
        }
    }

    void updateView() {
        list->clearChildren();
        items.clear();
        
        {
            GuiTreeItem* item = list->addItem("Terrain");
            item->subscribe([this](const GuiEvt_Selected& e) {
                e.invoke_next();
                invoke(GuiEvt_SpaceSelected(scene_data->scene_space.get()));
            });
        }
        
        GuiTreeItem* item_entries = list->addItem("Actors");
        item_entries->setCollapsed(false);

        auto& entries = scene_data->entries;
        for (int i = 0; i < entries.size(); ++i) {
            SceneEntry_OLD& entry = entries[i];
            Actor* actor = entry.instance.get();
            GuiTreeItem* item = item_entries->addItem(entry.instance->getName().c_str());
            item->user_ptr = &entry;
            item->subscribe([this](const GuiEvt_Selected& e) {
                e.invoke_next();
                invoke(GuiEvt_EntrySelected(static_cast<SceneEntry_OLD*>(e.elem->user_ptr)));
            });
            items.push_back(item);
        }
    }
};

class GuiSceneEditor
    : public GuiEditorWindow
    , public gpuSceneQueryInterface 
{
    enum class Mode {
        None,
        Space,
        Actor
    } mode = Mode::None;

    SceneEditorContext context;

    SceneData scene_data;

    // ===========================================
    GuiActorInspector* actor_inspector = nullptr;
    GuiSceneOutliner* scene_outliner = nullptr;

    RuntimeWorld world;

    gpuMesh mesh;
    std::unique_ptr<gpuGeometryRenderable> renderable;

    gpuMesh mesh2;
    std::unique_ptr<gpuGeometryRenderable> renderable2;
    ResourceRef<gpuMaterial> material2;

    SceneEntry_OLD* selected_entry = nullptr;

    void enableTransformTool() {
        if (!selected_entry) {
            return;
        }
        viewport.setTool(&tool_transform);
        tool_transform.translation = selected_entry->instance->getTranslation();
        tool_transform.rotation = selected_entry->instance->getRotation();
    }

    void selectEntry(SceneEntry_OLD* e) {
        selected_entry = e;
        actor_inspector->init(e->instance.get());
        scene_outliner->setSelected(e);
    }

    void clearMode() {
        if (mode == Mode::Actor) {
            selected_entry = nullptr;
            actor_inspector->clearChildren();
            viewport.clearTool();
        } else if (mode == Mode::Space) {
            if (scene_data.scene_space) {
                scene_data.scene_space->exitUi(context);
            }
            viewport.clearTool();
        }
        mode = Mode::None;
    }
    void enterSpaceMode() {
        clearMode();
        mode = Mode::Space;
        viewport.setTool(scene_data.scene_space.get());
        scene_data.scene_space->enterUi(context);
    }
    void enterActorMode(SceneEntry_OLD* entry) {
        clearMode();
        mode = Mode::Actor;
        selected_entry = entry;
        actor_inspector->init(entry->instance.get());
        enableTransformTool();
    }
public:
    GuiDockSpace dock_space;
    GuiViewport viewport;
    GuiViewportToolTransform tool_transform;

    GuiSceneEditor()
        : GuiEditorWindow("GenericScene", "scene")
    {
        actor_inspector = guiCreate<GuiActorInspector>();
        actor_inspector->subscribe([this](const GuiEvt_PropChanged&) {
            tool_transform.translation = selected_entry->instance->getTranslation();
            tool_transform.rotation = selected_entry->instance->getRotation();
        });

        scene_outliner = guiCreate<GuiSceneOutliner>(&scene_data);
        scene_outliner->subscribe([this](const GuiEvt_SpaceSelected& e) {
            enterSpaceMode();
        });
        scene_outliner->subscribe([this](const GuiEvt_EntrySelected& e) {
            enterActorMode(e.entry);
        });
        
        tool_transform.subscribe([this](const GuiEvt_GizmoTranslate& e) {
            if(!selected_entry) return;
            selected_entry->instance->translate(e.delta);
        });
        tool_transform.subscribe([this](const GuiEvt_GizmoRotate& e) {
            if(!selected_entry) return;
            selected_entry->instance->rotate(e.delta);
        });

        viewport.subscribe([this](const GuiEvt_RClick&) {
            auto menu = guiCreate<GuiMenuList>();
            guiAddTransientPopup(nullptr, menu, guiGetMousePos());
            auto item_create = menu->addItem("Create Actor...", 0);
            item_create->subscribe([this, menu](const GuiEvt_LClick&) {
                auto& entry = scene_data.entries.emplace_back();
                entry.instance.reset(new Actor);
                entry.instance->setName("Actor");
                world.spawn(entry.instance.get());
                scene_outliner->updateView();
                selectEntry(&entry);
                guiRemoveTransientPopup(menu);
            });
            auto item_focus = menu->addItem("Focus selected", 0);
            item_focus->subscribe([this, menu](const GuiEvt_LClick&) {
                // TODO:
                guiRemoveTransientPopup(menu);
            });
            auto item_test = menu->addItem("_test out of line rendering", 0);
            item_test->subscribe([this, menu](const GuiEvt_LClick&) {
                EngineRenderView rv(gfxm::rect(0, 0, 1, 1), RendererType::Default, true);
                rv.addQueryInterface(world.getSystem<SceneSystem>());
                rv.addQueryInterface(world.getSystem<scnRenderScene>());
                rv.addQueryInterface(&scene_data);
                rv.setFov(90.f);
                rv.setZNear(.01f);
                rv.setZFar(1000.f);
                const int SIDE = 64;
                rv.getRenderTarget()->setSize(SIDE, SIDE);

                gfxm::vec3 eye = gfxm::vec3(1, 1, 3);
                gfxm::mat4 views[6] = {
                    gfxm::lookAt(eye, eye + gfxm::vec3( 1.f,  .0f,  .0f), gfxm::vec3(.0f, -1.f,  .0f)),
                    gfxm::lookAt(eye, eye + gfxm::vec3(-1.f,  .0f,  .0f), gfxm::vec3(.0f, -1.f,  .0f)),
                    gfxm::lookAt(eye, eye + gfxm::vec3( 0.f,  1.f,  .0f), gfxm::vec3(.0f,  .0f,  1.f)),
                    gfxm::lookAt(eye, eye + gfxm::vec3( 0.f, -1.f,  .0f), gfxm::vec3(.0f,  .0f, -1.f)),
                    gfxm::lookAt(eye, eye + gfxm::vec3( 0.f,  .0f,  1.f), gfxm::vec3(.0f, -1.f,  .0f)),
                    gfxm::lookAt(eye, eye + gfxm::vec3( 0.f,  .0f, -1.f), gfxm::vec3(.0f, -1.f,  .0f)),
                };
                struct Face { gfxm::vec3 forward, up; };
                const Face faces[6] = {
                    {{ 1, 0, 0}, {0,-1, 0}}, {{-1, 0, 0}, {0,-1, 0}},
                    {{ 0, 1, 0}, {0, 0, 1}}, {{ 0,-1, 0}, {0, 0,-1}},
                    {{ 0, 0, 1}, {0,-1, 0}}, {{ 0, 0,-1}, {0,-1, 0}},
                };
                std::string sides[6] = { "+x", "-x", "+y", "-y", "+z", "-z" };
                
                auto sh_basis = [](const gfxm::vec3& d, float* out) {
                    out[0] = 0.282095f;
                    out[1] = 0.488603f * d.y;
                    out[2] = 0.488603f * d.z;
                    out[3] = 0.488603f * d.x;
                    out[4] = 1.092548f * d.x * d.y;
                    out[5] = 1.092548f * d.y * d.z;
                    out[6] = 0.315392f * (3.0f * d.z * d.z - 1.0f);
                    out[7] = 1.092548f * d.x * d.z;
                    out[8] = 0.546274f * (d.x * d.x - d.y * d.y);
                };
                gfxm::vec3 sh[9] = { 0 };
                for(int i = 0; i < 6; ++i) {
                    rv.setView(views[i]);
                    gpuGetPipeline()->drawSingleView(&rv, .0f);
                    gpuTexture2d* tex = rv.getRenderTarget()->getTexture("Final");
                    ktImage img;
                    tex->getData(&img);
                    /*if (i == 0) {
                        std::vector<gfxm::vec3> tex(SIDE * SIDE);
                        std::fill(tex.begin(), tex.end(), gfxm::vec3(1, 0, 0));
                        img.setData(tex.data(), SIDE, SIDE, 3, IMAGE_CHANNEL_FLOAT);
                    }*/
                    writeImagePng(std::format("__test{}.png", sides[i]), &img);

                    for (int y = 0; y < SIDE; ++y) {
                        float v = 2.f * (y + .5f) / SIDE - 1.f;
                        for (int x = 0; x < SIDE; ++x) {
                            float u = 2.f * (x + .5f) / SIDE - 1.f;
                            
                            gfxm::vec3 right = gfxm::cross(faces[i].forward, faces[i].up);
                            gfxm::vec3 d = gfxm::normalize(faces[i].forward + u * right + v * faces[i].up);

                            float dO = (4.f / (SIDE * SIDE)) / powf(1.f + u*u + v*v, 1.5f);

                            auto s = img.samplef((x + .5f) / SIDE, (y + .5f) / SIDE);
                            //s.x = powf(s.x, 2.2f);
                            //s.y = powf(s.y, 2.2f);
                            //s.z = powf(s.z, 2.2f);

                            float Y[9];
                            sh_basis(d, Y);
                            for (int j = 0; j < 9; ++j) {
                                sh[j].x += s.x * Y[j] * dO;
                                sh[j].y += s.y * Y[j] * dO;
                                sh[j].z += s.z * Y[j] * dO;
                            }
                        }
                    }
                }
                printf("\n{\n");
                for (int k = 0; k < 9; ++k) {
                    printf("\tvec3( %f, %f, %f ),\n", sh[k].x, sh[k].y, sh[k].z);
                }
                printf("}\n");
                ktImage img;
                img.setData(sh, 3, 3, 3, IMAGE_CHANNEL_FLOAT);
                writeImagePng("__SH.png", &img);

                for(int i = 0; i < 6; ++i) {
                    std::vector<gfxm::vec3> pixels(SIDE * SIDE);
                    for (int y = 0; y < SIDE; ++y) {
                        float v = 2.f * (y + .5f) / SIDE - 1.f;
                        for (int x = 0; x < SIDE; ++x) {
                            float u = 2.f * (x + .5f) / SIDE - 1.f;

                            gfxm::vec3 right = gfxm::cross(faces[i].forward, faces[i].up);
                            gfxm::vec3 d = gfxm::normalize(faces[i].forward + u * right + v * faces[i].up);

                            float Y[9];
                            sh_basis(d, Y);
                            for (int j = 0; j < 9; ++j) {
                                pixels[x + y * SIDE] += sh[j] * Y[j];
                            }
                        }
                    }

                    ktImage img;
                    img.setData(pixels.data(), SIDE, SIDE, 3, IMAGE_CHANNEL_FLOAT);
                    writeImagePng(std::format("__recon{}.png", sides[i]), &img);
                }

                guiRemoveTransientPopup(menu);
            });
        });

        viewport.getRenderView()->setView(gfxm::mat4(1.f));
        viewport.getRenderView()->setZFar(4000.f);
        viewport.getRenderView()->setZNear(.2f);
        viewport.getRenderView()->addQueryInterface(this);

        pushBack(&dock_space);
        auto node = dock_space.getRoot();
        node->setMode(GUI_DOCK_NODE_SINGLE);
        node->setLocked(true);
        node->addWindow(&viewport);
        node = node->splitLeft();
        node->split_pos = .20f;
        node->left->addWindow(actor_inspector);
        node = node->splitRight();
        node->split_pos = .85f;
        node->right->addWindow(scene_outliner);
        //node->addWindow(actor_inspector);
        

        viewport.setOwner(this);

        {
            Mesh3d mesh_ram;
            meshGenerateGrid(&mesh_ram, 160, 160, 160);
            mesh.setData(&mesh_ram);
            mesh.setDrawMode(MESH_DRAW_MODE::MESH_DRAW_LINES);
            mesh.setType(GPU_MESH_DESC_TYPE::LINE);
            renderable.reset(new gpuGeometryRenderable(nullptr, mesh.getMeshDesc(), 0, "Grid"));
            renderable->setTransform(gfxm::mat4(1.f));
        }

        {
            Mesh3d mesh_ram;
            meshGenerateCube(&mesh_ram);
            mesh2.setData(&mesh_ram);
            mesh2.setType(GPU_MESH_DESC_TYPE::GENERIC);
            mesh2.setDrawMode(MESH_DRAW_MODE::MESH_DRAW_TRIANGLES);
            material2 = loadResource<gpuMaterial>("materials/default3");
            renderable2.reset(new gpuGeometryRenderable(material2.get(), mesh2.getMeshDesc(), 0, "MyCube"));
            renderable2->setTransform(gfxm::mat4(1.f));
        }

        guiScheduleTick(this, 1.f / 30.f, GUI_TICK_CUSTOM);
    }

    void onTick(float dt, GUI_TICK_ID id) override {
        if (id != GUI_TICK_CUSTOM) {
            return;
        }
        world.update(dt);
        guiScheduleTick(this, 1.f / 30.f, GUI_TICK_CUSTOM);
    }

    void queryGeometry(const GeometryQuery& q) override {
        world.getSystem<SceneSystem>()->queryGeometry(q);
        world.getSystem<scnRenderScene>()->queryGeometry(q);
        scene_data.queryGeometry(q);
    }

    bool onSaveCommand(const std::string& path) override {
        LOG_DBG("onSaveCommand");
        nlohmann::json json;
        scene_data.toJson(json);
        std::ofstream f(path);
        f << json.dump(4);
        return true;
    }

    bool onOpenCommand(const std::string& path) override {
        LOG_DBG("onOpenCommand");

        clearMode();

        std::ifstream f(path, std::ios::binary);
        if (!f.is_open()) {
            LOG_ERR("Failed to open file '" << path << "'");
            return false;
        }
        std::string fstr((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        nlohmann::json json = nlohmann::json::parse(fstr);
        if (!scene_data.fromJson(json)) {
            return false;
        }

        // Update the "preview" state
        for(int i = 0; i < scene_data.entries.size(); ++i) {
            world.spawn(scene_data.entries[i].instance.get());
        }
        // Update ui state
        scene_outliner->updateView();
        
        return true;
    }
};

