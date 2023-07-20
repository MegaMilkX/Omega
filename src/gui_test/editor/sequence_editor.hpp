#pragma once

#include "editor_window.hpp"
#include "animation/animator/animator_sequence.hpp"
#include "gui/elements/viewport/gui_viewport.hpp"

#include "uaf/uaf.hpp"


class GuiTimelineWindow;

// TODO: REMOVE
extern std::set<GameRenderInstance*> game_render_instances;

#include "animation/animation_uaf.hpp"

struct SequenceEditorData {
    float dt = 1.f / 60.f;
    timer timer_;

    bool is_playing = false;
    float prev_timeline_cursor = .0f;
    float timeline_cursor = .0f;
    RHSHARED<sklSkeletonMaster> skeleton;
    RHSHARED<Animation> sequence;
    animSampler sampler;
    animSampleBuffer samples;
    std::set<sklSkeletonInstance*> skeleton_instances;
    gameActor* actor;
    GuiTimelineWindow* tl_window;

    // tmp
    RHSHARED<AudioClip> test_clip;
};

class GuiTimelineWindow : public GuiWindow {
    SequenceEditorProject* project = 0;
    
    std::unique_ptr<GuiTimelineEditor> tl;
    SequenceEditorData* data;

    enum CMD {
        CMD_NEW_TRACK_EVENT,
        CMD_NEW_TRACK_AUDIO,
        CMD_NEW_TRACK_HITBOX
    };
public:
    std::function<void(SeqEdEvent*)> on_event_selected;
    std::function<void(SeqEdHitbox*)> on_hitbox_selected;
    std::function<void(SeqEdItem*)> on_item_destroyed;

    GuiTimelineWindow(SequenceEditorData* data)
        : GuiWindow("Timeline"), data(data) {
        setSize(800, 300);

        tl.reset(new GuiTimelineEditor);
        tl->setOwner(this);
        addChild(tl.get());
        {
            auto track = tl->addBlockTrack("Skeletal Animation");
            track->setLocked(true);
            auto block = track->addItem(0, 39);
            block->color = GUI_COL_BUTTON;
            block->setLocked(true);
        }

        tl->on_play = [data]() {
            data->is_playing = true;
            data->prev_timeline_cursor = data->timeline_cursor;
        };
        tl->on_pause = [data]() {
            data->is_playing = false;
            data->prev_timeline_cursor = data->timeline_cursor;
        };
        tl->on_cursor = [data](int cur) {
            data->timeline_cursor = cur;
            data->prev_timeline_cursor = cur;
        };

        createMenuBar()
            ->addItem(new GuiMenuItem("Add", {
                new GuiMenuListItem("Event Track", CMD_NEW_TRACK_EVENT),
                new GuiMenuListItem("Audio Track", CMD_NEW_TRACK_AUDIO),
                new GuiMenuListItem("Hitbox Track", CMD_NEW_TRACK_HITBOX)
            }));
    }

