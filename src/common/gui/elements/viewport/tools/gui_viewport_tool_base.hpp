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
        : tool_name(name) {}
    virtual ~GuiViewportToolBase() {}
    const char* getToolName() const { return tool_name; }
    
    virtual void setViewport(GuiViewport* vp) { viewport = vp; }

    virtual void onDrawTool(const gfxm::rect& client_area, const gfxm::mat4& proj, const gfxm::mat4& view) {}

    virtual void onHitTest(GuiHitResult& hit, int x, int y) {
        return;
    }
    virtual void onLayout(const gfxm::rect& rect, uint64_t flags) {}
    virtual void onDraw() {
        assert(false);
    }
};
