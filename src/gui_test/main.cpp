#include <Windows.h>

#include <assert.h>

#include <string>

#include "platform/gl/glextutil.h"
#include "log/log.hpp"
#include "util/timer.hpp"

#include <unordered_map>
#include <memory>

#include "math/intersection.hpp"

#include "reflect.hpp"
#include "tools/import/import.hpp"

#include "gpu/gpu.hpp"
#include "gui/gui.hpp"
#include "typeface/font.hpp"
#include "resource/resource.hpp"
#include "input/input.hpp"
#include "mesh3d/generate_primitive.hpp"

#include "skeletal_model/skeletal_model.hpp"
#include "world/world.hpp"
#include "world/node/node_character_capsule.hpp"
#include "world/node/node_skeletal_model.hpp"
#include "world/component/components.hpp"
#include "world/controller/actor_controllers.hpp"

static float g_dt = 1.0f / 60.0f;
static float g_timeline_cursor = .0f;

class GuiTimelineWindow;

#include "animation/animator/animator_sequence.hpp"

enum SEQ_ED_TRACK_TYPE {
    SEQ_ED_TRACK_EVENT,
    SEQ_ED_TRACK_HITBOX
};

struct SeqEdItem {
    virtual ~SeqEdItem() {}
};

struct SeqEdEventTrack;
struct SeqEdEvent : public SeqEdItem {
    SeqEdEventTrack* track;
    int frame;
    // TODO:
};
struct SeqEdEventTrack {
    std::set<SeqEdEvent*> events;
    ~SeqEdEventTrack() {
        for (auto& e : events) {
            delete e;
        }
        events.clear();
    }
};

struct SeqEdHitboxTrack;
struct SeqEdHitbox : public SeqEdItem {
    SeqEdHitboxTrack* track;
    int frame;
    int length;
    // TODO:
};
struct SeqEdHitboxTrack {
    std::set<SeqEdHitbox*> hitboxes;
    ~SeqEdHitboxTrack() {
        for (auto& h : hitboxes) {
            delete h;
        }
        hitboxes.clear();
    }
};

struct SequenceEditorProject {
    RHSHARED<animEventSequence> event_seq;
    RHSHARED<animHitboxSequence> hitbox_seq;

    std::vector<std::unique_ptr<SeqEdEventTrack>> event_tracks;
    std::vector<std::unique_ptr<SeqEdHitboxTrack>> hitbox_tracks;

    SequenceEditorProject() {
        event_seq.reset_acquire();
        hitbox_seq.reset_acquire();
    }

    // Events
    SeqEdEventTrack* eventTrackAdd() {
        auto trk = new SeqEdEventTrack;
        event_tracks.push_back(std::unique_ptr<SeqEdEventTrack>(trk));
        // No need to recompile event sequence since new track is empty
        return trk;
    }
    void eventTrackRemove(SeqEdEventTrack* trk) {
        for (int i = 0; i < event_tracks.size(); ++i) {
            if (event_tracks[i].get() == trk) {
                event_tracks.erase(event_tracks.begin() + i);
                compileEventSequence();
                break;
            }
        }
    }
    SeqEdEvent* eventAdd(SeqEdEventTrack* track, int frame) {
        auto e = new SeqEdEvent;
        e->frame = frame;
        e->track = track;
        track->events.insert(e);
        compileEventSequence();
        return e;
    }
    void eventRemove(SeqEdEventTrack* track, SeqEdEvent* item) {
        track->events.erase(item);
        delete item;
        compileEventSequence();
    }
    void eventMove(SeqEdEvent* event, SeqEdEventTrack* destination_track, int destination_frame) {
        event->track->events.erase(event);
        destination_track->events.insert(event);
        event->track = destination_track;
        event->frame = destination_frame;
        compileEventSequence();
    }

    void compileEventSequence() {
        event_seq->clear();
        for (auto& trk : event_tracks) {
            for (auto& e : trk->events) {
                event_seq->insert(e->frame, 0/* TODO */);
            }
        }
    }

