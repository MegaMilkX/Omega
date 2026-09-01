#include "actor_inspector.hpp"

#include "gui/elements/tree_view.hpp"
#include "gui/elements/collapsing_header.hpp"


void GuiActorInspector::initNodeView(ActorNode* node) {
    if (!node) {
        node_inspector->clearChildren();
    }

    node_snap.reset(new rtti::PropSnapshot);
    node_snap_delta.reset(new rtti::PropSnapshot);
    node->makeSnapshot(*node_snap.get());
    node_inspector->init(node, node_snap.get(), node_snap_delta.get());
}
void GuiActorInspector::initNodeTreeView(GuiElement* elem, ActorNode* node) {
    if (!node) {
        return;
    }

    auto item = guiCreate<GuiTreeItem>(std::format("{} [{}]", node->getName(), node->get_type().get_name()).c_str());
    item->setCollapsed(false);
    item->user_ptr = (void*)node;
    elem->pushBack(item);

    item->subscribe<GuiEvt_Selected>([this](const GuiEvt_Selected& e) {
        initNodeView(static_cast<ActorNode*>(e.elem->user_ptr));
        e.invoke_next();
    });

    item->clearChildren();
    for (int i = 0; i < node->childCount(); ++i) {
        initNodeTreeView(item, node->getChild(i));
    }
}

void GuiActorInspector::initControls() {
    {
        auto node_buttons = guiCreate<GuiElement>();
        node_buttons->primary_axis = GUI_PRIMARY_AXIS::X;
        node_buttons->setSize(gui::fill(), gui::content());
        
        auto btn_create_node = guiCreate<GuiButton>("Add node");
        node_buttons->pushBack(btn_create_node);
        btn_create_node->subscribe<GuiEvt_LClick>([this, btn_create_node](const GuiEvt_LClick& e) {
            if (!node_type_selector) {
                guiAddTransientScope(btn_create_node, GUI_TRANSIENT_SCOPE_POP);

                node_type_selector = guiCreate<GuiActorNodeSelector>();
                node_type_selector->setOwner(btn_create_node);
                guiGetRoot()->pushBack(node_type_selector);

                node_type_selector->subscribe<GuiEvt_TypePicked>([this, btn_create_node](const GuiEvt_TypePicked& e) {
                    guiGetRoot()->removeChild(node_type_selector);
                    node_type_selector = nullptr;
                    guiRemoveTransientScope(btn_create_node);
                    addNode(e.type);
                });
            }
        });
        btn_create_node->subscribe<GuiEvt_ScopeLeft>([this](const GuiEvt_ScopeLeft&) {
            if(node_type_selector) {
                guiGetRoot()->removeChild(node_type_selector);
                node_type_selector = nullptr;
            }
        });
        
        pushBack(node_buttons);
    }

    tree_view = guiCreate<GuiTreeView>();
    tree_view->clearChildren();
    pushBack(tree_view);
    initNodeTreeView(tree_view, actor->getRoot());
    
    node_inspector = guiCreate<GuiInspector>();
    node_inspector->setSize(gui::fill(), gui::content());
    pushBack(node_inspector);

    pushBack("Drivers");
    for (int i = 0; i < actor->driverCount(); ++i) {
        auto drv = actor->getDriver(i);
        auto type = drv->get_type();
        GuiCollapsingHeader* header = guiCreate<GuiCollapsingHeader>(type.get_name());
        pushBack(header);
        header->setOpen(true);
        //buildPropertyUI(header, drv, type);
    }
}

void GuiActorInspector::addNode(rtti::type type) {
    auto node = actor->getRoot()->createChild(type);
    node->setName(type.get_name());
    tree_view->clearChildren();
    initNodeTreeView(tree_view, actor->getRoot());
}


GuiActorInspector::GuiActorInspector() {
    setSize(gui::px(400), gui::px(600));
    setStyleClasses({ "window" });
}

void GuiActorInspector::init(Actor* actor) {
    this->actor = actor;
    initControls();
}