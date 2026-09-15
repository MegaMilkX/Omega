#pragma once

#include "reflection/reflection.hpp"
#include "gui/elements/element.hpp"
#include "gui/elements/input_string.hpp"


class GuiInspector : public GuiElement {
    rtti::MetaObject* object = nullptr;
    rtti::PropSnapshot* snap = nullptr;
    rtti::PropSnapshot* snap_delta = nullptr;
    GuiInputString* search_bar = nullptr;
    GuiElement* container = nullptr;

    std::string filter;

    ResourceRef<gpuTexture2d> test_texture;
    void buildSingleProp(
        GuiElement* container,
        rtti::MetaObject* object,
        rtti::PropSnapshot& snap, // Base object state also serving as the schema
        rtti::PropSnapshot& snap_delta, // Changes accumulated over time in terms of properties touched. The values are not deltas in any way
        const std::vector<std::string>& prop_path,
        rtti::varying& var
    );
    void buildRows(const std::vector<std::string>& path, GuiElement*, rtti::MetaObject*, rtti::PropSnapshot&);
public:
    GuiInspector();

    void init(rtti::MetaObject* obj, rtti::PropSnapshot* snap, rtti::PropSnapshot* snap_delta);
    void updateView();
};