    // Hitboxes
    SeqEdHitboxTrack* hitboxTrackAdd() {
        auto trk = new SeqEdHitboxTrack;
        hitbox_tracks.push_back(std::unique_ptr<SeqEdHitboxTrack>(trk));
        return trk;
    }
    void hitboxTrackRemove(SeqEdHitboxTrack* trk) {
        for (int i = 0; i < hitbox_tracks.size(); ++i) {
            if (hitbox_tracks[i].get() == trk) {
                hitbox_tracks.erase(hitbox_tracks.begin() + i);
                compileHitboxSequence();
                break;
            }
        }
    }
    SeqEdHitbox* hitboxAdd(SeqEdHitboxTrack* track, int frame, int len) {
        auto hb = new SeqEdHitbox;
        hb->frame = frame;
        hb->length = len;
        hb->track = track;
        track->hitboxes.insert(hb);
        compileHitboxSequence();
        return hb;
    }
    void hitboxRemove(SeqEdHitboxTrack* track, SeqEdHitbox* item) {
        track->hitboxes.erase(item);
        delete item;
        compileHitboxSequence();
    }
    void hitboxMoveResize(SeqEdHitbox* hitbox, SeqEdHitboxTrack* dest_track, int dest_frame, int dest_len) {
        hitbox->track->hitboxes.erase(hitbox);
        dest_track->hitboxes.insert(hitbox);
        hitbox->track = dest_track;
        hitbox->frame = dest_frame;
        hitbox->length = dest_len;
        compileHitboxSequence();
    }
    void compileHitboxSequence() {
        hitbox_seq->clear();
        for (auto& trk : hitbox_tracks) {
            for (auto& h : trk->hitboxes) {
                hitbox_seq->insert(h->frame, h->length, ANIM_HITBOX_SPHERE);
            }
        }
    }

    void serializeJson() {
        // TODO
    }
    void deserializeJson() {
        // TODO
    }
};

struct SequenceEditorData {
    float dt = 1.f / 60.f;
    timer timer_;

    bool is_playing = false;
    float prev_timeline_cursor = .0f;
    float timeline_cursor = .0f;
    RHSHARED<sklSkeletonMaster> skeleton;
    RHSHARED<animSequence> sequence;
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
        size = gfxm::vec2(800, 300);

        tl.reset(new GuiTimelineEditor);
        tl->setOwner(this);
        addChild(tl.get());

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
    GuiInputNumeric translation = GuiInputNumeric("Translation", GUI_INPUT_NUMBER_TYPE::FLOAT, 3, 2);
    GuiInputNumeric rotation = GuiInputNumeric("Rotation", GUI_INPUT_NUMBER_TYPE::FLOAT, 3, 2);
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
        size = gfxm::vec2(300, 500);

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

class EditorGuiSequenceResourceList : public GuiWindow {
    std::unique_ptr<GuiContainerInner> container;
public:
    EditorGuiSequenceResourceList()
    : GuiWindow("Resources") {
        setSize(400, 500);
        setMinSize(200, 250);
        container.reset(new GuiContainerInner);
        container->setOwner(this);
        addChild(container.get());
    }
};

void sequenceEditorInit(
    SequenceEditorData& data,
    RHSHARED<sklSkeletonMaster> skl,
    RHSHARED<animSequence> sequence,
    gameActor* actor,
    GuiTimelineWindow* tl_window
) {
    data.skeleton_instances.clear();
    data.skeleton = skl;
    data.sequence = sequence;
    data.sampler = animSampler(skl.get(), sequence->getSkeletalAnimation().get());
    data.samples.init(skl.get());
    actor->forEachNode<nodeSkeletalModel>([&data](nodeSkeletalModel* node) {
        data.skeleton_instances.insert(node->getModelInstance()->getSkeletonInstance());
    });
    data.actor = actor;
    data.tl_window = tl_window;
    
    // tmp
    data.test_clip = resGet<AudioClip>("audio/sfx/gravel1.ogg");
}
void sequenceEditorUpdateAnimFrame(SequenceEditorProject& proj, SequenceEditorData& data) {
    timer timer_;
    timer_.start();
    audio().setListenerTransform(gfxm::mat4(1.f));

    float anim_len = data.sequence->getSkeletalAnimation()->length;

    data.sampler = animSampler(data.skeleton.get(), data.sequence->getSkeletalAnimation().get());
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
        data.timeline_cursor += data.sequence->getSkeletalAnimation()->fps * data.dt;
        if (data.timeline_cursor > data.sequence->getSkeletalAnimation()->length) {
            data.timeline_cursor -= data.sequence->getSkeletalAnimation()->length;
        }
        data.tl_window->setCursor(data.timeline_cursor);
    }
    data.timer_.start();
}


