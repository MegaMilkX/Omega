#pragma once

#include "gui/elements/element.hpp"
#include "gui/elements/tree_item.hpp"


class GuiTreeView : public GuiElement {
    // TODO: Handle cases where selected items get deleted externally
    GuiTreeItem* selected_item = nullptr;
public:
    GuiTreeView();

    GuiTreeItem* addItem(const char* name);

    GuiTreeItem* getSelectedItem() { return selected_item; }
};

