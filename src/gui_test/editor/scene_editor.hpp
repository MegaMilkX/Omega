#pragma once

#include "editor_window.hpp"
#include "gui/elements/viewport/gui_viewport.hpp"
#include "gui/elements/viewport/tools/gui_viewport_tool_transform.hpp"
#include "gui/elements/dock_space.hpp"
#include "gui_engine/actor_inspector.hpp"

#include "scene_editor/terrain/terrain_space.hpp"


struct SceneEntry {
    ResourceRef<ActorPrefab> prefab;
    std::unique_ptr<Actor> instance;
};



constexpr static int SCENE_DATA_VERSION = 2;

struct SceneData : public gpuSceneQueryInterface {
    std::unique_ptr<SceneSpace> scene_space;
    std::vector<SceneEntry> entries;

    SceneData() {
        scene_space.reset(new TerrainSceneSpace);
    }

    void queryGeometry(const GeometryQuery& q) override {
        scene_space->queryGeometry(q);
    }

    void toJson(nlohmann::json& json) const {
        json["version"] = SCENE_DATA_VERSION;

        nlohmann::json& jentries = json["entries"];
        jentries = nlohmann::json::array();
        for (int i = 0; i < entries.size(); ++i) {
            ActorPrefab prefab;
            entries[i].instance->makePrefab(prefab);
            nlohmann::json& jentry = jentries.emplace_back();
            prefab.toJson(jentry);
        }

        nlohmann::json& jspace = json["space"];
        scene_space->toJson(jspace);
    }
    bool fromJson(const nlohmann::json& json) {
        int version = json["version"].get<int>();
        LOG("SCENE VERSION: " << version);

        nlohmann::json jentries = json.value("entries", nlohmann::json::array());
        if (!jentries.is_array()) {
            return false;
        }

        entries.clear();
        for (auto it = jentries.begin(); it != jentries.end(); ++it) {
            nlohmann::json jentry = it->get<nlohmann::json>();
            ActorPrefab prefab;
            if (!prefab.fromJson(jentry)) {
                LOG_ERR("Failed to load actor");
                continue;
            }
            auto& entry = entries.emplace_back();
            entry.instance = std::unique_ptr<Actor>(prefab.instantiate()); // TODO: new is hidden here, kinda uncomfortable
        }

        if (version >= 2) {
            nlohmann::json jspace = json.value("space", nlohmann::json::object());
            scene_space.reset(new TerrainSceneSpace); // TODO: scene space types
            scene_space->fromJson(jspace);
        }

        return true;
    }
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
    SceneEditorContext context;

    SceneData scene_data;

    // ===========================================
    GuiActorInspector* actor_inspector = nullptr;
    GuiSceneInspector* scene_inspector = nullptr;

    RuntimeWorld world;

    gpuMesh mesh;
    std::unique_ptr<gpuGeometryRenderable> renderable;

    gpuMesh mesh2;
    std::unique_ptr<gpuGeometryRenderable> renderable2;
    ResourceRef<gpuMaterial> material2;

    SceneEntry* selected_entry = nullptr;

    void enableTransformTool() {
        if (!selected_entry) {
            return;
        }
        viewport.removeTool(&tool_transform);
        viewport.addTool(&tool_transform);
        tool_transform.translation = selected_entry->instance->getTranslation();
        tool_transform.rotation = selected_entry->instance->getRotation();
    }

    void selectEntry(SceneEntry* e) {
        selected_entry = e;
        actor_inspector->init(e->instance.get());
        scene_inspector->setSelected(e);
    }

public:
    GuiDockSpace dock_space;
    GuiViewport viewport;
    GuiViewportToolTransform tool_transform;

    GuiSceneDocument()
        : GuiEditorWindow("GenericScene", "scene")
    {
        actor_inspector = guiCreate<GuiActorInspector>();
        actor_inspector->subscribe([this](const GuiEvt_PropChanged&) {
            enableTransformTool(); // TODO: actually just update transform data
        });

        scene_inspector = guiCreate<GuiSceneInspector>(&scene_data);
        scene_inspector->subscribe([this](const GuiEvt_EntrySelected& e) {
            selected_entry = e.entry;
            actor_inspector->init(e.entry->instance.get());
            enableTransformTool();
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

        viewport.getRenderView()->setView(gfxm::mat4(1.f));
        viewport.getRenderView()->setZFar(4000.f);
        viewport.getRenderView()->setZNear(.2f);
        {
            viewport.getRenderView()->addQueryInterface(world.getSystem<SceneSystem>());
            viewport.getRenderView()->addQueryInterface(world.getSystem<scnRenderScene>());
            viewport.getRenderView()->addQueryInterface(&scene_data);
        }

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
        node->right->addWindow(scene_inspector);
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

        //
        viewport.addTool(scene_data.scene_space.get());
        scene_data.scene_space->enterUi(context);
    }

    void onTick(float dt, GUI_TICK_ID id) override {
        if (id != GUI_TICK_CUSTOM) {
            return;
        }
        world.update(dt);
        guiScheduleTick(this, 1.f / 30.f, GUI_TICK_CUSTOM);
    }

    void onDraw() override {
        //viewport.getRenderView()->getRenderBucket()->add(renderable.get());
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
        nlohmann::json json;
        scene_data.toJson(json);
        std::ofstream f(path);
        f << json.dump(4);
        return true;
    }

    bool onOpenCommand(const std::string& path) override {
        LOG_DBG("onOpenCommand");

        if (scene_data.scene_space) {
            scene_data.scene_space->exitUi(context);
        }
        viewport.clearTools();

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
        scene_inspector->updateView();

        //
        viewport.addTool(scene_data.scene_space.get());
        scene_data.scene_space->enterUi(context);
        
        return true;
    }
};

