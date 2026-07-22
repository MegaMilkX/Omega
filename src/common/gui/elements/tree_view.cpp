#include "tree_view.hpp"


GuiTreeView::GuiTreeView() {
    setSize(gui::fill(), 250);
    //setMaxSize(0, 0);
    setMinSize(0, 100);
    addFlags(GUI_FLAG_RESIZE_Y);

    setStyleClasses({ "tree-view" });

    auto models = addItem("Models");
    models->addItem("chara_24");
    models->addItem("sword");
    models->addItem("gun");
    auto b = addItem("Textures");
    b->addItem("bricks.png");
    b->addItem("terrain")->addItem("grass.png");
    addItem("Shaders")->addItem("default.glsl");

    subscribe<GuiEvt_Selected>([this](const GuiEvt_Selected& e) {
        if (selected_item) {
            selected_item->removeFlags(GUI_FLAG_SELECTED);
            selected_item->invoke(GuiEvt_Deselected{ selected_item });
            selected_item = nullptr;
        }
        selected_item = dynamic_cast<GuiTreeItem*>(e.elem);
        LOG_DBG("GuiTreeView: selected");
    });
}

GuiTreeItem* GuiTreeView::addItem(const char* name) {
    auto item = new GuiTreeItem(name);
    addChild(item);
    return item;
}

