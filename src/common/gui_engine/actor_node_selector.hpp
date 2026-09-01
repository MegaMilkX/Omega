#pragma once

#include "gui/elements/element.hpp"
#include "gui/elements/button.hpp"
#include "gui/elements/tree_view.hpp"
#include "reflection/type.hpp"


class GuiActorNodeSelector : public GuiElement {
    rtti::type selected_type;

    GuiElement* description = nullptr;

    void initTypeTreeImpl(GuiElement* elem, rtti::type base_type);
    void initTypeTree(GuiElement* elem, rtti::type base_type);
    void initControls();
public:
    GuiActorNodeSelector();
};