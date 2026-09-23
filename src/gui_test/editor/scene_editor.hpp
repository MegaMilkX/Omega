#pragma once

#include "editor_window.hpp"
#include "gui/elements/viewport/gui_viewport.hpp"
#include "gui_engine/actor_inspector.hpp"

struct SceneEntry {
    ResourceRef<ActorPrefab> prefab;
    std::unique_ptr<Actor> instance;
};

struct SceneData {
    std::vector<SceneEntry> entries;

    // TODO:
    //void toJson(nlohmann::json&);
    //bool fromJson(const nlohmann::json&);
};

struct GuiEvt_EntrySelected : public GuiEvent {
    GuiEvt_EntrySelected(SceneEntry* e) : entry(e) {}
    SceneEntry* entry = nullptr;
};

class GuiSceneInspector : public GuiElement {
    SceneData* scene_data = nullptr;

    GuiTreeView* list = nullptr;
    std::vector<GuiTreeItem*> items; // TODO: Should just access items through list
public:
    GuiSceneInspector(SceneData* scn)
    : scene_data(scn) {
        setSize(200, 400);
        list = guiCreate<GuiTreeView>();
        list->setSize(gui::fill(), gui::fill());
        pushBack(list);
        updateView();
    }

    void setSelected(SceneEntry* e) {
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
        
        auto& entries = scene_data->entries;
        for (int i = 0; i < entries.size(); ++i) {
            SceneEntry& entry = entries[i];
            Actor* actor = entry.instance.get();
            GuiTreeItem* item = list->addItem(entry.instance->getName().c_str());
            item->user_ptr = &entry;
            item->subscribe([this](const GuiEvt_Selected& e) {
                e.invoke_next();
                invoke(GuiEvt_EntrySelected(static_cast<SceneEntry*>(e.elem->user_ptr)));
            });
            items.push_back(item);
        }
    }
};

class GuiSceneDocument : public GuiEditorWindow {
    SceneData scene_data;

    // ===========================================
    GuiActorInspector* actor_inspector = nullptr;
    GuiSceneInspector* scene_inspector = nullptr;

    GameRenderInstance render_instance;

    gpuMesh mesh;
    std::unique_ptr<gpuGeometryRenderable> renderable;

    gpuMesh mesh2;
    std::unique_ptr<gpuGeometryRenderable> renderable2;
    ResourceRef<gpuMaterial> material2;

    SceneEntry* selected_entry = nullptr;

    void selectEntry(SceneEntry* e) {
        selected_entry = e;
        actor_inspector->init(e->instance.get());
        scene_inspector->setSelected(e);
    }

public:
    GuiViewport viewport;

    GuiSceneDocument(GuiActorInspector* inspector = nullptr)
        : actor_inspector(inspector), GuiEditorWindow("GenericScene", "scene")
    {
        scene_inspector = guiCreate<GuiSceneInspector>(&scene_data);
        scene_inspector->subscribe([this](const GuiEvt_EntrySelected& e) {
            selected_entry = e.entry;
            actor_inspector->init(e.entry->instance.get());
        });
        guiGetRoot()->getWindowLayer()->pushBack(scene_inspector);

        viewport.subscribe([this](const GuiEvt_RClick&) {
            auto menu = guiCreate<GuiMenuList>();
            guiAddTransientPopup(nullptr, menu, guiGetMousePos());
            auto item_create = menu->addItem("Create Actor...", 0);
            item_create->subscribe([this, menu](const GuiEvt_LClick&) {
                auto& entry = scene_data.entries.emplace_back();
                entry.instance.reset(new Actor);
                entry.instance->setName("Actor");
                render_instance.world.spawn(entry.instance.get());
                scene_inspector->updateView();
                selectEntry(&entry);
                guiRemoveTransientPopup(menu);
            });
            auto item_focus = menu->addItem("Focus selected", 0);
            item_focus->subscribe([this, menu](const GuiEvt_LClick&) {
                // TODO:
                guiRemoveTransientPopup(menu);
            });
        });

        render_instance.render_view = gpuGetPipeline()->createOffscreenView(RendererType::Default, 640, 480);
        render_instance.render_view->setView(gfxm::mat4(1.f));
        game_render_instances.insert(&render_instance);
        viewport.render_instance = &render_instance;
        {
            render_instance.render_view->addQueryInterface(render_instance.world.getSystem<SceneSystem>());
            render_instance.render_view->addQueryInterface(render_instance.world.getSystem<scnRenderScene>());
        }

        addChild(&viewport);
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
        render_instance.world.update(dt);
        guiScheduleTick(this, 1.f / 30.f, GUI_TICK_CUSTOM);
    }

    void onDraw() override {
        render_instance.render_view->getRenderBucket()->add(renderable.get());
        /*
        render_instance.render_view->getRenderBucket()->add(renderable2.get());
        viewport.render_instance->world.getRenderScene()->draw(render_instance.render_view->getRenderBucket());

        static float time = .0f;
        time += .01f;
        renderable2->setVec4("color", gfxm::vec4(gfxm::hsv2rgb(sinf(time * (1.f/7.f)), 1.f, 1.f), 1.f));
        */
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