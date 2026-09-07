#pragma once

#include "gui/elements/element.hpp"
#include "gui/elements/tree_item.hpp"


class GuiTreeView : public GuiElement {
    // TODO: Handle cases where selected items get deleted externally
    GuiTreeItem* selected_item = nullptr;
    GuiTreeItem* scroll_target = nullptr;

    void scrollToImpl(GuiTreeItem* item);
public:
    GuiTreeView();

    GuiTreeItem* addItem(const char* name);
    GuiTreeItem* getSelectedItem() { return selected_item; }
    
    void scrollTo(GuiTreeItem* item);
    void onTick(float dt, GUI_TICK_ID) override;
};

