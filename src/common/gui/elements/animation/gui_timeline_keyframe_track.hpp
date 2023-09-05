#pragma once

#include "gui/elements/animation/gui_timeline_track_base.hpp"
#include "gui/elements/animation/gui_timeline_keyframe_item.hpp"
#include "gui/gui_system.hpp"


class GuiTimelineKeyframeTrack : public GuiTimelineTrackBase {
    std::string name;
    int track_id = 0;
    std::vector<std::unique_ptr<GuiTimelineKeyframeItem>> items;

    void sort() {
        std::sort(items.begin(), items.end(), [](const std::unique_ptr<GuiTimelineKeyframeItem>& a, const std::unique_ptr<GuiTimelineKeyframeItem>& b)->bool {
            return a->frame < b->frame;
        });
    }
public:
    GuiTimelineKeyframeTrack(const char* name, int track_id = 0)
        : name(name), track_id(track_id) {

    }

    const std::string& getName() const override { return name; }

    GuiTimelineKeyframeItem* addItem(int at, bool silent = false) {
        for (auto& item : items) {
            if (item->frame == at) {
                return item.get();
            }
        }

        auto item = new GuiTimelineKeyframeItem(at);
        item->setOwner(this);
        addChild(item);
        items.push_back(std::unique_ptr<GuiTimelineKeyframeItem>(item));
        sort();
        if (!silent) {
            notifyOwner<GuiTimelineKeyframeTrack*, GuiTimelineKeyframeItem*>(
                GUI_NOTIFY::TIMELINE_KEYFRAME_ADDED,
                this, item
            );
        }
        return item;
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        rc_bounds = rc;
        client_area = rc_bounds;
        for (auto& item : items) {
            gfxm::vec2 p(
                getScreenXAtFrame(item->frame),
                client_area.center().y
            );
            item->layout(gfxm::rect(p, p), flags);
        }
    }
    void onDraw() override {
        for (auto& i : items) {
            i->draw();
        }
    }
};