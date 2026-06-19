#pragma once

#include <assert.h>
#include "gui/gui.hpp"
#include "math/gfxm.hpp"


class GuiViewport;
class GuiViewportToolBase : public GuiElement {
    const char* tool_name = 0;
protected:
    GuiViewport* viewport = 0;
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
    
    virtual void setViewport(GuiViewport* vp) { viewport = vp; }

    virtual void onDrawTool(const gfxm::rect& client_area, const gfxm::mat4& proj, const gfxm::mat4& view) {}

    virtual void onHitTest(GuiHitResult& hit, int x, int y) override {
        hit.add(GUI_HIT::CLIENT, this);
        return;
    }
    int measureWidth(const std::optional<int>& height) override {
        return 0;
    }
    int measureHeight(const std::optional<int>& width) override {
        return 0;
    }
    void layout_2(const gui_layout_context& ctx) override {
        rc_bounds = gfxm::rect(gfxm::vec2(0, 0), gfxm::vec2(ctx.width.value_or(0), ctx.height.value_or(0)));
        client_area = rc_bounds;
    }
    virtual void onDraw() {
        assert(false);
    }
};