    void init(SequenceEditorProject* prj) {
        project = prj;
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
        switch (msg) {
        case GUI_MSG::NOTIFY:
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::MENU_COMMAND:
                switch (params.getB<int>()) {
                case CMD_NEW_TRACK_EVENT: {
                    SeqEdEventTrack* trk = project->eventTrackAdd();
                    GuiTimelineEventTrack* gui_trk = tl->addEventTrack(trk);
                    gui_trk->user_ptr = trk;
                    gui_trk->type = SEQ_ED_TRACK_EVENT;
                    break;
                }
                case CMD_NEW_TRACK_AUDIO: {
                    break;
                }
                case CMD_NEW_TRACK_HITBOX: {
                    SeqEdHitboxTrack* trk = project->hitboxTrackAdd();
                    GuiTimelineBlockTrack* gui_trk = tl->addBlockTrack();
                    gui_trk->user_ptr = trk;
                    gui_trk->type = SEQ_ED_TRACK_HITBOX;
                    break;
                }
                }
                return true;
            case GUI_NOTIFY::TIMELINE_EVENT_ADDED: {
                auto gui_trk = params.getB<GuiTimelineEventTrack*>();
                auto gui_item = params.getC<GuiTimelineEventItem*>();
                gui_item->user_ptr = project->eventAdd((SeqEdEventTrack*)gui_trk->user_ptr, gui_item->frame);
                return true;
            }
            case GUI_NOTIFY::TIMELINE_EVENT_REMOVED: {
                auto gui_trk = params.getB<GuiTimelineEventTrack*>();
                auto gui_item = params.getC<GuiTimelineEventItem*>();
                if (on_item_destroyed) {
                    on_item_destroyed((SeqEdEvent*)gui_item->user_ptr);
                }
                project->eventRemove((SeqEdEventTrack*)gui_trk->user_ptr, (SeqEdEvent*)gui_item->user_ptr);
                return true;
            }
            case GUI_NOTIFY::TIMELINE_EVENT_MOVED: {
                auto gui_trk = params.getB<GuiTimelineEventTrack*>();
                auto gui_item = params.getC<GuiTimelineEventItem*>();
                project->eventMove((SeqEdEvent*)gui_item->user_ptr, (SeqEdEventTrack*)gui_trk->user_ptr, gui_item->frame);
                return true;
            }
            case GUI_NOTIFY::TIMELINE_BLOCK_ADDED: {
                auto gui_trk = params.getB<GuiTimelineBlockTrack*>();
                auto gui_item = params.getC<GuiTimelineBlockItem*>();
                if (gui_trk->type == SEQ_ED_TRACK_HITBOX) {
                    gui_item->color = GUI_COL_RED;
                    gui_item->user_ptr = project->hitboxAdd(
                        (SeqEdHitboxTrack*)gui_trk->user_ptr, gui_item->frame, gui_item->length
                    );
                }
                return true;
            }
            case GUI_NOTIFY::TIMELINE_BLOCK_REMOVED:{
                auto gui_trk = params.getB<GuiTimelineBlockTrack*>();
                auto gui_item = params.getC<GuiTimelineBlockItem*>();
                if (gui_trk->type == SEQ_ED_TRACK_HITBOX) {
                    if (on_item_destroyed) {
                        on_item_destroyed((SeqEdHitbox*)gui_item->user_ptr);
                    }
                    project->hitboxRemove(
                        (SeqEdHitboxTrack*)gui_trk->user_ptr,
                        (SeqEdHitbox*)gui_item->user_ptr
                    );
                }
                return true;
            }
            case GUI_NOTIFY::TIMELINE_BLOCK_MOVED_RESIZED:{
                auto gui_trk = params.getB<GuiTimelineBlockTrack*>();
                auto gui_item = params.getC<GuiTimelineBlockItem*>();
                if (gui_trk->type == SEQ_ED_TRACK_HITBOX) {
                    project->hitboxMoveResize(
                        (SeqEdHitbox*)gui_item->user_ptr,
                        (SeqEdHitboxTrack*)gui_trk->user_ptr,
                        gui_item->frame,
                        gui_item->length
                    );
                }
                return true;
            }
            case GUI_NOTIFY::TIMELINE_EVENT_SELECTED: {
                auto gui_item = params.getC<GuiTimelineEventItem*>();
                if (on_event_selected) {
                    on_event_selected((SeqEdEvent*)gui_item->user_ptr);
                }
                return true;
            }
            case GUI_NOTIFY::TIMELINE_BLOCK_SELECTED: {
                auto gui_item = params.getC<GuiTimelineBlockItem*>();
                if (gui_item->type == SEQ_ED_TRACK_HITBOX) {
                    if (on_hitbox_selected) {
                        on_hitbox_selected((SeqEdHitbox*)gui_item->user_ptr);
                    }
                }
                return true;
            }
                
            }
            break;
        }

        return GuiWindow::onMessage(msg, params);
    }
    void setCursor(float cur) {
        tl->setCursorSilent(cur);
    }
};

