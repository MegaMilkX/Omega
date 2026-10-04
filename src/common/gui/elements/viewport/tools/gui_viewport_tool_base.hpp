#pragma once

#include <assert.h>
#include "gui/gui.hpp"
#include "math/gfxm.hpp"
#include "gui/elements/zstack.hpp"


class GuiViewport;
class GuiViewportToolBase : public GuiElement {
    const char* tool_name = 0;
protected:
    GuiViewport* viewport = nullptr;
    GuiViewportToolBase* next_tool = nullptr;
public:
    gfxm::mat4 projection;
    gfxm::mat4 view;

    GuiViewportToolBase(const char* name)
        : tool_name(name)
    {
        subscribe<GuiEvt_Focus>([this](const GuiEvt_Focus& e) {
            e.new_focused = this;
        });
    }
    virtual ~GuiViewportToolBase() {}
    const char* getToolName() const { return tool_name; }
    
    void detachAllTools() {
        if (next_tool) {
            next_tool->detachAllTools();
            removeChild(next_tool);
            next_tool = nullptr;
        }
        viewport = nullptr;
    }
    void attachTool(GuiViewportToolBase* next) {
        if (next_tool) {
            next_tool->attachTool(next);
            return;
        }
        next_tool = next;
        next_tool->setViewport(viewport);
        pushBack(next_tool);
        guiSetFocusedWindow(next_tool);
    }
    void detachTool(GuiViewportToolBase* next) {
        if (next_tool == next) {
            next->detachAllTools();
            removeChild(next);
            next->setViewport(nullptr);
            next_tool = next_tool->next_tool;
            if (next_tool) {
                pushBack(next_tool);
            }
            return;
        }

        if (next_tool) {
            next_tool->detachTool(next);
        }
    }
    GuiViewportToolBase* getNextTool() {
        return next_tool;
    }

    virtual void setViewport(GuiViewport* vp) { viewport = vp; }
    void setViewProjection(const gfxm::mat4& view, const gfxm::mat4& projection) {
        this->view = view;
        this->projection = projection;
    }

    virtual void onDrawTool(const gfxm::rect& client_area, const gfxm::mat4& proj, const gfxm::mat4& view) {}

    int measureWidth(const std::optional<int>& height) override {
        return 0;
    }
    int measureHeight(const std::optional<int>& width) override {
        return 0;
    }
    void layout_2(const gui_layout_context& ctx) override {
        rc_bounds = gfxm::rect(gfxm::vec2(0, 0), gfxm::vec2(ctx.width.value_or(0), ctx.height.value_or(0)));
        client_area = rc_bounds;
        if (next_tool) {
            next_tool->layout_2(ctx);
        }
    }
    virtual void onDraw() {
        //assert(false);
    }
};
