#pragma once

#include "gui/elements/element.hpp"
#include "gui/gui_system.hpp"
#include "gui/elements/animation/gui_timeline_track_base.hpp"
#include "gui/elements/animation/gui_timeline_block_item.hpp"


class GuiTimelineBlockTrack : public GuiTimelineTrackBase {
    std::vector<std::unique_ptr<GuiTimelineBlockItem>> blocks;
    int block_paint_start_frame = 0;
    int mouse_cur_frame = 0;
    GuiTimelineBlockItem* block_painted_item = 0;
    void startBlockPaint(int at) {
        block_paint_start_frame = at;
        block_painted_item = addItem(at, 1);
        block_painted_item->highlight_override = true;
    }
    void stopBlockPaint() {
        if (block_painted_item) {
            block_painted_item->highlight_override = false;
            block_painted_item = 0;
        }
    }
    void updateBlockPaint() {
        if (!block_painted_item) {
            return;
        }
        if (mouse_cur_frame > block_paint_start_frame) {
            block_painted_item->frame = block_paint_start_frame;
            block_painted_item->length = mouse_cur_frame - block_paint_start_frame;
            notifyOwner<GuiTimelineBlockTrack*, GuiTimelineBlockItem*>(
                GUI_NOTIFY::TIMELINE_BLOCK_MOVED_RESIZED,
                this, block_painted_item
            );
        } else if(mouse_cur_frame < block_paint_start_frame) {
            block_painted_item->frame = mouse_cur_frame;
            block_painted_item->length = block_paint_start_frame - mouse_cur_frame;
            notifyOwner<GuiTimelineBlockTrack*, GuiTimelineBlockItem*>(
                GUI_NOTIFY::TIMELINE_BLOCK_MOVED_RESIZED,
                this, block_painted_item
            );
        }
    }
    void sort() {
        std::sort(blocks.begin(), blocks.end(), [](const std::unique_ptr<GuiTimelineBlockItem>& a, const std::unique_ptr<GuiTimelineBlockItem>& b)->bool {
            return a->frame < b->frame;
        });
    }
public:
    int type = 0;
    void* user_ptr = 0;