class GuiTimelineEventInspector : public GuiElement {
    SeqEdEvent* event;
    std::unique_ptr<GuiComboBox> type_combo;
public:
    GuiTimelineEventInspector(SeqEdEvent* e)
        : event(e) {
        type_combo.reset(new GuiComboBox);
        type_combo->setOwner(this);
        addChild(type_combo.get());
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        return false;
    }

    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        rc_bounds = rc;
        client_area = rc;
        gfxm::vec2 cur = client_area.min;
        gfxm::rect rc_ = client_area;
        for (auto& ch : children) {
            ch->layout(rc_, flags);
            cur.y += ch->getClientArea().max.y - ch->getClientArea().min.y;
            rc_.min.y = cur.y;
        }
    }
    void onDraw() override {
        for (auto& ch : children) {
            ch->draw();
        }
    }
};
class GuiTimelineHitboxInspector : public GuiElement {
    SeqEdHitbox* hitbox;
    GuiComboBox shape_combo;
    GuiInputFloat3 translation = GuiInputFloat3("Translation", 0, 2);
    GuiInputFloat3 rotation = GuiInputFloat3("Rotation", 0, 2);
public:
    GuiTimelineHitboxInspector(SeqEdHitbox* hb)
        : hitbox(hb) {
        shape_combo.setOwner(this);
        translation.setOwner(this);
        rotation.setOwner(this);
        addChild(&shape_combo);
        addChild(&translation);
        addChild(&rotation);
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        rc_bounds = rc;
        client_area = rc;
        gfxm::vec2 cur = client_area.min;
        gfxm::rect rc_ = client_area;
        for (auto& ch : children) {
            ch->layout(rc_, flags);
            cur.y += ch->getClientArea().max.y - ch->getClientArea().min.y;
            rc_.min.y = cur.y;
        }
    }
    void onDraw() override {
        for (auto& ch : children) {
            ch->draw();
        }
    }
};
class GuiTimelineItemInspectorWindow : public GuiWindow {
    enum MODE {
        NONE,
        EVENT,
        HITBOX
    };
    MODE mode = NONE;
    SeqEdItem* selected_item = 0;

    std::unique_ptr<GuiLabel> label;
    std::unique_ptr<GuiElement> inspector;
public:
    GuiTimelineItemInspectorWindow()
        : GuiWindow("Inspector") {
        setSize(300, 500);        

        label.reset(new GuiLabel("Nothing selected"));
        addChild(label.get());

    }

    void init(SeqEdEvent* e) {
        if (inspector) {
            removeChild(inspector.get());
        }
        selected_item = e;
        inspector.reset(new GuiTimelineEventInspector(e));
        inspector->setOwner(this);
        addChild(inspector.get());

        label->setCaption("Event");
    }
    void init(SeqEdHitbox* hb) {
        if (inspector) {
            removeChild(inspector.get());
        }
        selected_item = hb;
        inspector.reset(new GuiTimelineHitboxInspector(hb));
        inspector->setOwner(this);
        addChild(inspector.get());

        label->setCaption("Hitbox");
    }
    void onItemDestroyed(SeqEdItem* item) {
        if (selected_item != item) {
            return;
        }
        selected_item = 0;
        if (inspector) {
            removeChild(inspector.get());
        }
        inspector.reset();
        label->setCaption("Nothing selected");
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
        return GuiWindow::onMessage(msg, params);
    }
};

