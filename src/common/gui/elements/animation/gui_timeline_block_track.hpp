#pragma once

#include "gui/elements/gui_element.hpp"
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
    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        for (int i = blocks.size() - 1; i >= 0; --i) {
            auto& b = blocks[i];
            GuiHitResult hit;
            hit = b->hitTest(x, y);
            if (hit.hasHit()) {
                return hit;
            }
        }
        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::LBUTTON_DOWN:
            getOwner()->sendMessage(msg, params);
            break;
        case GUI_MSG::LBUTTON_UP:
            getOwner()->sendMessage(msg, params);
            break;
        case GUI_MSG::MOUSE_MOVE: {
            gfxm::vec2 m(params.getA<int32_t>(), params.getB<int32_t>());
            gfxm::vec2 mlcl = m - client_area.min;
            mouse_cur_frame = getFrameAtScreenPos(mlcl.x, mlcl.y);
            getOwner()->sendMessage(msg, params);
            break;
        } 
        case GUI_MSG::RBUTTON_DOWN: {
            guiCaptureMouse(this);
            auto m = guiGetMousePosLocal(client_area);
            gfxm::vec2 mlcl = m - client_area.min;
            startBlockPaint(getFrameAtScreenPos(mlcl.x, mlcl.y));
            break;
        } 
        case GUI_MSG::RBUTTON_UP: {
            auto m = guiGetMousePosLocal(client_area);
            stopBlockPaint();
            guiCaptureMouse(0);
            break;
        } 
        case GUI_MSG::NOTIFY:
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::TIMELINE_DRAG_BLOCK: {
                auto block = params.getB<GuiTimelineBlockItem*>();
                gfxm::vec2 mouse = guiGetMousePosLocal(client_area);
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
                break;
            }
            case GUI_NOTIFY::TIMELINE_DRAG_BLOCK_CROSS_TRACK: {
                auto block = params.getB<GuiTimelineBlockItem*>();
                gfxm::vec2 mouse = guiGetMousePosLocal(client_area);
                mouse = mouse - client_area.min;
                mouse -= block->grab_point;
                if (block->getOwner() != this && block->type == this->type) {
                    if (gfxm::point_in_rect(client_area, guiGetMousePosLocal(client_area))) {
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
                break;
            }
            case GUI_NOTIFY::TIMELINE_ERASE_BLOCK: {
                removeItem(params.getB<GuiTimelineBlockItem*>());
                break;
            }
            case GUI_NOTIFY::TIMELINE_RESIZE_BLOCK_LEFT: {
                auto block = params.getB<GuiTimelineBlockItem*>();
                gfxm::vec2 mouse = guiGetMousePosLocal(client_area);
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
                break;
            }
            case GUI_NOTIFY::TIMELINE_RESIZE_BLOCK_RIGHT: {
                auto block = params.getB<GuiTimelineBlockItem*>();
                gfxm::vec2 mouse = guiGetMousePosLocal(client_area);
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
                break;
            }
            case GUI_NOTIFY::TIMELINE_BLOCK_SELECTED:
                notifyOwner<GuiTimelineBlockTrack*, GuiTimelineBlockItem*>(
                    GUI_NOTIFY::TIMELINE_BLOCK_SELECTED, this, params.getC<GuiTimelineBlockItem*>()
                );
                break;
            }
            break;
        }
        GuiTimelineTrackBase::onMessage(msg, params);
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = rc;
        client_area = rc;
        
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
            i->layout(p, gfxm::rect(p, p2), flags);
        }
    }
    void onDraw() override {
        for (auto& i : blocks) {
            i->draw();
        }
    }
};