class GuiEditorWindow;
static std::map<std::string, GuiEditorWindow*> s_active_editors;
class GuiEditorWindow : public GuiWindow {
    std::string file_path;
public:

    GuiEditorWindow(const char* title)
    : GuiWindow(title) {}
    ~GuiEditorWindow() {
        s_active_editors.erase(file_path);
    }

    const std::string& getFilePath() const {
        return file_path;
    }
    virtual bool loadFile(const std::string& spath) {
        file_path = spath;
        return true;
    }
};


#include "gui/elements/viewport/gui_viewport.hpp"
#include "gui/elements/viewport/tools/gui_viewport_tool_csg_object_mode.hpp"
#include "gui/elements/viewport/tools/gui_viewport_tool_csg_face_mode.hpp"
#include "gui/elements/viewport/tools/gui_viewport_tool_transform.hpp"
#include "gui/elements/viewport/tools/gui_viewport_tool_csg_create_box.hpp"
#include "gui/elements/viewport/tools/gui_viewport_tool_csg_cut.hpp"

std::set<GameRenderInstance*> game_render_instances;

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

        content_padding = gfxm::rect(1, 1, 1, 1);
        addChild(&viewport);

        gpuGetPipeline()->initRenderTarget(&render_target);
        
        render_instance.render_bucket = &render_bucket;
        render_instance.render_target = &render_target;
        render_instance.view_transform = gfxm::mat4(1.0f);
        game_render_instances.insert(&render_instance);

        viewport.render_instance = &render_instance;

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
    GuiHitResult onHitTest(int x, int y) override {
        return GuiWindow::onHitTest(x, y);
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
        content_padding = gfxm::rect(0, 0, 0, 0);

        dock_space.setDockGroup(this);
        csg_viewport.setDockGroup(this);

        addChild(&dock_space);
        csg_viewport.setOwner(this);
        dock_space.getRoot()->addWindow(&csg_viewport);
    }
};

static void gpuDrawTextureToDefaultFrameBuffer(gpuTexture2d* texture) {
    int screen_w = 0, screen_h = 0;
    platformGetWindowSize(screen_w, screen_h);

    const char* vs = R"(
        #version 450 
        in vec3 inPosition;
        out vec2 uv_frag;
        
        void main(){
            uv_frag = vec2((inPosition.x + 1.0) / 2.0, (inPosition.y + 1.0) / 2.0);
            vec4 pos = vec4(inPosition, 1);
            gl_Position = pos;
        })";
    const char* fs = R"(
        #version 450
        in vec2 uv_frag;
        out vec4 outAlbedo;
        uniform sampler2D texAlbedo;
        void main(){
            vec4 pix = texture(texAlbedo, uv_frag);
            float a = pix.a;
            outAlbedo = vec4(pix.rgb, 1);
        })";
    gpuShaderProgram prog(vs, fs);
    float vertices[] = {
        -1.0f, -1.0f, 0.0f,     3.0f, -1.0f, 0.0f,      -1.0f, 3.0f, 0.0f
    };

    GLuint gvao;
    glGenVertexArrays(1, &gvao);
    glBindVertexArray(gvao);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        3, GL_FLOAT, GL_FALSE,
        0, (void*)0 /* offset */
    );

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0, 0, screen_w, screen_h);
    glScissor(0, 0, screen_w, screen_h);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture->getId());
    glUseProgram(prog.getId());
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glUseProgram(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &gvao);
}

enum EXTENSION {
    EXT_UNKNOWN,
    EXT_PNG, EXT_JPG, EXT_TGA,
    EXT_FBX, EXT_OBJ, EXT_DAE, EXT_3DS
};