inline void sequenceEditorInit(
    SequenceEditorData& data,
    RHSHARED<sklSkeletonMaster> skl,
    RHSHARED<Animation> sequence,
    gameActor* actor,
    GuiTimelineWindow* tl_window
) {
    data.skeleton_instances.clear();
    data.skeleton = skl;
    data.sequence = sequence;
    data.sampler = animSampler(skl.get(), sequence.get());
    data.samples.init(skl.get());
    actor->forEachNode<nodeSkeletalModel>([&data](nodeSkeletalModel* node) {
        data.skeleton_instances.insert(node->getModelInstance()->getSkeletonInstance());
    });
    data.actor = actor;
    data.tl_window = tl_window;
    
    // tmp
    data.test_clip = resGet<AudioClip>("audio/sfx/gravel1.ogg");
}
inline void sequenceEditorUpdateAnimFrame(SequenceEditorProject& proj, SequenceEditorData& data) {
    timer timer_;
    timer_.start();
    audio().setListenerTransform(gfxm::mat4(1.f));

    float anim_len = data.sequence->length;

    data.sampler = animSampler(data.skeleton.get(), data.sequence.get());
    data.samples.has_root_motion = false;
    for (auto& skl_inst : data.skeleton_instances) {
        auto model_inst_skel = skl_inst->getSkeletonMaster();
        if (data.skeleton.get() != model_inst_skel) {
            continue;
        }
        data.sampler.sample(&data.samples[0], data.samples.count(), data.timeline_cursor);
        data.samples.applySamples(skl_inst);

        {
            auto& seq = proj.event_seq;
            animEventBuffer buf;
            buf.reserve(seq->eventCount());
            seq->sample(&buf, data.prev_timeline_cursor, data.timeline_cursor, anim_len);
            for (int j = 0; j < buf.eventCount(); ++j) {
                audio().playOnce3d(data.test_clip->getBuffer(), gfxm::vec3(.0f, .0f, .0f), .1f);
            }
        }
        {
            auto& seq = proj.hitbox_seq;
            animHitboxBuffer buf;
            buf.reserve(seq->blockCount());
            seq->sample(&buf, data.prev_timeline_cursor, data.timeline_cursor);
            for (int j = 0; j < buf.hitboxCount(); ++j) {
                // TODO: Draw hitboxes
                dbgDrawSphere(gfxm::mat4(1.f), .5f, 0xFF0000FF);
            }
        }
    }
    if (data.timer_.is_started()) {
        data.dt = data.timer_.stop();
    }
    if (data.is_playing) {
        data.prev_timeline_cursor = data.timeline_cursor;
        data.timeline_cursor += data.sequence->fps * data.dt;
        if (data.timeline_cursor > data.sequence->length) {
            data.timeline_cursor -= data.sequence->length;
        }
        data.tl_window->setCursor(data.timeline_cursor);
    }
    data.timer_.start();
}

class GuiSequenceDocument : public GuiEditorWindow {
    SequenceEditorData seqed_data;
    SequenceEditorProject seq_ed_proj;

    GuiDockSpace dock_space;
    GuiViewport viewport;
    GuiWindow vp_wnd;
    GuiTimelineWindow timeline;
    GuiTimelineItemInspectorWindow timeline_inspector;

    RHSHARED<Animation> animation;
    RHSHARED<sklSkeletonMaster> skeleton_master;
    HSHARED<sklSkeletonInstance> skeleton_instance;
    RHSHARED<mdlSkeletalModelMaster> model_master;
    HSHARED<mdlSkeletalModelInstance> model_instance;

    gpuRenderTarget render_target;
    gpuRenderBucket render_bucket;
    GameRenderInstance render_instance;
    gameActor actor;
    RHSHARED<Animation> seq_run;

    RHSHARED<gpuMaterial> material_color;
    gpuMesh gpu_mesh_plane;
    std::unique_ptr<gpuRenderable> renderable_plane;
    gpuUniformBuffer* renderable_plane_ubuf;

