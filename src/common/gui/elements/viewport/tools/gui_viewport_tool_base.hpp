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
    GuiViewportToolBase* nested_tool = nullptr;

    void signalToolChange();
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
    
    void attachTool(GuiViewportToolBase* nested) {
        if(nested_tool) {
            nested_tool->setViewport(nullptr);
            removeChild(nested_tool);
            nested_tool = nullptr;
        }
        
        if (nested == nullptr) {
            signalToolChange();
            return;
        }

        nested_tool = nested;
        nested_tool->setViewport(viewport);
        pushBack(nested_tool);
        guiSetFocusedWindow(nested_tool);
        signalToolChange();
    }
    void detachTool() {
        if (!nested_tool) {
            return;
        }

        nested_tool->setViewport(nullptr);
        removeChild(nested_tool);
        nested_tool = nullptr;
        signalToolChange();
    }
    GuiViewportToolBase* getNestedTool() {
        return nested_tool;
    }

    virtual void setViewport(GuiViewport* vp) {
        viewport = vp;
        if (nested_tool) {
            nested_tool->setViewport(vp);
        }
    }
    void setViewProjection(const gfxm::mat4& view, const gfxm::mat4& projection) {
        this->view = view;
        this->projection = projection;
        if (nested_tool) {
            nested_tool->setViewProjection(view, projection);
        }
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
        if (nested_tool) {
            nested_tool->layout_2(ctx);
        }
    }
    virtual void onDraw() {
        //assert(false);
    }
};