EXTENSION getExtensionId(const std::string& ext) {
    static const std::unordered_map<std::string, EXTENSION> string_to_extension = {
        { ".png", EXT_PNG }, { ".jpg", EXT_JPG }, { ".jpeg", EXT_JPG }, { ".tga", EXT_TGA },
        { ".fbx", EXT_FBX }, { ".obj", EXT_OBJ }, { ".dae", EXT_DAE }, { ".3ds", EXT_3DS }
    };

    auto it = string_to_extension.find(ext);
    if (it == string_to_extension.end()) {
        return EXT_UNKNOWN;
    }
    return it->second;
}
const std::list<type>& getImporterTypesByExtension(EXTENSION ext) {
    static const std::list<type> empty;
    static const std::unordered_map<EXTENSION, std::list<type>> ext_to_types = {
        { EXT_PNG, { type_get<gpuTexture2dImporter>() } },
        { EXT_JPG, { type_get<gpuTexture2dImporter>() } },
        { EXT_TGA, { type_get<gpuTexture2dImporter>() } },

        { EXT_FBX, { type_get<ModelImporter>() } }
    };
    
    auto it = ext_to_types.find(ext);
    if (it == ext_to_types.end()) {
        return empty;
    }
    return it->second;
}

class GuiImportWnd : public GuiWindow {
    Importer* importer = 0;
public:
    GuiImportWnd(type importer_type, const std::experimental::filesystem::path& src_path)
    : GuiWindow("Import") {
        addFlags(GUI_FLAG_BLOCKING);
        setSize(400, 600);
        setPosition(800, 200);

        importer = (Importer*)importer_type.construct_new();
        assert(importer);
        if (!importer) {
            return;
        }
        importer->source_path = src_path.wstring();

        auto desc = importer_type.get_desc();
        
        // TODO: Derived properties should also be visible
        for (auto& prop : desc->properties) {
            if (prop.t == type_get<int>()) {
                int value = prop.getValue<int>(importer);
                auto elem = new GuiInput(prop.name.c_str(), GUI_INPUT_INT);
                elem->setInt(value);
                addChild(elem);
            } else if(prop.t == type_get<std::string>()) {
                std::string value = prop.getValue<std::string>(importer);
                auto elem = new GuiInput(prop.name.c_str(), GUI_INPUT_TEXT);
                elem->setText(value.c_str());
                addChild(elem);
            }
        }
        addChild(new GuiButton("Import"));
        auto cancel_btn = new GuiButton("Cancel");
        cancel_btn->addFlags(GUI_FLAG_SAME_LINE);
        addChild(cancel_btn);
        /*
        GuiInputTextLine* inptext_source = new GuiInputTextLine();
        inptext_source->setText("example/source/file.path");
        addChild(inptext_source);
        
        addChild(new GuiCheckBox("Import option A"));
        addChild(new GuiCheckBox("Import option B"));

        addChild(new GuiButton("Confirm"));
        addChild(new GuiButton("Cancel"));*/
    }
    ~GuiImportWnd() {
        if (importer) {
            importer->get_type().destruct_delete(importer);
            importer = 0;
        }
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::NOTIFY: {
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::BUTTON_CLICKED:
                std::wstring import_file_name = importer->source_path;
                size_t offset = import_file_name.find_first_of(L'\0', 0);
                if (std::string::npos != offset) {
                    import_file_name.resize(offset);
                }
                import_file_name = import_file_name + std::wstring(L".import");
                HANDLE f = CreateFileW(import_file_name.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
                if (f != INVALID_HANDLE_VALUE) {
                    nlohmann::json json = {};
                    importer->get_type().serialize_json(json, importer);
                    std::string buf = json.dump(2);
                    DWORD written = 0;
                    if (!WriteFile(f, buf.c_str(), buf.size(), &written, 0)) {
                        LOG_ERR("Failed to write import file");
                    }
                    CloseHandle(f);
                } else {
                    // TODO: Report an error to the user
                }
                return true;
            }
            return false;
        }
        }
        return GuiWindow::onMessage(msg, params);
    }
};


