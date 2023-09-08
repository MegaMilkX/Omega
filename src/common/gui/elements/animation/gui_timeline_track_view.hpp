#pragma once

#include "gui/elements/element.hpp"
#include "gui/elements/animation/gui_timeline_keyframe_track.hpp"
#include "gui/elements/animation/gui_timeline_event_track.hpp"
#include "gui/elements/animation/gui_timeline_block_track.hpp"
#include "gui/elements/animation/gui_timeline_utils.hpp"


class GuiTimelineTrackView : public GuiElement {
    std::vector<std::unique_ptr<GuiTimelineTrackBase>> tracks;
    int cursor_frame = 0;
    bool is_pressed = false;
    bool is_panning = false;
    gfxm::vec2 last_mouse_pos = gfxm::vec2(0, 0);
    float frame_screen_width = 20.0f;
    int primary_divider = 5;
    int skip_divider = 1;
    
    int getFrameAtScreenPos(float x, float y) {
        return ((x + content_offset.x - 10.0f + (frame_screen_width * .5f)) / frame_screen_width);
    }
public:
    gfxm::vec2 content_offset = gfxm::vec2(0, 0);

    float track_height = 20.0f;
    float track_margin = 1.0f;
    
    void setCursor(int frame, bool send_notification = true) {
        cursor_frame = frame;
        cursor_frame = std::max(0, cursor_frame);
        assert(getOwner());
        if (getOwner() && send_notification) {
            getOwner()->sendMessage(GUI_MSG::NOTIFY, (uint64_t)GUI_NOTIFY::TIMELINE_JUMP, cursor_frame);
        }
    }
    int getCursor() const {
        return cursor_frame;
    }
    void setContentOffset(float x, float y) {
        //x = gfxm::_max(.0f, x);
        content_offset.x = x;
        content_offset.y = y;
        for (auto& t : tracks) {
            t->content_offset_x = x;
        }
        if (getOwner()) {
            getOwner()->notify(GUI_NOTIFY::TIMELINE_PAN_X, content_offset.x);
            getOwner()->notify(GUI_NOTIFY::TIMELINE_PAN_Y, content_offset.y);
        }
    }
    void setFrameScale(float x) {
        float fsw_old = frame_screen_width;
        frame_screen_width = x;
        frame_screen_width = gfxm::_max(1.0f, frame_screen_width);
        frame_screen_width = (int)gfxm::_min(20.0f, frame_screen_width);
        float diff = frame_screen_width - fsw_old;
        if (fsw_old != frame_screen_width) {
            if (diff > .0f) {
                content_offset.x += (guiGetMousePos().x - client_area.min.x - 10.f) * (1.f / frame_screen_width);
            } else if (diff < .0f) {
                content_offset.x -= (guiGetMousePos().x - client_area.min.x - 10.f) * (1.f / frame_screen_width);
            }
            content_offset.x *= (frame_screen_width / fsw_old);
            setContentOffset(content_offset.x, content_offset.y);
        }
        for (auto& t : tracks) {
            t->frame_screen_width = frame_screen_width;
        }
        int mid;
        guiTimelineCalcDividers(frame_screen_width, primary_divider, mid, skip_divider);
        assert(getOwner());
        if (getOwner()) {
            getOwner()->sendMessage(GUI_MSG::NOTIFY, (uint64_t)GUI_NOTIFY::TIMELINE_ZOOM, frame_screen_width);
        }
    }
    GuiTimelineTrackView() {}

    GuiTimelineKeyframeTrack* findKeyframeTrack(const std::string& name) {
        for (auto& t : tracks) {
            if (t->getName() == name) {
                auto track = dynamic_cast<GuiTimelineKeyframeTrack*>(t.get());
                if (track) {
                    return track;
                }
            }
        }
        return 0;
    }

