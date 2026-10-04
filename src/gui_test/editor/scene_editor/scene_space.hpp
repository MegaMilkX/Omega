#pragma once

#include "nlohmann/json.hpp"
#include "gui/elements/viewport/tools/gui_viewport_tool_base.hpp"
#include "gpu/scene_query_interface.hpp"


struct SceneEditorContext {

};

class SceneSpace : public gpuSceneQueryInterface, public GuiViewportToolBase {
public:
    SceneSpace(const char* name)
        : GuiViewportToolBase(name) {}
    virtual ~SceneSpace() {}

    virtual void enterUi(SceneEditorContext& ctx) = 0;
    virtual void exitUi(SceneEditorContext& ctx) = 0;

    virtual void toJson(nlohmann::json& json) const = 0;
    virtual bool fromJson(const nlohmann::json& json) = 0;
};

