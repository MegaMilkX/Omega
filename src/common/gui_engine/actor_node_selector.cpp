#include "actor_node_selector.hpp"

#include "world/node/actor_node.hpp"


void GuiActorNodeSelector::initTypeTreeImpl(GuiElement* elem, rtti::type base_type) {
    if(base_type.is_constructible()) {
        auto item = guiCreate<GuiTreeItem>(base_type.get_name());
        elem->pushBack(item);
        item->subscribe<GuiEvt_Selected>([this, base_type](const GuiEvt_Selected& e) {
            selected_type = base_type;
            
            description->clearChildren();
            description->pushBack(
                std::format("{}\nsizeof: {}", selected_type.get_name(), selected_type.get_size())
            );

            e.invoke_next();
        });
    }
    for (auto t : base_type.get_desc()->derived_types) {
        initTypeTreeImpl(elem, t);
    }
}

void GuiActorNodeSelector::initTypeTree(GuiElement* elem, rtti::type base_type) {
    elem->clearChildren();
    for (auto t : base_type.get_desc()->derived_types) {
        initTypeTreeImpl(elem, t);
    }
}

void GuiActorNodeSelector::initControls() {    
    clearChildren();
    
    {
        auto create_node_list = guiCreate<GuiTreeView>();
        create_node_list->setSize(gui::fill(), gui::fill());
        rtti::type base_type = rtti::type_get<ActorNode>();
        initTypeTree(create_node_list, base_type);
        pushBack(create_node_list);
    }

    {
        description = guiCreate<GuiElement>();
        description->setSize(gui::fill(), gui::content());
        pushBack(description);
    }

    {
        auto buttons = guiCreate<GuiElement>();
        buttons->setSize(gui::fill(), gui::content());
        buttons->primary_axis = GUI_PRIMARY_AXIS::X;
        pushBack(buttons);

        auto btn_select = buttons->pushBack(guiCreate<GuiButton>("Select"));
        btn_select->subscribe<GuiEvt_LClick>([this](const GuiEvt_LClick& e) {
            invoke(GuiEvt_TypePicked{selected_type});
        });
        auto btn_cancel = buttons->pushBack(guiCreate<GuiButton>("Cancel"));
    }
}


GuiActorNodeSelector::GuiActorNodeSelector() {
    setSize(gui::px(400), gui::px(600));
    setStyleClasses({ "window" });

    initControls();
}

