#pragma once

#include "gui/elements/element.hpp"
#include "gui/elements/animation/gui_timeline_track_base.hpp"
#include "gui/elements/animation/gui_timeline_event_item.hpp"
#include "gui/gui_system.hpp"

struct GuiTimelineEventMoved {
    uint32_t from;
    uint32_t to;
};
class GuiTimelineEventTrack : public GuiTimelineTrackBase {
    int track_id = 0;
    std::vector<std::unique_ptr<GuiTimelineEventItem>> items;
    std::set<int> occupied_frames;
    void sort() {
        std::sort(items.begin(), items.end(), [](const std::unique_ptr<GuiTimelineEventItem>& a, const std::unique_ptr<GuiTimelineEventItem>& b)->bool {
            return a->frame < b->frame;
        });
    }
public:
    int type = 0;
    void* user_ptr = 0;

    GuiTimelineEventTrack(int track_id = 0)
        : track_id(track_id) {}
    GuiTimelineEventItem* addItem(int at, bool silent = false) {
        if (occupied_frames.find(at) != occupied_frames.end()) {
            return 0;
        }
        auto ptr = new GuiTimelineEventItem(at);
        ptr->setOwner(this);
        addChild(ptr);
        items.push_back(std::unique_ptr<GuiTimelineEventItem>(ptr));
        occupied_frames.insert(at);
        sort();
        if (!silent) {
            notifyOwner<GuiTimelineEventTrack*, GuiTimelineEventItem*>(
                GUI_NOTIFY::TIMELINE_EVENT_ADDED,
                this, ptr
            );
        }
        return ptr;
    }
    void removeItem(GuiTimelineEventItem* item, bool silent = false) {
        for (int i = 0; i < items.size(); ++i) {
            if (items[i].get() == item) {
                if (!silent) {
                    notifyOwner<GuiTimelineEventTrack*, GuiTimelineEventItem*>(
                        GUI_NOTIFY::TIMELINE_EVENT_REMOVED,
                        this, items[i].get()
                    );
                }
                occupied_frames.erase(items[i]->frame);
                removeChild(item);
                items.erase(items.begin() + i);
                break;
            }
        }
    }
    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return;
        }
        for (int i = items.size() - 1; i >= 0; --i) {
            auto& item = items[i];
            item->onHitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
        }
        hit.add(GUI_HIT::CLIENT, this);
        return;
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::RBUTTON_DOWN: {
            gfxm::vec2 mouse = guiGetMousePos() - client_area.min;
            int frame = getFrameAtScreenPos(mouse.x, mouse.y);
            addItem(frame);
            }return true;
        case GUI_MSG::NOTIFY:
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::TIMELINE_DRAG_EVENT: {
                auto evt = params.getB<GuiTimelineEventItem*>();
                int original_frame = evt->frame;
                gfxm::vec2 mouse = guiGetMousePos();
                mouse = mouse - client_area.min;
                int frame = getFrameAtScreenPos(mouse.x, mouse.y);
                if (occupied_frames.find(frame) == occupied_frames.end() && evt->frame != frame) {
                    occupied_frames.erase(evt->frame);
                    evt->frame = frame;
                    occupied_frames.insert(frame);
                    sort();
                    notifyOwner<GuiTimelineEventTrack*, GuiTimelineEventItem*>(
                        GUI_NOTIFY::TIMELINE_EVENT_MOVED,
                        this,
                        evt
                    );
                }
                notifyOwner(GUI_NOTIFY::TIMELINE_DRAG_EVENT, evt);
                return true;
            }
            case GUI_NOTIFY::TIMELINE_DRAG_EVENT_CROSS_TRACK: {
                auto evt = params.getB<GuiTimelineEventItem*>();
                gfxm::vec2 mouse = guiGetMousePos();
                mouse = mouse - client_area.min;
                if (evt->getOwner() != this) {
                    if (gfxm::point_in_rect(client_area, guiGetMousePos())) {
                        GuiTimelineEventItem* new_evt = addItem(evt->frame, true);
                        if (new_evt) {
                            new_evt->user_ptr = evt->user_ptr;
                            ((GuiTimelineEventTrack*)evt->getOwner())
                                ->removeItem(evt, true);
                            new_evt->sendMessage(GUI_MSG::LBUTTON_DOWN, GUI_MSG_PARAMS());

                            notifyOwner<GuiTimelineEventTrack*, GuiTimelineEventItem*>(
                                GUI_NOTIFY::TIMELINE_EVENT_MOVED,
                                this,
                                new_evt
                            );
                        }
                    }
                }
                return true;
            }
            case GUI_NOTIFY::TIMELINE_ERASE_EVENT: {
                removeItem(params.getB<GuiTimelineEventItem*>());
                return true;
            }
            case GUI_NOTIFY::TIMELINE_EVENT_SELECTED:
                notifyOwner<GuiTimelineEventTrack*, GuiTimelineEventItem*>(
                    GUI_NOTIFY::TIMELINE_EVENT_SELECTED, this, params.getC<GuiTimelineEventItem*>()
                );
                return true;
            }
            break;
        }
        return GuiTimelineTrackBase::onMessage(msg, params);
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        rc_bounds = rc;
        client_area = rc_bounds;
        for (auto& i : items) {
            gfxm::vec2 p(
                getScreenXAtFrame(i->frame),
                client_area.center().y
            );
            i->layout(gfxm::rect(p, p), flags);
        }
    }
    void onDraw() override {
        for (auto& i : items) {
            i->draw();
        }
    }
};

