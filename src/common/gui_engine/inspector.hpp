#pragma once

#include "reflection/reflection.hpp"
#include "gui/elements/element.hpp"
#include "gui/elements/input_string.hpp"


class GuiInspector : public GuiElement {
    rtti::MetaObject* object = nullptr;
    rtti::PropSnapshot* snap = nullptr;
    GuiInputString* search_bar = nullptr;
    GuiElement* container = nullptr;

    std::string filter;

    ResourceRef<gpuTexture2d> test_texture;

public:
    GuiInspector();

    void init(rtti::MetaObject* obj, rtti::PropSnapshot* snap);
    void updateView();
};