    void updateReferenceDisplayData() {
        if (animation) {
            seq_run = animation;
        }

        render_instance.world.despawnActor(&actor);
        auto root = actor.setRoot<nodeCharacterCapsule>("capsule");
        auto node = root->createChild<nodeSkeletalModel>("model");
        node->setModel(model_master);
        render_instance.world.spawnActor(&actor);

        sequenceEditorInit(
            seqed_data,
            skeleton_master,
            seq_run,
            &actor,
            &timeline
        );
    }
public:
    GuiSequenceDocument()
        : GuiEditorWindow("SequenceDocument"),
        timeline(&seqed_data),
        render_bucket(gpuGetPipeline(), 1000),
        render_target(800, 600)
    {
        dock_space.setDockGroup(this);
        vp_wnd.setDockGroup(this);
        timeline.setDockGroup(this);
        timeline_inspector.setDockGroup(this);

        {
            guiAdd(&timeline_inspector, this, new GuiInputFilePath("Animation", 
                [this](const std::string& path) {
                    animation = resGet<Animation>(path.c_str());
                    updateReferenceDisplayData();
                }, 
                [this]()->std::string {
                    if (!animation) {
                        return "";
                    }
                    return animation.getReferenceName();
                }, 
                GUI_INPUT_FILE_READ, "anim", fsGetCurrentDirectory().c_str()), GUI_FLAG_PERSISTENT
            );
            guiAdd(&timeline_inspector, this, new GuiInputFilePath("Skeleton",
                [this](const std::string& path) {
                    if (skeleton_master && skeleton_instance) {
                        skeleton_instance.reset(0);
                    }
                    skeleton_master = resGet<sklSkeletonMaster>(path.c_str());
                    if (skeleton_master) {
                        skeleton_instance = skeleton_master->createInstance();
                    }
                    updateReferenceDisplayData();
                }, 
                [this]()->std::string {
                    if (!skeleton_master) {
                        return "";
                    }
                    return skeleton_master.getReferenceName();
                }, 
                GUI_INPUT_FILE_READ, "skeleton", fsGetCurrentDirectory().c_str()), GUI_FLAG_PERSISTENT
            );
            guiAdd(&timeline_inspector, this, new GuiInputFilePath("Reference model",
                [this](const std::string& path) {
                    if (model_master && model_instance) {
                        model_instance.reset(0);
                    }
                    model_master = resGet<mdlSkeletalModelMaster>(path.c_str());
                    if (model_master) {
                        model_instance = model_master->createInstance();
                    }
                    updateReferenceDisplayData();
                }, 
                [this]()->std::string {
                    if (!model_master) {
                        return "";
                    }
                    return model_master.getReferenceName();
                }, 
                GUI_INPUT_FILE_READ, "skeletal_model", fsGetCurrentDirectory().c_str()), GUI_FLAG_PERSISTENT
            );
        }

        content_padding = gfxm::rect(0, 0, 0, 0);
        addChild(&dock_space);

        vp_wnd.addChild(&viewport);
        vp_wnd.setOwner(this);
        dock_space.getRoot()->addWindow(&vp_wnd);

        timeline.on_event_selected = [this](SeqEdEvent* e) {
            timeline_inspector.init(e);
        };
        timeline.on_hitbox_selected = [this](SeqEdHitbox* hb) {
            timeline_inspector.init(hb);
        };
        timeline.on_item_destroyed = [this](SeqEdItem* item) {
            timeline_inspector.onItemDestroyed(item);
        };

        dock_space.getRoot()->splitBottom();
        dock_space.getRoot()->split_pos = .7f;
        dock_space.getRoot()->right->addWindow(&timeline);
        dock_space.getRoot()->left->splitRight();
        dock_space.getRoot()->left->split_pos = .8f;
        dock_space.getRoot()->left->right->addWindow(&timeline_inspector);

        {
            seq_run.reset_acquire();
            seq_run = getAnimation("models/chara_24/Run.anim");
        }
        {
            auto root = actor.setRoot<nodeCharacterCapsule>("capsule");
            auto node = root->createChild<nodeSkeletalModel>("model");
            node->setModel(resGet<mdlSkeletalModelMaster>("models/chara_24/chara_24.skeletal_model"));
        }
        sequenceEditorInit(
            seqed_data,
            resGet<sklSkeletonMaster>("models/chara_24/chara_24.skeleton"),
            seq_run,
            &actor,
            &timeline
        );
        timeline.init(&seq_ed_proj);

        gpuGetPipeline()->initRenderTarget(&render_target);
        render_instance.world.spawnActor(&actor);
        render_instance.render_bucket = &render_bucket;
        render_instance.render_target = &render_target;
        render_instance.view_transform = gfxm::mat4(1.0f);
        game_render_instances.insert(&render_instance);
        viewport.render_instance = &render_instance;


        material_color = resGet<gpuMaterial>("materials/color.mat");
        Mesh3d mesh_plane;
        meshGenerateCheckerPlane(&mesh_plane, 50, 50, 50);
        gpu_mesh_plane.setData(&mesh_plane);
        renderable_plane.reset(new gpuRenderable(material_color.get(), gpu_mesh_plane.getMeshDesc()));
        renderable_plane_ubuf = gpuGetPipeline()->createUniformBuffer(UNIFORM_BUFFER_MODEL);
        renderable_plane->attachUniformBuffer(renderable_plane_ubuf);
        renderable_plane_ubuf->setMat4(
            gpuGetPipeline()->getUniformBufferDesc(UNIFORM_BUFFER_MODEL)->getUniform("matModel"),
            gfxm::mat4(1.f)
        );
    }
    ~GuiSequenceDocument() {
        game_render_instances.erase(&render_instance);
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        render_instance.render_bucket->add(renderable_plane.get());
        sequenceEditorUpdateAnimFrame(seq_ed_proj, seqed_data);
        GuiWindow::onLayout(rc, flags);
    }
};