    GuiTimelineKeyframeTrack* addKeyframeTrack(GUI_KEYFRAME_TYPE type, const char* name, int track_id = 0) {
        auto ptr = new GuiTimelineKeyframeTrack(type, name, track_id);
        addChild(ptr);
        ptr->setOwner(this);
        tracks.push_back(std::unique_ptr<GuiTimelineTrackBase>(ptr));
        notifyOwner<GuiTimelineKeyframeTrack*>(GUI_NOTIFY::TIMELINE_KEYFRAME_TRACK_ADDED, ptr);
        return ptr;
    }
    GuiTimelineEventTrack* addEventTrack(int track_id = 0) {
        auto ptr = new GuiTimelineEventTrack(track_id);
        addChild(ptr);
        ptr->setOwner(this);
        tracks.push_back(std::unique_ptr<GuiTimelineTrackBase>(ptr));
        notifyOwner<GuiTimelineEventTrack*>(GUI_NOTIFY::TIMELINE_EVENT_TRACK_ADDED, ptr);
        return ptr;
    }
    GuiTimelineBlockTrack* addBlockTrack() {
        auto ptr = new GuiTimelineBlockTrack;
        addChild(ptr);
        ptr->setOwner(this);
        tracks.push_back(std::unique_ptr<GuiTimelineTrackBase>(ptr));
        notifyOwner<GuiTimelineBlockTrack*>(GUI_NOTIFY::TIMELINE_BLOCK_TRACK_ADDED, ptr);
        return ptr;
    }
    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return;
        }
        for (auto& i : tracks) {
            i->onHitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
        }
        hit.add(GUI_HIT::CLIENT, this);
        return;
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::MOUSE_SCROLL: {
            float diff = params.getA<int32_t>() / 100;
            setFrameScale(frame_screen_width + diff);
            } return true;
        case GUI_MSG::LBUTTON_DOWN:
            is_pressed = true;
            setCursor((guiGetMousePos().x - client_area.min.x - 10.f + frame_screen_width * .5f + content_offset.x) / frame_screen_width);
            guiCaptureMouse(this);
            return true;
        case GUI_MSG::LBUTTON_UP:
            is_pressed = false;
            guiCaptureMouse(0);
            return true;
        case GUI_MSG::MBUTTON_DOWN:
            is_panning = true;
            guiCaptureMouse(this);
            return true;
        case GUI_MSG::MBUTTON_UP:
            is_panning = false;
            guiCaptureMouse(0);
            return true;
        case GUI_MSG::MOUSE_MOVE:
            if (is_pressed) {
                setCursor((guiGetMousePos().x - client_area.min.x - 10.f + frame_screen_width * .5f + content_offset.x) / frame_screen_width);
            } else if(is_panning) {
                gfxm::vec2 offs = last_mouse_pos - gfxm::vec2(params.getA<int32_t>(), params.getB<int32_t>());
                content_offset += offs;
                content_offset = gfxm::vec2(content_offset.x, gfxm::_max(.0f, content_offset.y));
                setContentOffset(content_offset.x, content_offset.y);
            }
            last_mouse_pos = gfxm::vec2(params.getA<int32_t>(), params.getB<int32_t>());
            return true;
        case GUI_MSG::NOTIFY:
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::TIMELINE_DRAG_EVENT:
                for (auto& t : tracks) {
                    t->notify(GUI_NOTIFY::TIMELINE_DRAG_EVENT_CROSS_TRACK, params.getB<GuiTimelineEventItem*>());
                }
                return true;
            case GUI_NOTIFY::TIMELINE_DRAG_BLOCK:
                for (auto& t : tracks) {
                    t->notify(GUI_NOTIFY::TIMELINE_DRAG_BLOCK_CROSS_TRACK, params.getB<GuiTimelineBlockItem*>());
                }
                return true;
            }
            break;
        }
        return false;
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        rc_bounds = rc;
        client_area = rc_bounds;

        for (int i = 0; i < tracks.size(); ++i) {
            float y_offs = i * (track_height + track_margin);
            gfxm::rect rc(
                client_area.min + gfxm::vec2(.0f, y_offs),
                gfxm::vec2(client_area.max.x, client_area.min.y + y_offs + track_height)
            );
            tracks[i]->layout(rc, flags);
        }
    }
    void onDraw() override {
        guiDrawRect(client_area, GUI_COL_BG_INNER);
        guiDrawPushScissorRect(client_area);

        float bar_width = frame_screen_width * primary_divider * 2.f;
        float bar_width2 = bar_width * 2.f;
        float client_width = client_area.max.x - client_area.min.x;
        int bar_count = (ceilf(client_width / (float)bar_width) + 1) / 2 + 1;
        for (int i = 0; i < bar_count; ++i) {
            gfxm::vec2 offs_rem = gfxm::vec2(fmodf(content_offset.x, bar_width), fmodf(content_offset.y, bar_width));
            int a = content_offset.x / bar_width;
            float offs = bar_width;
            if ((a % 2) == 0) {
                offs = .0f;
            }
            guiDrawRect(
                gfxm::rect(
                    client_area.min + gfxm::vec2(i * bar_width2 + 10.0f - offs_rem.x + offs, .0f),
                    gfxm::vec2(client_area.min.x + bar_width + i * bar_width2 + 10.0f - offs_rem.x + offs, client_area.max.y)
                ), GUI_COL_BG_INNER_ALT
            );
        }
        int v_line_count = client_width / frame_screen_width + 1;
        for (int i = 0; i < v_line_count; ++i) {
            uint64_t color = GUI_COL_BG;
            gfxm::vec2 offs_rem = gfxm::vec2(fmodf(content_offset.x, frame_screen_width), fmodf(content_offset.y, frame_screen_width));
            
            int frame_id = i + (int)(content_offset.x / frame_screen_width);
            if ((frame_id % skip_divider)) {
                continue;
            }
            if ((frame_id % primary_divider) == 0) {
                color = GUI_COL_BUTTON;
            }
            guiDrawLine(
                gfxm::rect(
                    gfxm::vec2(client_area.min.x + i * frame_screen_width + 10.0f - offs_rem.x, client_area.min.y),
                    gfxm::vec2(client_area.min.x + i * frame_screen_width + 10.0f - offs_rem.x, client_area.max.y)
                ), 1.f, color
            );
        }

        for (int i = 0; i < tracks.size(); ++i) {
            guiDrawLine(gfxm::rect(
                gfxm::vec2(client_area.min.x + 10.0f, client_area.min.y + (i + 1) * (track_height + track_margin)),
                gfxm::vec2(client_area.max.x + 10.0f, client_area.min.y + (i + 1) * (track_height + track_margin))
            ), 1.f, GUI_COL_BUTTON);
        }

        for (int i = 0; i < tracks.size(); ++i) {
            tracks[i]->draw();
        }

        // Draw timeline cursor
        guiDrawLine(gfxm::rect(
            gfxm::vec2(client_area.min.x + 10.0f + cursor_frame * frame_screen_width - content_offset.x, client_area.min.y),
            gfxm::vec2(client_area.min.x + 10.0f + cursor_frame * frame_screen_width - content_offset.x, client_area.max.y)
        ), 1.f, GUI_COL_TIMELINE_CURSOR);

        guiDrawPopScissorRect();
    }
};

