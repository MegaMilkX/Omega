#pragma once

#include "gui/elements/gui_element.hpp"


#include "gui/elements/animation/gui_timeline_track_list.hpp"
#include "gui/elements/animation/gui_timeline_track_view.hpp"
#include "gui/elements/animation/gui_timeline_bar.hpp"
#include "gui/elements/splitters/gui_splitter_grid4.hpp"


// Should persist when events are being dragged across tracks or inside the same track
struct GuiTimelineEventData {
    GuiTimelineEventTrack* track;
    int frame;
    void* user_ptr;
};

class GuiTimelineEditor : public GuiElement {
    bool is_playing = false;

    gfxm::rect rc_left;
    gfxm::rect rc_right;
    gfxm::rect rc_splitter;

    std::unique_ptr<GuiSplitterGrid4> splitter;
    std::unique_ptr<GuiTimelineBar> track_bar;
    std::unique_ptr<GuiTimelineTrackList> track_list;
    std::unique_ptr<GuiTimelineTrackView> track_view;
    // TODO:
    std::unique_ptr<GuiButton> button_play;

    std::unique_ptr<GuiScrollBarV> scroll_v;
    std::unique_ptr<GuiScrollBarH> scroll_h;

    int next_track_id = 0;
public:
    std::function<void(void)> on_play;
    std::function<void(void)> on_pause;
    std::function<void(int)> on_cursor;

    GuiTimelineEventTrack* addEventTrack(void* user_ptr) {
        track_list->addItem("EventTrack");
        auto trk = track_view->addEventTrack(next_track_id);
        int track_id = next_track_id;
        ++next_track_id;
        trk->user_ptr = user_ptr;
        return trk;
    }
    GuiTimelineBlockTrack* addBlockTrack() {
        track_list->addItem("BlockTrack");
        auto trk = track_view->addBlockTrack();
        return trk;
    }

    void togglePlay() {
        if (!is_playing) {
            is_playing = true;
            button_play->setIcon(guiLoadIcon("svg/entypo/controller-paus.svg"));
            if (on_play) { on_play(); }
        } else {
            is_playing = false;
            button_play->setIcon(guiLoadIcon("svg/entypo/controller-play.svg"));
            if (on_pause) { on_pause(); }
        }
    }

    void setCursor(int frame, bool send_notification = true) {
        frame = std::max(frame, 0);
        track_bar->setCursor(frame, send_notification);
        track_view->setCursor(frame, send_notification);
        if (on_cursor) { on_cursor(frame); }
    }
    void setCursorSilent(int frame) {
        frame = std::max(frame, 0);
        track_bar->setCursor(frame, false);
        track_view->setCursor(frame, false);
    }
    GuiTimelineEditor() {
        splitter.reset(new GuiSplitterGrid4);
        addChild(splitter.get());

        scroll_v.reset(new GuiScrollBarV);
        scroll_h.reset(new GuiScrollBarH);

        track_bar.reset(new GuiTimelineBar);

        track_list.reset(new GuiTimelineTrackList);

        track_view.reset(new GuiTimelineTrackView);

        button_play.reset(new GuiButton(""));
        button_play->setIcon(guiLoadIcon("svg/entypo/controller-play.svg"));
        button_play->setOwner(this);

        scroll_v->setOwner(this);
        scroll_h->setOwner(this);
        track_bar->setOwner(this);
        track_list->setOwner(this);
        track_view->setOwner(this);

        splitter->setElemTopLeft(button_play.get());
        splitter->setElemTopRight(track_bar.get());
        splitter->setElemBottomLeft(track_list.get());
        splitter->setElemBottomRight(track_view.get());
    }
    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        GuiHitResult hit = splitter->hitTest(x, y);
        if (hit.hasHit()) {
            return hit;
        }
        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::NOTIFY:
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::TIMELINE_JUMP:
                setCursor(params.getB<int>(), false);
                break;
            case GUI_NOTIFY::TIMELINE_ZOOM:
                track_bar->setFrameWidth(params.getB<float>());
                break;
            case GUI_NOTIFY::TIMELINE_PAN_X:
                track_bar->content_offset.x = params.getB<float>();
                track_view->content_offset.x = params.getB<float>();
                break;
            case GUI_NOTIFY::TIMELINE_PAN_Y:
                track_bar->content_offset.y = params.getB<float>();
                track_view->content_offset.y = params.getB<float>();
                break;
            case GUI_NOTIFY::BUTTON_CLICKED:
                togglePlay();
                break;
            case GUI_NOTIFY::TIMELINE_EVENT_ADDED: {
                forwardMessageToOwner(msg, params);
                }break;
            case GUI_NOTIFY::TIMELINE_EVENT_REMOVED: {
                forwardMessageToOwner(msg, params);
                }break;
            case GUI_NOTIFY::TIMELINE_EVENT_MOVED: {
                forwardMessageToOwner(msg, params);
                }break;
            case GUI_NOTIFY::TIMELINE_BLOCK_ADDED: {
                forwardMessageToOwner(msg, params);
                }break;
            case GUI_NOTIFY::TIMELINE_BLOCK_REMOVED: {
                forwardMessageToOwner(msg, params);
                }break;
            case GUI_NOTIFY::TIMELINE_BLOCK_MOVED_RESIZED: {
                forwardMessageToOwner(msg, params);
                }break;
            case GUI_NOTIFY::TIMELINE_EVENT_SELECTED:
                forwardMessageToOwner(msg, params);
                break;
            case GUI_NOTIFY::TIMELINE_BLOCK_SELECTED:
                forwardMessageToOwner(msg, params);
                break;
            }
            break;
        }
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = rc;
        client_area = bounding_rect;

        splitter->layout(client_area.min, client_area, flags);
    }
    void onDraw() override {
        splitter->draw();
    }
};

