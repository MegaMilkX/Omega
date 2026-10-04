#pragma once

#include "gui/elements/viewport/tools/gui_viewport_tool_base.hpp"
#include "gpu/scene_query_interface.hpp"


class TerrainSceneSpace;

class TerrainEditMode : public GuiViewportToolBase {
    TerrainSceneSpace* space = nullptr;
protected:
    TerrainSceneSpace* getSpace() { return space; }
public:
    TerrainEditMode(const char* name, TerrainSceneSpace* space)
        : GuiViewportToolBase(name), space(space) {}
    virtual ~TerrainEditMode() {}
    virtual void queryGeometry(const GeometryQuery&) {}
};

