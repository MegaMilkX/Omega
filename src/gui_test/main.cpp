#include <Windows.h>

#include <assert.h>

#include <string>

#include "platform/gl/glextutil.h"
#include "log/log.hpp"
#include "util/timer.hpp"

#include <unordered_map>
#include <memory>

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
    RHSHARED<animAnimatorSequence> sequence;
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
        : GuiWindow("TimelineWindow"), data(data) {
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

    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
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
                return;
                break;
            case GUI_NOTIFY::TIMELINE_EVENT_ADDED: {
                auto gui_trk = params.getB<GuiTimelineEventTrack*>();
                auto gui_item = params.getC<GuiTimelineEventItem*>();
                gui_item->user_ptr = project->eventAdd((SeqEdEventTrack*)gui_trk->user_ptr, gui_item->frame);
                break;
            }
            case GUI_NOTIFY::TIMELINE_EVENT_REMOVED: {
                auto gui_trk = params.getB<GuiTimelineEventTrack*>();
                auto gui_item = params.getC<GuiTimelineEventItem*>();
                if (on_item_destroyed) {
                    on_item_destroyed((SeqEdEvent*)gui_item->user_ptr);
                }
                project->eventRemove((SeqEdEventTrack*)gui_trk->user_ptr, (SeqEdEvent*)gui_item->user_ptr);
                break;
            }
            case GUI_NOTIFY::TIMELINE_EVENT_MOVED: {
                auto gui_trk = params.getB<GuiTimelineEventTrack*>();
                auto gui_item = params.getC<GuiTimelineEventItem*>();
                project->eventMove((SeqEdEvent*)gui_item->user_ptr, (SeqEdEventTrack*)gui_trk->user_ptr, gui_item->frame);
                break;
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
                break;
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
                break;
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
                break;
            }
            case GUI_NOTIFY::TIMELINE_EVENT_SELECTED: {
                auto gui_item = params.getC<GuiTimelineEventItem*>();
                if (on_event_selected) {
                    on_event_selected((SeqEdEvent*)gui_item->user_ptr);
                }
                break;
            }
            case GUI_NOTIFY::TIMELINE_BLOCK_SELECTED: {
                auto gui_item = params.getC<GuiTimelineBlockItem*>();
                if (gui_item->type == SEQ_ED_TRACK_HITBOX) {
                    if (on_hitbox_selected) {
                        on_hitbox_selected((SeqEdHitbox*)gui_item->user_ptr);
                    }
                }
                break;
            }
                
            }
            break;
        }

        GuiWindow::onMessage(msg, params);
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
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::NOTIFY:
            break;
        }
    }

    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = rc;
        client_area = rc;
        gfxm::vec2 cur = client_area.min;
        gfxm::rect rc_ = client_area;
        for (auto& ch : children) {
            ch->layout(cur, rc_, flags);
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
    GuiInputFloat3 translation;
    GuiInputFloat3 rotation;
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
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = rc;
        client_area = rc;
        gfxm::vec2 cur = client_area.min;
        gfxm::rect rc_ = client_area;
        for (auto& ch : children) {
            ch->layout(cur, rc_, flags);
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
        : GuiWindow("Timeline Item Inspector") {
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

    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
        GuiWindow::onMessage(msg, params);
    }
};

class EditorGuiSequenceResourceList : public GuiWindow {
    std::unique_ptr<GuiContainer> container;
public:
    EditorGuiSequenceResourceList()
    : GuiWindow("Sequence resources") {
        setSize(400, 500);
        setMinSize(200, 250);
        container.reset(new GuiContainer);
        container->setOwner(this);
        addChild(container.get());
    }
};

void sequenceEditorInit(
    SequenceEditorData& data,
    RHSHARED<sklSkeletonMaster> skl,
    RHSHARED<animAnimatorSequence> sequence,
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

struct GameRenderInstance {
    gameWorld world;
    gpuRenderTarget* render_target;
    gpuRenderBucket* render_bucket;
    gfxm::vec2 viewport_size;
    gfxm::mat4 view_transform;
};
std::vector<GameRenderInstance*> game_render_instances;

class GuiGizmoTransform3d : public GuiElement {
    int axis_id_hovered = 0;
    float dxz = .0f;
    float dyz = .0f;
    float dzz = .0f;
    bool is_dragging = false;
    gfxm::vec2 last_mouse_pos;
public:
    gfxm::mat4 view = gfxm::mat4(1.f);
    gfxm::mat4 projection = gfxm::mat4(1.f);
    gfxm::mat4 model = gfxm::mat4(1.f);

    GuiHitResult hitTest(int x, int y) override {
        if (!is_dragging) {
            gfxm::mat4 m = projection * view * model;
            float dx = guiHitTestLine3d(gfxm::vec3(.0f, .0f, .0f), gfxm::vec3(1.f, .0f, .0f), gfxm::vec2(x, y) - client_area.min, client_area, m, dxz);
            float dy = guiHitTestLine3d(gfxm::vec3(.0f, .0f, .0f), gfxm::vec3(.0f, 1.f, .0f), gfxm::vec2(x, y) - client_area.min, client_area, m, dyz);
            float dz = guiHitTestLine3d(gfxm::vec3(.0f, .0f, .0f), gfxm::vec3(.0f, .0f, 1.f), gfxm::vec2(x, y) - client_area.min, client_area, m, dzz);
            int min_id = 0;
            float min_dist = FLT_MAX;
            float min_z = FLT_MAX;
            axis_id_hovered = 0;
            if (dx < 20.f && (dx < min_dist && dxz < min_z)) {
                min_dist = dx;
                min_z = dxz;
                axis_id_hovered = 1;
            }
            if (dy < 20.f && (dy < min_dist && dyz < min_z)) {
                min_dist = dy;
                min_z = dyz;
                axis_id_hovered = 2;
            }
            if (dz < 20.f && (dz < min_dist && dzz < min_z)) {
                min_dist = dz;
                min_z = dzz;
                axis_id_hovered = 3;
            }
            if (axis_id_hovered == 0) {
                return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
            }
        } else {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }

        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::MOUSE_MOVE: {
            gfxm::vec2 mouse_pos = gfxm::vec2(params.getA<int32_t>(), params.getB<int32_t>()) - client_area.min;
            gfxm::mat4 transform = projection * view * model;
            gfxm::vec2 vpsz(client_area.max.x - client_area.min.x, client_area.max.y - client_area.min.y);
            if (is_dragging) {
                gfxm::ray R = gfxm::ray_viewport_to_world(
                    vpsz, gfxm::vec2(mouse_pos.x, vpsz.y - mouse_pos.y),
                    projection, view
                );

                gfxm::vec3 Np(.0f, 1.f, .0f);
                gfxm::vec3 Pp(.0f, .0f, .0f);
                gfxm::vec3 Pi;
                float denom = gfxm::dot(Np, R.direction);
                if (fabsf(denom) > FLT_EPSILON) {
                    gfxm::vec3 p = Pp - R.origin;
                    float t = gfxm::dot(Np, p) / denom;
                    Pi = R.origin + R.direction * t;
                }

                gfxm::vec2 diff = mouse_pos - last_mouse_pos;
                switch (axis_id_hovered) {
                case 1: {
                    model = gfxm::translate(gfxm::mat4(1.f), Pi);
                    }break;
                case 2:
                    break;
                case 3:
                    break;
                }
            }
            last_mouse_pos = mouse_pos;
        } break;
        case GUI_MSG::LBUTTON_DOWN:
            is_dragging = true;
            guiCaptureMouse(this);
            break;
        case GUI_MSG::LBUTTON_UP:
            is_dragging = false;
            guiCaptureMouse(0);
            break;
        }
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = rc;
        client_area = bounding_rect;
    }
    
    void onDraw() override {
        guiPushViewportRect(client_area);
        guiPushProjection(projection);

        uint32_t col_x = 0xFF0000FF;
        uint32_t col_y = 0xFF00FF00;
        uint32_t col_z = 0xFFFF0000;
        if (axis_id_hovered == 1) { col_x = GUI_COL_TIMELINE_CURSOR; }
        if (axis_id_hovered == 2) { col_y = GUI_COL_TIMELINE_CURSOR; }
        if (axis_id_hovered == 3) { col_z = GUI_COL_TIMELINE_CURSOR; }

        // Translator
        guiDrawLine3(gfxm::vec3(.0f, .0f, .0f), gfxm::vec3(1.f, .0f, .0f), col_x)
            .model_transform = view * model;
        guiDrawLine3(gfxm::vec3(.0f, .0f, .0f), gfxm::vec3(.0f, 1.f, .0f), col_y)
            .model_transform = view * model;
        guiDrawLine3(gfxm::vec3(.0f, .0f, .0f), gfxm::vec3(.0f, .0f, 1.f), col_z)
            .model_transform = view * model;
        gfxm::mat4 m
            = gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(.8f, .0f, .0f))
            * gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(-90.0f), gfxm::vec3(.0f, .0f, 1.f)));
        guiDrawCone(.05f, .2f, col_x)
            .model_transform = view * model * m;
        m = gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(.0f, .8f, .0f));
        guiDrawCone(.05f, .2f, col_y)
            .model_transform = view * model * m;
        m = gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(.0f, .0f, .8f))
            * gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(90.0f), gfxm::vec3(1.f, .0f, .0f)));
        guiDrawCone(.05f, .2f, col_z)
            .model_transform = view * model * m;

        // Rotator
        guiDrawCircle3(1.f, 0xFF0000FF)
            .model_transform = view * model * gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(-90.0f), gfxm::vec3(.0f, .0f, 1.f)));
        guiDrawCircle3(1.f, 0xFF00FF00)
            .model_transform = view * model * gfxm::mat4(1.f);
        guiDrawCircle3(1.f, 0xFFFF0000)
            .model_transform = view * model * gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(90.0f), gfxm::vec3(1.f, .0f, .0f)));

        guiPopProjection();
        guiPopViewportRect();
    }
};
class GuiViewport : public GuiElement {
    std::unique_ptr<GuiGizmoTransform3d> gizmo_transform;
public:
    gfxm::vec2 last_mouse_pos;
    float cam_angle_y = .0f;
    float cam_angle_x = .0f;
    bool dragging = false;

