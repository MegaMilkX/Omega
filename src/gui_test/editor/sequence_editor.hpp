#pragma once

#include "editor_window.hpp"
#include "animation/animator/animator_sequence.hpp"
#include "gui/elements/viewport/gui_viewport.hpp"

#include "world/node/node_decal.hpp"
#include "world/node/node_particle_emitter.hpp"

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
    RHSHARED<Skeleton> skeleton;
    RHSHARED<Animation> sequence;
    animSampler sampler;
    animSampleBuffer samples;
    std::set<SkeletonPose*> skeleton_instances;
    Actor* actor;
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

    GuiTimelineEditor* getTimeline() { return tl.get(); }

    void init(SequenceEditorProject* prj) {
        project = prj;
    }
    void setCursor(float cur) {
        tl->setCursorSilent(cur);
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
    std::unique_ptr<GuiInputFloat3> translation;
    std::unique_ptr<GuiInputFloat3> rotation;
public:
    GuiTimelineHitboxInspector(SeqEdHitbox* hb)
        : hitbox(hb) {
        shape_combo.setOwner(this);
        translation.reset(new GuiInputFloat3("Translation", 0, 2));
        rotation.reset(new GuiInputFloat3("Rotation", 0, 2));
        translation->setOwner(this);
        rotation->setOwner(this);
        addChild(&shape_combo);
        addChild(translation.get());
        addChild(rotation.get());
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
    RHSHARED<Skeleton> skl,
    RHSHARED<Animation> sequence,
    Actor* actor,
    GuiTimelineWindow* tl_window
) {
    data.skeleton_instances.clear();
    data.skeleton = skl;
    data.sequence = sequence;
    data.sampler = animSampler(skl.get(), sequence.get());
    data.samples.init(skl.get());
    actor->forEachNode<SkeletalModelNode>([&data](SkeletalModelNode* node) {
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

#include "world/experimental/actor_anim.hpp"
class GuiSequenceDocument : public GuiEditorWindow {
    SequenceEditorData seqed_data;
    SequenceEditorProject seq_ed_proj;

    GuiDockSpace dock_space;
    GuiViewport viewport;
    GuiWindow vp_wnd;
    GuiTimelineWindow timeline;
    GuiTimelineItemInspectorWindow timeline_inspector;
    std::unique_ptr<GuiElement> prop_container;
    GuiWindow wnd_new_timeline;

    RHSHARED<Animation> animation;
    RHSHARED<Skeleton> skeleton_master;
    HSHARED<SkeletonPose> skeleton_instance;
    RHSHARED<mdlSkeletalModelMaster> model_master;
    HSHARED<mdlSkeletalModelInstance> model_instance;

    gpuRenderTarget render_target;
    gpuRenderBucket render_bucket;
    GameRenderInstance render_instance;
    Actor actor;
    ActorSampleBuffer buf;
    ActorAnimation anim;
    ActorAnimSampler sampler;
    RHSHARED<Animation> seq_run;

    RHSHARED<gpuMaterial> material_color;
    gpuMesh gpu_mesh_plane;
    std::unique_ptr<gpuRenderable> renderable_plane;
    gpuUniformBuffer* renderable_plane_ubuf;

    GuiTreeItem* selected_tree_item = 0;
    gameActorNode* selected_node = 0;

    std::vector<GuiInputBase*> property_inputs;

    void updateReferenceDisplayData() {
        if (animation) {
            seq_run = animation;
        }

        render_instance.world.despawnActor(&actor);
        actor.setFlags(ACTOR_FLAG_UPDATE);
        auto root = actor.setRoot<CharacterCapsuleNode>("capsule");
        auto node = root->createChild<SkeletalModelNode>("model");
        node->setModel(model_master);
        auto decal = root->createChild<DecalNode>("decal");
        decal->setTexture(resGet<gpuTexture2d>("textures/decals/pentagram.png"));
        auto emitter = root->createChild<ParticleEmitterNode>("emitter");
        emitter->setEmitter(resGet<ParticleEmitterMaster>("particle_emitters/test_emitter.pte"));
        render_instance.world.spawnActor(&actor);

        sequenceEditorInit(
            seqed_data,
            skeleton_master,
            seq_run,
            &actor,
            &timeline
        );
    }

    void onNodePropertyModified(gameActorNode* node, const type_property_desc* prop) {
        std::string track_name = node->getName() + "." + prop->name;
        auto tl = timeline.getTimeline();
        auto track = tl->findKeyframeTrack(track_name);
        if (!track) {
            if (prop->t == type_get<float>()) {
                track = tl->addKeyframeTrack(GUI_KEYFRAME_TYPE::FLOAT, track_name.c_str(), 0);
            } else if (prop->t == type_get<gfxm::vec2>()) {
                track = tl->addKeyframeTrack(GUI_KEYFRAME_TYPE::VEC2, track_name.c_str(), 0);
            } else if (prop->t == type_get<gfxm::vec3>()) {
                track = tl->addKeyframeTrack(GUI_KEYFRAME_TYPE::VEC3, track_name.c_str(), 0);
            } else if (prop->t == type_get<gfxm::vec4>()) {
                track = tl->addKeyframeTrack(GUI_KEYFRAME_TYPE::VEC4, track_name.c_str(), 0);
            } else {
                return;
            }
        }
        auto item = track->addItem(tl->getCursor(), false);
        if (prop->t == type_get<float>()) {
            item->setFloat(prop->getValue<float>(node));
        } else if (prop->t == type_get<gfxm::vec2>()) {
            item->setVec2(prop->getValue<gfxm::vec2>(node));
        } else if (prop->t == type_get<gfxm::vec3>()) {
            item->setVec3(prop->getValue<gfxm::vec3>(node));
        } else if (prop->t == type_get<gfxm::vec4>()) {
            item->setVec4(prop->getValue<gfxm::vec4>(node));
        } else {
            assert(false);
            return;
        }
    }

    void buildActorTreeGuiImpl(gameActorNode* node, GuiElement* container) {
        auto item = new GuiTreeItem(node->getName().c_str());
        guiAdd(container, container, item);
        item->on_click = [this, node](GuiTreeItem* itm) {
            if (selected_tree_item) {
                selected_tree_item->setSelected(false);
                selected_tree_item = 0;
            }
            itm->setSelected(true);
            selected_tree_item = itm;
            selected_node = node;
            buildNodePropertyList(selected_node, prop_container.get());
        };

        for (int i = 0; i < node->childCount(); ++i) {
            auto n = node->getChild(i);
            buildActorTreeGuiImpl(const_cast<gameActorNode*>(n), item);
        }
    }
    void buildActorTreeGui(Actor* actor, GuiElement* container) {
        selected_tree_item = 0;
        selected_node = 0;
        container->clearChildren();
        buildActorTreeGuiImpl(actor->getRoot(), container);
    }
    void buildPropertyControl(gameActorNode* node, const type_property_desc* prop, GuiElement* container) {
        void* object = node;

        if (prop->t == type_get<gfxm::vec4>()) {
            auto control = new GuiInputFloat4(prop->name.c_str(),
                [object, prop](float* out) {
                    gfxm::vec4 v = prop->getValue<gfxm::vec4>(object);
                    *(gfxm::vec4*)out = v;
                },
                [this, node, object, prop](float* in) {
                    prop->setValue(object, (gfxm::vec4*)in);
                    onNodePropertyModified(node, prop);
                },
                2, false
            );
            property_inputs.push_back(control);
            container->pushBack(control);
        } else if(prop->t == type_get<gfxm::vec3>()) {
            auto control = new GuiInputFloat3(prop->name.c_str(),
                [object, prop](float* out) {
                    gfxm::vec3 v = prop->getValue<gfxm::vec3>(object);
                    *(gfxm::vec3*)out = v;
                },
                [this, node, object, prop](float* in) {
                    prop->setValue(object, (gfxm::vec3*)in);
                    onNodePropertyModified(node, prop);
                },
                2, false
            );
            property_inputs.push_back(control);
            container->pushBack(control);
        } else {
            container->pushBack(new GuiLabel(prop->name.c_str()));
        }
    }
    void buildNodePropertyList(gameActorNode* node, GuiElement* container) {
        property_inputs.clear();
        
        container->clearChildren();

        auto type = node->get_type();
        container->pushBack(new GuiLabel(node->getName().c_str()));
        container->pushBack(new GuiInputFloat3("translation",
            [node](float* out) {
                gfxm::vec3 p = node->getTranslation();
                *(gfxm::vec3*)out = p;
            },
            [node](float* in) {
                node->setTranslation(*(gfxm::vec3*)in);
            },
            2, false
        ));
        container->pushBack(new GuiInputFloat3("rotation",
            [node](float* out) {
                gfxm::quat q = node->getRotation();
                gfxm::vec3 euler = gfxm::to_euler(q);
                *(gfxm::vec3*)out = euler;
            },
            [node](float* in) {
                node->setRotation(gfxm::euler_to_quat(*(gfxm::vec3*)in));
            },
            2, false
        ));
        for (auto& prop : type.get_desc()->properties) {
            buildPropertyControl(node, &prop, container);
        }
    }
public:
    GuiSequenceDocument()
        : GuiEditorWindow("SequenceDocument", "anim"),
        timeline(&seqed_data),
        render_bucket(gpuGetPipeline(), 1000),
        render_target(800, 600)
    {
        dock_space.setDockGroup(this);
        vp_wnd.setDockGroup(this);
        timeline.setDockGroup(this);
        timeline.setOwner(this);
        timeline_inspector.setDockGroup(this);

        wnd_new_timeline.setOwner(this);
        {
            GuiTreeHandler* e = new GuiTreeHandler;
            e->setSize(gui::perc(20), gui::perc(100));
            GuiTreeItem* itm = new GuiTreeItem("Summary");
            e->pushBack(itm);
            GuiTreeItem* itm_box = itm->addItem("Box");
            GuiTreeItem* itm_transform = itm_box->addItem("Transform");
            itm_transform->addItem("X Translation");
            itm_transform->addItem("Y Translation");
            itm_transform->addItem("Z Translation");
            itm_transform->addItem("X Euler");
            itm_transform->addItem("Y Euler");
            itm_transform->addItem("Z Euler");
            itm_transform->addItem("X Scale");
            itm_transform->addItem("Y Scale");
            itm_transform->addItem("Z Scale");

            wnd_new_timeline.pushBack(e);
        }
        {
            GuiAnimCurveView* e = new GuiAnimCurveView;
            e->addFlags(GUI_FLAG_SAME_LINE);
            wnd_new_timeline.pushBack(e);
        }

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
        dock_space.getRoot()->right->addWindow(&wnd_new_timeline);
        dock_space.getRoot()->left->splitRight();
        dock_space.getRoot()->left->split_pos = .8f;
        dock_space.getRoot()->left->right->addWindow(&timeline_inspector);

        {
            seq_run.reset_acquire();
            seq_run = getAnimation("models/chara_24/Run.anim");
        }
        {
            actor.setFlags(ACTOR_FLAG_UPDATE);
            auto root = actor.setRoot<CharacterCapsuleNode>("capsule");
            auto node = root->createChild<SkeletalModelNode>("model");
            node->setModel(resGet<mdlSkeletalModelMaster>("models/chara_24/chara_24.skeletal_model"));
            auto decal = root->createChild<DecalNode>("decal");
            decal->setTexture(resGet<gpuTexture2d>("textures/decals/pentagram.png"));
            decal->setColor(gfxm::vec4(1, 1, 1, 1));
            decal->setSize(2, 1, 2);
            auto emitter = root->createChild<ParticleEmitterNode>("emitter");
            emitter->setEmitter(resGet<ParticleEmitterMaster>("particle_emitters/test_emitter.pte"));
            emitter->setTranslation(0, 0, 0);
            buf.initialize(&actor);
            sampler.init(&anim, &buf);
        }
        sequenceEditorInit(
            seqed_data,
            resGet<Skeleton>("models/chara_24/chara_24.skeleton"),
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

        
        {
            auto tree_view = new GuiTreeView();
            guiAdd(&timeline_inspector, this, tree_view);
            buildActorTreeGui(&actor, tree_view);

            prop_container.reset(new GuiElement);
            prop_container->overflow = GUI_OVERFLOW_FIT;
            guiAdd(&timeline_inspector, this, prop_container.get());            
            buildNodePropertyList(actor.getRoot(), prop_container.get());

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
                    skeleton_master = resGet<Skeleton>(path.c_str());
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
    }
    ~GuiSequenceDocument() {
        game_render_instances.erase(&render_instance);
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
        switch (msg) {
        case GUI_MSG::NOTIFY: {
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::TIMELINE_KEYFRAME_REMOVED:
            case GUI_NOTIFY::TIMELINE_KEYFRAME_ADDED: {
                auto track = params.getB<GuiTimelineKeyframeTrack*>();
                auto item = params.getC<GuiTimelineKeyframeItem*>();
                if (track->getType() == GUI_KEYFRAME_TYPE::FLOAT) {
                    auto n = anim.createFloatNode(track->getName());
                    for (int i = 0; i < track->keyframeCount(); ++i) {
                        auto kf = track->getKeyframe(i);
                        n->curve_[kf->frame] = kf->getFloat();
                    }
                } else if (track->getType() == GUI_KEYFRAME_TYPE::VEC2) {
                    auto n = anim.createVec2Node(track->getName());
                    for (int i = 0; i < track->keyframeCount(); ++i) {
                        auto kf = track->getKeyframe(i);
                        n->curve_[kf->frame] = kf->getVec2();
                    }
                } else if (track->getType() == GUI_KEYFRAME_TYPE::VEC3) {
                    auto n = anim.createVec3Node(track->getName());
                    for (int i = 0; i < track->keyframeCount(); ++i) {
                        auto kf = track->getKeyframe(i);
                        n->curve_[kf->frame] = kf->getVec3();
                    }
                } else if (track->getType() == GUI_KEYFRAME_TYPE::VEC4) {
                    auto n = anim.createVec4Node(track->getName());
                    for (int i = 0; i < track->keyframeCount(); ++i) {
                        auto kf = track->getKeyframe(i);
                        n->curve_[kf->frame] = kf->getVec4();
                    }
                }
                sampler.init(&anim, &buf);
                break;
            }
            }
            return true;
        }
        }
        return GuiWindow::onMessage(msg, params);
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        render_instance.render_bucket->add(renderable_plane.get());
        sequenceEditorUpdateAnimFrame(seq_ed_proj, seqed_data);
        buf.clear_flags();
        sampler.sampleAt(seqed_data.timeline_cursor);
        buf.apply();
        for (auto inp : property_inputs) {
            inp->refreshData();
        }
        GuiWindow::onLayout(rc, flags);
    }
};