class GuiImportFbxWnd : public GuiWindow {
public:
    GuiImportFbxWnd(const std::experimental::filesystem::path& src_path)
    : GuiWindow("Import FBX Draft window") {
        addFlags(GUI_FLAG_BLOCKING);
        setSize(600, 800);
        setPosition(800, 200);

        addChild(new GuiButton("Import"));
        auto cancel_btn = new GuiButton("Cancel");
        cancel_btn->addFlags(GUI_FLAG_SAME_LINE);
        addChild(cancel_btn);

        addChild(new GuiInputFilePath("Source", "./MyModel.fbx", "fbx"));
        addChild(new GuiInputFilePath("Import file", "./MyModel.fbx.import", "fbx.import"));
        
        auto model = new GuiCollapsingHeader("Model");
        addChild(model);
        model->addChild(new GuiCheckBox("Import model"));
        model->addChild(new GuiInputFilePath("Output file", "./MyModel.skeletal_model", "skeletal_model"));

        auto skeleton = new GuiCollapsingHeader("Skeleton");
        addChild(skeleton);
        skeleton->addChild(new GuiCheckBox("Import skeleton"));
        skeleton->addChild(new GuiInputFilePath("Skeleton path", "./MyModel.skeleton", "skeleton"));

        auto animations = new GuiCollapsingHeader("Animation");
        addChild(animations);
        animations->addChild(new GuiCheckBox("Import animations"));
        {
            auto anim = new GuiCollapsingHeader("Idle (Idle)", false, false);
            animations->addChild(anim);
            anim->addChild(new GuiInputFilePath("Output file", "./Idle.animation", "animation"));
            anim->addChild(new GuiInputNumeric("Range", GUI_INPUT_NUMBER_TYPE::INT32, 2));
            anim->addChild(new GuiCheckBox("Root motion"));
            anim->addChild(new GuiComboBox("Reference bone"));
            animations->addChild(new GuiCollapsingHeader("Walk (Walk)", false, false));
            animations->addChild(new GuiCollapsingHeader("Run (Run)", false, false));
            animations->addChild(new GuiCollapsingHeader("Action (Action)", false, false));
        }

        auto materials = new GuiCollapsingHeader("Materials");
        addChild(materials);
        materials->addChild(new GuiCheckBox("Import materials"));
        materials->addChild(new GuiCollapsingHeader("DefaultMaterial"));
        
        auto meshes = new GuiCollapsingHeader("Meshes");
        addChild(meshes);
        meshes->addChild(new GuiCheckBox("Import meshes"));
        meshes->addChild(new GuiCollapsingHeader("Body"));
        meshes->addChild(new GuiCollapsingHeader("Head"));
        meshes->addChild(new GuiCollapsingHeader("Weapon"));


    }
    ~GuiImportFbxWnd() {}
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::NOTIFY: {
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::BUTTON_CLICKED:/*
                std::wstring import_file_name = importer->source_path;
                size_t offset = import_file_name.find_first_of(L'\0', 0);
                if (std::string::npos != offset) {
                    import_file_name.resize(offset);
                }
                import_file_name = import_file_name + std::wstring(L".import");
                HANDLE f = CreateFileW(import_file_name.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
                if (f != INVALID_HANDLE_VALUE) {
                    nlohmann::json json = {};
                    importer->get_type().serialize_json(json, importer);
                    std::string buf = json.dump(2);
                    DWORD written = 0;
                    if (!WriteFile(f, buf.c_str(), buf.size(), &written, 0)) {
                        LOG_ERR("Failed to write import file");
                    }
                    CloseHandle(f);
                } else {
                    // TODO: Report an error to the user
                }*/
                return true;
            }
            return false;
        }
        }
        return GuiWindow::onMessage(msg, params);
    }
};


class GuiSequenceDocument : public GuiEditorWindow {
    SequenceEditorData seqed_data;
    SequenceEditorProject seq_ed_proj;

    GuiDockSpace dock_space;
    GuiViewport viewport;
    GuiWindow vp_wnd;
    GuiTimelineWindow timeline;
    GuiTimelineItemInspectorWindow timeline_inspector;
    EditorGuiSequenceResourceList resource_list;