    GameRenderInstance* render_instance;

    GuiViewport() {
        gizmo_transform.reset(new GuiGizmoTransform3d);
        gizmo_transform->setOwner(this);
        addChild(gizmo_transform.get());
    }

    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::MOUSE_MOVE: {
            gfxm::vec2 mouse_pos = gfxm::vec2(params.getA<int32_t>(), params.getB<int32_t>());
            if (dragging) {
                cam_angle_x -= mouse_pos.y - last_mouse_pos.y;
                cam_angle_y -= mouse_pos.x - last_mouse_pos.x;
            }
            last_mouse_pos = mouse_pos;
            } break;
        case GUI_MSG::LBUTTON_DOWN:
            dragging = true;
            guiCaptureMouse(this);
            break;
        case GUI_MSG::LBUTTON_UP:
            dragging = false;
            guiCaptureMouse(0);
            break;
        }
    }

    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }

        if (gizmo_transform) {
            GuiHitResult hit = gizmo_transform->hitTest(x, y);
            if (hit.hasHit()) {
                return hit;
            }
        }

        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = rc;
        client_area = bounding_rect;
        render_instance->viewport_size = gfxm::vec2(
            client_area.max - client_area.min
        );
        gfxm::quat qx = gfxm::angle_axis(gfxm::radian(cam_angle_x), gfxm::vec3(1, 0, 0));
        gfxm::quat qy = gfxm::angle_axis(gfxm::radian(cam_angle_y), gfxm::vec3(0, 1, 0));
        gfxm::quat q = qy * qx;
        gfxm::mat4 m = gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(0, 1, 0)) * gfxm::to_mat4(q);
        m = gfxm::translate(m, gfxm::vec3(0,0,1) * 2.0f);
        render_instance->view_transform = gfxm::inverse(m);

        if (gizmo_transform) {
            gizmo_transform->view = render_instance->view_transform;
            gizmo_transform->projection = gfxm::perspective(gfxm::radian(65.0f), render_instance->viewport_size.x / render_instance->viewport_size.y, 0.01f, 1000.0f);
            gizmo_transform->layout(client_area.min, client_area, flags);
        }
    }
    void onDraw() override {
        guiDrawRectTextured(client_area, render_instance->render_target->textures[0].get(), GUI_COL_WHITE);
        if (gizmo_transform) {
            gizmo_transform->draw();
        }
    }
};

