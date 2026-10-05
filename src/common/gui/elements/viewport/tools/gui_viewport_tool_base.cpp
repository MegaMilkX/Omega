#include "gui_viewport_tool_base.hpp"

#include "../gui_viewport.hpp"


void GuiViewportToolBase::signalToolChange() {
    if(!viewport) return;
    viewport->signalToolChange();
}