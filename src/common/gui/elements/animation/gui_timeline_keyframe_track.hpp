#pragma once

#include "gui/elements/animation/gui_timeline_track_base.hpp"
#include "gui/elements/animation/gui_timeline_keyframe_item.hpp"
#include "gui/gui_system.hpp"

enum class GUI_KEYFRAME_TYPE {
    FLOAT,
    VEC2,
    VEC3,
    VEC4
};

class GuiTimelineKeyframeTrack : public GuiTimelineTrackBase {
    GUI_KEYFRAME_TYPE type;
    std::string name;
    int track_id = 0;
    std::vector<std::unique_ptr<GuiTimelineKeyframeItem>> items;

    void sort() {
        std::sort(items.begin(), items.end(), [](const std::unique_ptr<GuiTimelineKeyframeItem>& a, const std::unique_ptr<GuiTimelineKeyframeItem>& b)->bool {
            return a->frame < b->frame;
        });
    }
public:
    GuiTimelineKeyframeTrack(GUI_KEYFRAME_TYPE type, const char* name, int track_id = 0)
        : type(type), name(name), track_id(track_id) {

    }

    GUI_KEYFRAME_TYPE getType() const { return type; }
    const std::string& getName() const override { return name; }

    int keyframeCount() const {
        return items.size();
    }
    GuiTimelineKeyframeItem* getKeyframe(int i) {
        return items[i].get();
    }

    GuiTimelineKeyframeItem* addItem(int at, bool silent = false) {
        for (auto& item : items) {
            if (item->frame == at) {
                if (!silent) {
                    notifyOwner<GuiTimelineKeyframeTrack*, GuiTimelineKeyframeItem*>(
                        GUI_NOTIFY::TIMELINE_KEYFRAME_ADDED,
                        this, item.get()
                    );
                }
                return item.get();
            }
        }

        auto item = new GuiTimelineKeyframeItem(at);
        item->setOwner(this);
        item->owner_track = this;
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
    void onLayout(const gfxm::vec2& extents, uint64_t flags) override {
        rc_bounds = gfxm::rect(gfxm::vec2(0, 0), extents);
        client_area = rc_bounds;
        for (auto& item : items) {
            gfxm::vec2 p(
                getScreenXAtFrame(item->frame),
                client_area.center().y
            );
            item->layout_position = p;
            item->layout(gfxm::vec2(100, 100) /* UNUSED */, flags);
        }
    }
    void onDraw() override {
        for (auto& i : items) {
            i->draw();
        }
    }
};