    gpuRenderTarget render_target;
    gpuRenderBucket render_bucket;
    GameRenderInstance render_instance;
    gameActor actor;
    RHSHARED<animSequence> seq_run;

    RHSHARED<gpuMaterial> material_color;
    gpuMesh gpu_mesh_plane;
    std::unique_ptr<gpuRenderable> renderable_plane;
    gpuUniformBuffer* renderable_plane_ubuf;
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
        resource_list.setDockGroup(this);

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
        dock_space.getRoot()->left->right->addWindow(&resource_list);

        {
            seq_run.reset_acquire();
            seq_run->setSkeletalAnimation(resGet<Animation>("models/chara_24/Run.animation"));
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

class GuiLayoutTestWindow : public GuiWindow {
    struct RECT {
        uint32_t color;
        gfxm::vec2 pos;
        gfxm::vec2 size;

        gfxm::vec2 current_size;

        void layout(const gfxm::vec2 available_size) {
            //current_size = size;
            current_size = gfxm::vec2(
                size.x == .0f ? available_size.x : size.x,
                size.y == .0f ? available_size.y : size.y
            );
        }
    };
    static const int RECT_COUNT = 5;
    RECT rects[RECT_COUNT] = {
        RECT{
            GUI_COL_BG_DARK,
            gfxm::vec2(0, 0),
            gfxm::vec2(0, 250)
        },
        RECT{
            GUI_COL_RED,
            gfxm::vec2(150, 0),
            gfxm::vec2(200, 50)
        },
        RECT{
            GUI_COL_GREEN,
            gfxm::vec2(160, 200),
            gfxm::vec2(150, 25)
        },
        RECT{
            0xFFFFAAAA,
            gfxm::vec2(200, 160),
            gfxm::vec2(200, 50)
        },
        RECT{
            GUI_COL_ACCENT,
            gfxm::vec2(300, 300),
            gfxm::vec2(150, 25)
        }
    };
public:
    GuiTextBuffer text;
    GuiLayoutTestWindow()
        : GuiWindow("LayoutTest"), text(guiGetDefaultFont()) {
    }
    GuiHitResult onHitTest(int x, int y) override {
        for (int i = 0; i < RECT_COUNT; ++i) {
            // TODO: 
        }
        return GuiWindow::onHitTest(x, y);
    }
    void onDraw() {
        gfxm::vec2 containing_size = client_area.max - client_area.min;

        int line_elem_idx = 0;
        float line_x = .0f;
        float line_y = .0f;
        float line_height = .0f;
        for (int i = 0; i < RECT_COUNT; ++i) {
            auto& rect = rects[i];
            gfxm::vec2 available_size(containing_size.x - line_x, containing_size.y - line_y);
            rect.layout(available_size);
            gfxm::vec2 pos = gfxm::vec2(line_x, line_y);
            gfxm::vec2 size = rect.current_size;
            uint32_t color = rect.color;

            if (pos.x + size.x >= containing_size.x && line_elem_idx > 0) {
                line_y += line_height;
                line_x = .0f;
                line_height = .0f;
                line_elem_idx = 0;
                --i;
                continue;
            }

            gfxm::rect rc(
                client_area.min + pos,
                client_area.min + pos + size
            );
            guiDrawRectRound(rc, 10.f, color);

            line_x += size.x;
            line_height = gfxm::_max(line_height, size.y);
            ++line_elem_idx;
        }

        text.replaceAll("Hello", strlen("Hello"));
        text.prepareDraw(guiGetCurrentFont(), true);
        text.draw(client_area.min, GUI_COL_TEXT, GUI_COL_ACCENT);
    }
};

void guiCenterWindowToParent(GuiWindow* wnd) {
    auto parent = wnd->getParent();
    assert(parent);
    if (!parent) {
        return;
    }
    gfxm::rect rc = parent->getClientArea();
    gfxm::vec2 pos = (rc.min + rc.max) * .5f;
    pos -= wnd->size * .5f;
    wnd->setPosition(pos);
}

GuiWindow* tryOpenEditWindow(const std::string& ext, const std::string& spath) {
    auto it = s_active_editors.find(spath);
    if (it != s_active_editors.end()) {
        guiBringWindowToTop(it->second);
        guiSetActiveWindow(it->second);
        return it->second;
    }
    // TODO:
    GuiEditorWindow* wnd = 0;
    if (ext == ".png") {
        wnd = dynamic_cast<GuiEditorWindow*>(guiCreateWindow<GuiCsgDocument>());
    }

    assert(wnd);
    if (!wnd) {
        return 0;
    }
    wnd->loadFile(spath);

    s_active_editors[spath] = wnd;

    guiSetActiveWindow(wnd);
    return wnd;
}
GuiWindow* tryOpenImportWindow(const std::string& ext, const std::string& spath) {
    // TODO:
    return 0;
}

bool messageCb(GUI_MSG msg, GUI_MSG_PARAMS params) {
    //LOG(guiMsgToString(msg));
    switch (msg) {
    case GUI_MSG::FILE_EXPL_OPEN_FILE: {
        std::string spath = params.getA<GuiFileListItem*>()->path_canonical;
        LOG_WARN(spath);
        std::experimental::filesystem::path fpath(spath);
        if (!fpath.has_extension()) {
            // TODO: Show a message box with a warning, don't open file
            return true;
        }
        std::string ext = fpath.extension().string();
        if (tryOpenEditWindow(ext, fpath.string())) {
            return true;
        }
        if (tryOpenImportWindow(ext, fpath.string())) {
            return true;
        }
#if defined _WIN32
        ShellExecuteA(0, 0, fpath.string().c_str(), 0, 0, SW_SHOW);
#endif
        return true;
    }
    };
    return false;
}
bool dropFileCb(const std::experimental::filesystem::path& path) {
    std::string str = path.string();
    LOG_DBG(str);
    
    if (!path.has_extension()) {
        return false;
    }

    std::string ext_str = path.extension().string();
    LOG_DBG(ext_str);
    EXTENSION ext = getExtensionId(ext_str);
    if (ext == EXT_UNKNOWN) {
        return false;
    }

    const auto& importer_types = getImporterTypesByExtension(ext);
    if (importer_types.empty()) {
        return false;
    }
    // TODO: Show importer selection window
    // picking first one for now
    type importer_type = *importer_types.begin();

    auto wnd = new GuiImportWnd(importer_type, path);
    guiAddManagedWindow(wnd);
    wnd->setTitle(importer_type.get_name().c_str());
    guiGetRoot()->addChild(wnd);
    guiGetRoot()->bringToTop(wnd);
    guiCenterWindowToParent(wnd);
    return true;
}


#include "audio/audio_mixer.hpp"
#include "audio/res_cache_audio_clip.hpp"
inline void audioInit() {
    resAddCache<AudioClip>(new resCacheAudioClip);
    audio().init(44100, 16);
}
inline void audioCleanup() {
    audio().cleanup();
}


#include "gui_cdt_test_window.hpp"
int main(int argc, char* argv) {
    reflectInit();
    importInit();
    platformInit(true, true);
    gpuInit(new build_config::gpuPipelineCommon());
    typefaceInit();
    Font* fnt = fontGet("ProggyClean.ttf", 16, 72);
    guiInit(fnt);
    guiSetMessageCallback(&messageCb);
    guiSetDropFileCallback(&dropFileCb);

    resInit();
    animInit();
    audioInit();

    std::unique_ptr<GuiDockSpace> gui_root;
    gui_root.reset(new GuiDockSpace);
    int screen_width = 0, screen_height = 0;
    platformGetWindowSize(screen_width, screen_height);

    gui_root.reset(new GuiDockSpace());
    gui_root->pos = gfxm::vec2(0.0f, 0.0f);
    gui_root->size = gfxm::vec2(screen_width, screen_height);

    auto wnd = new GuiWindow("1 Test window");
    wnd->pos = gfxm::vec2(120, 160);
    wnd->size = gfxm::vec2(640, 700);
    wnd->addChild(new GuiTextBox());
    wnd->addChild(new GuiImage(resGet<gpuTexture2d>("1648920106773.jpg").get()));
    wnd->addChild(new GuiButton());
    wnd->addChild(new GuiButton());
    auto wnd2 = new GuiWindow("2 Other test window");
    wnd2->pos = gfxm::vec2(850, 200);
    wnd2->size = gfxm::vec2(320, 800);
    wnd2->addChild(new GuiImage(resGet<gpuTexture2d>("effect_004.png").get()));
    auto wnd_demo = new GuiDemoWindow();
    auto wnd_explorer = new GuiFileExplorerWindow();
    auto wnd_nodes = new GuiNodeEditorWindow();
    auto wnd_cdt = new GuiCdtTestWindow();
    auto wnd_seq = guiCreateWindow<GuiSequenceDocument>();
    auto wnd_csg2 = new GuiCsgDocument;

    auto wnd_imp = new GuiImportFbxWnd("2b.fbx");
    guiAddManagedWindow(wnd_imp);

    auto wnd_layout = new GuiLayoutTestWindow();
    guiAddManagedWindow(wnd_layout);

    guiGetRoot()->createMenuBar()
        ->addItem(new GuiMenuItem("File", {
                new GuiMenuListItem("New", {
                    new GuiMenuListItem("Sequence"),
                    new GuiMenuListItem("Shmequence")
                }),
                new GuiMenuListItem("Open..."),
                new GuiMenuListItem("Save"),
                new GuiMenuListItem("Save As..."),
                new GuiMenuListItem("Exit")
        }))
        ->addItem(new GuiMenuItem("Edit"))
        ->addItem(new GuiMenuItem("View"))
        ->addItem(new GuiMenuItem("Settings"));

    gui_root->getRoot()->addWindow(wnd);
    gui_root->getRoot()->addWindow(wnd2);
    gui_root->getRoot()->addWindow(wnd_nodes);
    gui_root->getRoot()->addWindow(wnd_cdt);
    gui_root->getRoot()->addWindow(wnd_seq);
    gui_root->getRoot()->addWindow(wnd_csg2);
    gui_root->getRoot()->splitLeft();
    gui_root->getRoot()->left->addWindow(wnd_demo);
    gui_root->getRoot()->left->addWindow(wnd_explorer);
    gui_root->getRoot()->split_pos = 0.20f;
    gui_root->getRoot()->right->split_pos = 0.3f;
    
    //guiBringWindowToTop(wnd_csg);

    gpuUniformBuffer* ubufCam3d = gpuGetPipeline()->createUniformBuffer(UNIFORM_BUFFER_CAMERA_3D);
    gpuGetPipeline()->attachUniformBuffer(ubufCam3d);    
    

    timer timer_;
    while (platformIsRunning()) {
        timer_.start();
        platformPollMessages();
        inputUpdate(g_dt);
        
        gpuFrameBufferUnbind();
        guiLayout();
        guiDraw();

        // Process and render world instances
        for(auto& inst : game_render_instances) {
            inst->world.update(g_dt);
            
            //render_bucket.add(renderable_plane.get());
            inst->world.getRenderScene()->draw(inst->render_bucket);
            ubufCam3d->setMat4(
                gpuGetPipeline()->getUniformBufferDesc(UNIFORM_BUFFER_CAMERA_3D)->getUniform("matProjection"),
                inst->projection
            );
            ubufCam3d->setMat4(
                gpuGetPipeline()->getUniformBufferDesc(UNIFORM_BUFFER_CAMERA_3D)->getUniform("matView"),
                inst->view_transform
            );
            gpuDraw(
                inst->render_bucket, inst->render_target,
                inst->view_transform,
                inst->projection
            );
            
            inst->render_target->bindFrameBuffer("Normal", 0);
            dbgDrawDraw(
                inst->projection,
                inst->view_transform
            );
            inst->render_bucket->clear();
        }
        dbgDrawClearBuffers();

        guiRender();

        //gpuDrawTextureToDefaultFrameBuffer(gpuGetPipeline()->tex_albedo.get());

        platformSwapBuffers();
        g_dt = timer_.stop();
    }

    audioCleanup();
    resCleanup();
    animCleanup();

    gui_root.reset();
    guiCleanup();
    typefaceCleanup();
    gpuCleanup();
    platformCleanup();
    return 0;
}