    GuiTimelineBlockItem* addItem(int at, int len, bool silent = false) {
        auto ptr = new GuiTimelineBlockItem(at, len);
        ptr->type = this->type;
        ptr->setOwner(this);
        addChild(ptr);
        blocks.push_back(std::unique_ptr<GuiTimelineBlockItem>(ptr));
        sort();
        if (!silent) {
            notifyOwner<GuiTimelineBlockTrack*, GuiTimelineBlockItem*>(
                GUI_NOTIFY::TIMELINE_BLOCK_ADDED, this, ptr
            );
        }
        return ptr;
    }
    void removeItem(GuiTimelineBlockItem* item, bool silent = false) {
        stopBlockPaint();
        for (int i = 0; i < blocks.size(); ++i) {
            if (blocks[i].get() == item) {
                if (!silent) {
                    notifyOwner<GuiTimelineBlockTrack*, GuiTimelineBlockItem*>(
                        GUI_NOTIFY::TIMELINE_BLOCK_REMOVED, this, item
                    );
                }
                removeChild(item);
                blocks.erase(blocks.begin() + i);
                break;
            }
        }
    }
    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return;
        }
        for (int i = blocks.size() - 1; i >= 0; --i) {
            auto& b = blocks[i];
            b->hitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
        }
        hit.add(GUI_HIT::CLIENT, this);
        return;
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        if (isLocked()) {
            return GuiTimelineTrackBase::onMessage(msg, params);
        }
        switch (msg) {
        case GUI_MSG::LBUTTON_DOWN:
            getOwner()->sendMessage(msg, params);
            return true;
        case GUI_MSG::LBUTTON_UP:
            getOwner()->sendMessage(msg, params);
            return true;
        case GUI_MSG::MOUSE_MOVE: {
            gfxm::vec2 m(params.getA<int32_t>(), params.getB<int32_t>());
            gfxm::vec2 mlcl = m - client_area.min;
            mouse_cur_frame = getFrameAtScreenPos(mlcl.x, mlcl.y);
            getOwner()->sendMessage(msg, params);
            return true;
        } 
        case GUI_MSG::RBUTTON_DOWN: {
            guiCaptureMouse(this);
            auto m = guiGetMousePos();
            gfxm::vec2 mlcl = m - client_area.min;
            startBlockPaint(getFrameAtScreenPos(mlcl.x, mlcl.y));
            return true;
        } 
        case GUI_MSG::RBUTTON_UP: {
            auto m = guiGetMousePos();
            stopBlockPaint();
            guiCaptureMouse(0);
            return true;
        } 
        case GUI_MSG::NOTIFY:
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::TIMELINE_DRAG_BLOCK: {
                auto block = params.getB<GuiTimelineBlockItem*>();
                gfxm::vec2 mouse = guiGetMousePos();
                mouse = mouse - client_area.min;
                mouse -= block->grab_point;
                int frame = getFrameAtScreenPos(mouse.x, mouse.y);
                if (frame != block->frame) {
                    block->frame = frame;
                    sort();
                    notifyOwner<GuiTimelineBlockTrack*, GuiTimelineBlockItem*>(
                        GUI_NOTIFY::TIMELINE_BLOCK_MOVED_RESIZED,
                        this, block
                    );
                }
                notifyOwner(GUI_NOTIFY::TIMELINE_DRAG_BLOCK, block);
                return true;
            }
            case GUI_NOTIFY::TIMELINE_DRAG_BLOCK_CROSS_TRACK: {
                auto block = params.getB<GuiTimelineBlockItem*>();
                gfxm::vec2 mouse = guiGetMousePos();
                mouse = mouse - client_area.min;
                mouse -= block->grab_point;
                if (block->getOwner() != this && block->type == this->type) {
                    if (gfxm::point_in_rect(client_area, guiGetMousePos())) {
                        GuiTimelineBlockItem* new_block = addItem(block->frame, block->length, true);
                        if (new_block) {
                            new_block->user_ptr = block->user_ptr;
                            new_block->type = block->type;
                            new_block->color = block->color;
                            new_block->grab_point = block->grab_point;
                            ((GuiTimelineBlockTrack*)block->getOwner())
                                ->removeItem(block, true);
                            new_block->sendMessage(GUI_MSG::LBUTTON_DOWN, GUI_MSG_PARAMS());

                            notifyOwner<GuiTimelineBlockTrack*, GuiTimelineBlockItem*>(
                                GUI_NOTIFY::TIMELINE_BLOCK_MOVED_RESIZED,
                                this, new_block
                            );
                        }
                    }
                }
                return true;
            }
            case GUI_NOTIFY::TIMELINE_ERASE_BLOCK: {
                removeItem(params.getB<GuiTimelineBlockItem*>());
                return true;
            }
            case GUI_NOTIFY::TIMELINE_RESIZE_BLOCK_LEFT: {
                auto block = params.getB<GuiTimelineBlockItem*>();
                gfxm::vec2 mouse = guiGetMousePos();
                mouse = mouse - client_area.min;
                int frame = getFrameAtScreenPos(mouse.x, mouse.y);
                if (frame != block->frame && frame != block->frame + block->length) {
                    if (frame < block->frame + block->length) {
                        int diff = frame - block->frame;
                        block->frame = frame;
                        block->length -= diff;
                        notifyOwner<GuiTimelineBlockTrack*, GuiTimelineBlockItem*>(
                            GUI_NOTIFY::TIMELINE_BLOCK_MOVED_RESIZED,
                            this, block
                        );
                    }
                }
                return true;
            }
            case GUI_NOTIFY::TIMELINE_RESIZE_BLOCK_RIGHT: {
                auto block = params.getB<GuiTimelineBlockItem*>();
                gfxm::vec2 mouse = guiGetMousePos();
                mouse = mouse - client_area.min;
                int frame = getFrameAtScreenPos(mouse.x, mouse.y);
                if (frame != block->frame && frame != block->frame + block->length) {
                    if (frame > block->frame) {
                        block->length = frame - block->frame;
                        notifyOwner<GuiTimelineBlockTrack*, GuiTimelineBlockItem*>(
                            GUI_NOTIFY::TIMELINE_BLOCK_MOVED_RESIZED,
                            this, block
                        );
                    }
                }
                return true;
            }
            case GUI_NOTIFY::TIMELINE_BLOCK_SELECTED:
                notifyOwner<GuiTimelineBlockTrack*, GuiTimelineBlockItem*>(
                    GUI_NOTIFY::TIMELINE_BLOCK_SELECTED, this, params.getC<GuiTimelineBlockItem*>()
                );
                return true;
            }
            break;
        }
        return GuiTimelineTrackBase::onMessage(msg, params);
    }
    void onLayout(const gfxm::vec2& extents, uint64_t flags) override {
        rc_bounds = gfxm::rect(gfxm::vec2(0, 0), extents);
        client_area = rc_bounds;
        
        updateBlockPaint();
        for (auto& i : blocks) {
            gfxm::vec2 p(
                getScreenXAtFrame(i->frame),
                client_area.min.y
            );
            gfxm::vec2 p2(
                getScreenXAtFrame(i->frame + i->length),
                client_area.max.y
            );
            i->layout_position = p;
            i->layout(p2 - p, flags);
        }
    }
    void onDraw() override {
        for (auto& i : blocks) {
            i->draw();
        }
    }
};