class GuiLayoutExperiments : public GuiWindow {
public:
    GuiLayoutExperiments()
        : GuiWindow("Gui Layout Experiments") {
        size = gfxm::vec2(800, 600);
        pos = gfxm::vec2(1000, 300);
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
    platformInit(true, true);
    gpuInit(new build_config::gpuPipelineCommon());
    typefaceInit();
    Typeface typeface_nimbusmono;
    typefaceLoad(&typeface_nimbusmono, "nimbusmono-bold.otf");
    Font* fnt = new Font(&typeface_nimbusmono, 12, 72);
    guiInit(fnt);

    resInit();
    animInit();
    audioInit();

    SequenceEditorData seqed_data;
    SequenceEditorProject seq_ed_proj;

    std::unique_ptr<GuiDockSpace> gui_root;
    gui_root.reset(new GuiDockSpace);
    int screen_width = 0, screen_height = 0;
    platformGetWindowSize(screen_width, screen_height);

    gui_root.reset(new GuiDockSpace());
    gui_root->getRoot()->splitTop();
    //gui_root.getRoot()->left->split();
    gui_root->getRoot()->right->splitLeft();
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
    auto wnd3 = new GuiWindow("Viewport");
    wnd3->content_padding = gfxm::rect(0, 0, 0, 0);
    wnd3->pos = gfxm::vec2(850, 200);
    wnd3->size = gfxm::vec2(400, 700);
    //wnd3->addChild(new GuiImage(gpuGetPipeline()->tex_albedo.get()));
    auto gui_viewport = new GuiViewport;
    wnd3->addChild(gui_viewport);
    gui_root->getRoot()->right->right->addWindow(wnd3);
    auto wnd4 = new GuiDemoWindow();
    auto wnd6 = new GuiFileExplorerWindow();
    auto wnd7 = new GuiNodeEditorWindow();
    auto wnd_experiments = new GuiLayoutExperiments();
    auto wnd_timeline = new GuiTimelineWindow(&seqed_data);
    auto wnd9 = new EditorGuiSequenceResourceList();/*
    wnd9->createMenuBar()
        ->addItem("File")
        ->addItem("Edit")
        ->addItem("View")
        ->addItem("Settings");*/
    auto wnd10 = new GuiCdtTestWindow();
    auto wnd_timeline_inspector = new GuiTimelineItemInspectorWindow();
    wnd_timeline->on_event_selected = [wnd_timeline_inspector](SeqEdEvent* e) {
        wnd_timeline_inspector->init(e);
    };
    wnd_timeline->on_hitbox_selected = [wnd_timeline_inspector](SeqEdHitbox* hb) {
        wnd_timeline_inspector->init(hb);
    };
    wnd_timeline->on_item_destroyed = [wnd_timeline_inspector](SeqEdItem* item) {
        wnd_timeline_inspector->onItemDestroyed(item);
    };

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

    gui_root->getRoot()->left->addWindow(wnd);
    gui_root->getRoot()->left->addWindow(wnd2);
    gui_root->getRoot()->left->addWindow(wnd3);
    gui_root->getRoot()->left->splitLeft();
    gui_root->getRoot()->left->left->addWindow(wnd4);
    gui_root->getRoot()->right->left->addWindow(wnd7);
    gui_root->getRoot()->right->left->addWindow(wnd6);
    gui_root->getRoot()->right->left->addWindow(wnd9);
    gui_root->getRoot()->right->right->addWindow(wnd_timeline);
    gui_root->getRoot()->split_pos = 0.7f;
    gui_root->getRoot()->left->split_pos = 0.20f;
    gui_root->getRoot()->right->split_pos = 0.3f;

    gpuRenderTarget render_target;
    gpuGetPipeline()->initRenderTarget(&render_target);
    gpuRenderBucket render_bucket(gpuGetPipeline(), 1000);
    RHSHARED<animAnimatorSequence> seq_run;
    {
        seq_run.reset_acquire();
        seq_run->setSkeletalAnimation(resGet<Animation>("models/chara_24/Run2.animation"));
    }
    gameActor actor;
    {
        auto root = actor.setRoot<nodeCharacterCapsule>("capsule");
        auto node = root->createChild<nodeSkeletalModel>("model");
        node->setModel(resGet<mdlSkeletalModelMaster>("models/chara_24/chara_24.skeletal_model"));
    }
    GameRenderInstance render_instance;
    render_instance.world.spawnActor(&actor);
    render_instance.render_bucket = &render_bucket;
    render_instance.render_target = &render_target;
    render_instance.viewport_size = gfxm::vec2(100, 100);
    render_instance.view_transform = gfxm::mat4(1.0f);
    game_render_instances.push_back(&render_instance);
    gui_viewport->render_instance = &render_instance;
    
    sequenceEditorInit(
        seqed_data,
        resGet<sklSkeletonMaster>("models/chara_24/chara_24.skeleton"),
        seq_run,
        &actor,
        wnd_timeline
    );
    wnd_timeline->init(&seq_ed_proj);

    RHSHARED<gpuMaterial> material_color = resGet<gpuMaterial>("materials/color.mat");
    Mesh3d mesh_plane;
    meshGenerateCheckerPlane(&mesh_plane, 50, 50, 50);
    gpuMesh gpu_mesh_plane;
    gpu_mesh_plane.setData(&mesh_plane);
    std::unique_ptr<gpuRenderable> renderable_plane;
    renderable_plane.reset(new gpuRenderable(material_color.get(), gpu_mesh_plane.getMeshDesc()));
    gpuUniformBuffer* renderable_plane_ubuf = gpuGetPipeline()->createUniformBuffer(UNIFORM_BUFFER_MODEL);
    renderable_plane->attachUniformBuffer(renderable_plane_ubuf);
    renderable_plane_ubuf->setMat4(
        gpuGetPipeline()->getUniformBufferDesc(UNIFORM_BUFFER_MODEL)->getUniform("matModel"),
        gfxm::mat4(1.f)
    );

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

        sequenceEditorUpdateAnimFrame(seq_ed_proj, seqed_data);
        // Process and render world instances
        for (int i = 0; i < game_render_instances.size(); ++i) {
            auto& inst = game_render_instances[i];
            inst->render_target->setSize(inst->viewport_size.x, inst->viewport_size.y);
            inst->world.update(g_dt);
            inst->render_bucket->clear();
            inst->render_bucket->add(renderable_plane.get());
            inst->world.getRenderScene()->draw(&render_bucket);
            ubufCam3d->setMat4(
                gpuGetPipeline()->getUniformBufferDesc(UNIFORM_BUFFER_CAMERA_3D)->getUniform("matProjection"),
                gfxm::perspective(gfxm::radian(65.0f), inst->viewport_size.x / inst->viewport_size.y, 0.01f, 1000.0f)
            );
            ubufCam3d->setMat4(
                gpuGetPipeline()->getUniformBufferDesc(UNIFORM_BUFFER_CAMERA_3D)->getUniform("matView"),
                inst->view_transform
            );
            gpuDraw(inst->render_bucket, inst->render_target);
            
            inst->render_target->bindFrameBuffer("Normal", 0);
            dbgDrawDraw(
                gfxm::perspective(gfxm::radian(65.0f), inst->viewport_size.x / inst->viewport_size.y, 0.01f, 1000.0f),
                inst->view_transform
            );
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