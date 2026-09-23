#include "actor_inspector.hpp"

#include "world/actor_ops.hpp"
#include "gui/elements/tree_view.hpp"
#include "gui/elements/collapsing_header.hpp"
#include "gui/elements/file_explorer.hpp"


struct GuiActorNodeDDPayload : public GuiElementDDPayload {
    ActorNode* node = nullptr;
    GuiActorNodeDDPayload(ActorNode* node, GuiElement* elem) : node(node), GuiElementDDPayload(elem) {}
};


void GuiActorInspector::initNodeView(ActorNode* node) {
    if (!node) {
        node_inspector->clearChildren();
        return;
    }

    node_snap.reset(new rtti::PropSnapshot);
    node_snap_delta.reset(new rtti::PropSnapshot);
    node->makeSnapshot(*node_snap.get());
    node_inspector->init(node, node_snap.get(), node_snap_delta.get());
}
void GuiActorInspector::initNodeTreeViewImpl(GuiElement* elem, ActorNode* node) {
    if (!node) {
        return;
    }

    auto item = guiCreate<GuiTreeItem>(std::format("{} [{}]", node->getName(), node->get_type().get_name()).c_str());
    item->user_ptr = node;

    if (!has_flags(node->getFlags(), FActorNode::TreeOwned)) {
        item->setLocked(true);
    }

    tree_items.push_back(item);

    guiDragSubscribe(item);
    
    item->subscribe([this, item, node](const GuiEvt_RClick& e) {
        // TODO: This is fucky but works
        item->invokeBubble(GuiEvt_Selected{item});

        auto menu = guiCreate<GuiMenuList>();

        guiAddTransientPopup(nullptr, menu, guiGetMousePos());

        auto btn_create = menu->addItem("Create child...", 0);
        btn_create->subscribe([this, menu, node](const GuiEvt_LClick&) {
            openAddNode(nullptr, node);
            guiRemoveTransientPopup(menu);
        });

        if(has_flags(node->getFlags(), FActorNode::TreeOwned)) {
            auto btn_replace = menu->addItem("Replace...", 0);
            btn_replace->subscribe([this, menu, node](const GuiEvt_LClick&) {
                openReplaceNode(nullptr, node);
                guiRemoveTransientPopup(menu);
            });
        }

        auto btn_duplicate = menu->addItem("Duplicate", 0);
        btn_duplicate->subscribe([this, node, menu](const GuiEvt_LClick&) {
            auto parent = node->getParent();
            if (!parent) {
                return;
            }

            rtti::PropSnapshot snap;
            node->makeSnapshot(snap);

            auto copy = parent->createChild(node->get_type());
            copy->applySnapshot(snap);

            initNodeTreeView();
            selectNode(copy);

            guiRemoveTransientPopup(menu);
        });
          
        if(has_flags(node->getFlags(), FActorNode::TreeOwned)) {
            auto btn_remove = menu->addItem("Remove", 0);
            btn_remove->subscribe([this, node, menu](const GuiEvt_LClick&) {
                auto parent = node->getParent();
                node->removeThis();
                initNodeTreeView();
                guiRemoveTransientPopup(menu);
                selectNode(parent);
            });
        }
    });

    item->subscribe([this, item, node](const GuiEvt_PullStart&) {
        if (node->isRoot()) {
            return;
        }
        guiDragStart(new GuiActorNodeDDPayload(node, item));
    });
    item->subscribe([this](const GuiEvt_PullStop&) {
        guiDragStop();
    });
    item->subscribe([this, node](const GuiEvt_DragDrop& e) {
        auto pld = guiDragGetPayload<GuiActorNodeDDPayload>();
        if (!pld) {
            return;
        }

        if (!ActorOps::reparentNode(node, pld->node)) {
            return;
        }

        initNodeTreeView();
        selectNode(pld->node);
    });

    item->setCollapsed(false);
    item->user_ptr = (void*)node;
    elem->pushBack(item);

    item->subscribe([this](const GuiEvt_Selected& e) {
        initNodeView(static_cast<ActorNode*>(e.elem->user_ptr));
        e.invoke_next();
    });

    item->clearChildren();
    for (int i = 0; i < node->childCount(); ++i) {
        initNodeTreeViewImpl(item, node->getChild(i));
    }
}
void GuiActorInspector::initNodeTreeView() {
    tree_view->clearChildren();
    tree_items.clear();
    initNodeTreeViewImpl(tree_view, actor->getRoot());
}

void GuiActorInspector::initControls() {
    clearChildren();
    {
        auto btn = guiCreate<GuiButton>("Save");
        pushBack(btn);
        btn->setWidth(gui::fill());
        btn->subscribe([this, btn](const GuiEvt_LClick& e) {
            GuiFileExplorerParams params = {
                .mode = GuiFileExplorerModeSave,
                .filters = {
                    { "Prefab", { "apf" } }
                }
            };
            auto popup = guiCreate<GuiFileExplorer>(params);
            guiAddTransientPopup(btn, popup);
            popup->subscribe([this, popup](const GuiEvt_FileConfirmed& e) {
                std::string fname = e.files[0];
                ActorPrefab prefab;
                actor->makePrefab(prefab);
                nlohmann::json json;
                prefab.toJson(json);
                std::ofstream f(fname);
                f << json.dump(4);
                guiRemoveTransientPopup(popup);
            });
        });
    }

    {
        self_inspector.reset(new InspectorState);
        actor->makeSnapshot(self_inspector->snap);
        self_inspector->inspector.init(actor, &self_inspector->snap, &self_inspector->delta);
        pushBack(&self_inspector->inspector);
        self_inspector->inspector.setSize(gui::fill(), gui::content());
    }

    {
        auto node_buttons = guiCreate<GuiElement>();
        node_buttons->primary_axis = GUI_PRIMARY_AXIS::X;
        node_buttons->setSize(gui::fill(), gui::content());
        
        auto btn_create_node = guiCreate<GuiButton>("Add node");
        node_buttons->pushBack(btn_create_node);
        btn_create_node->subscribe<GuiEvt_LClick>([this, btn_create_node](const GuiEvt_LClick& e) {
            openAddNode(btn_create_node, actor->getRoot());
        });
        
        pushBack(node_buttons);
    }

    tree_view = guiCreate<GuiTreeView>();
    tree_view->clearChildren();
    pushBack(tree_view);
    initNodeTreeView();
    
    node_inspector = guiCreate<GuiInspector>();
    node_inspector->setSize(gui::fill(), gui::content());
    pushBack(node_inspector);

    driver_inspectors.clear();
    pushBack("Drivers");
    for (int i = 0; i < actor->driverCount(); ++i) {
        auto drv = actor->getDriver(i);
        auto type = drv->get_type();
        GuiCollapsingHeader* header = guiCreate<GuiCollapsingHeader>(type.get_name());
        pushBack(header);
        header->setOpen(true);

        auto& di_ptr = driver_inspectors.emplace_back();
        di_ptr.reset(new InspectorState);
        drv->makeSnapshot(di_ptr->snap);
        di_ptr->inspector.init(drv, &di_ptr->snap, &di_ptr->delta);
        di_ptr->inspector.setSize(gui::fill(), gui::content());
        header->pushBack(&di_ptr->inspector);
    }
}

GuiActorNodeSelector* GuiActorInspector::openNodeSelector(GuiElement* scope) {
    if (guiIsTransientScopeRoot(scope)) {
        return nullptr;
    }

    auto node_type_selector = guiCreate<GuiActorNodeSelector>();
    
    guiAddTransientPopup(scope, node_type_selector);
    return node_type_selector;
}
void GuiActorInspector::openAddNode(GuiElement* scope, ActorNode* root) {
    auto selector = openNodeSelector(scope);
    selector->subscribe([this, selector, root](const GuiEvt_TypePicked& e) {
        guiRemoveTransientPopup(selector);
        addNode(e.type, root);
    });
}
void GuiActorInspector::openReplaceNode(GuiElement* scope, ActorNode* node) {    
    auto selector = openNodeSelector(scope);
    selector->subscribe([this, selector, node](const GuiEvt_TypePicked& e) {
        guiRemoveTransientPopup(selector);
        auto new_node = ActorOps::replaceNode(node, e.type);
        if (!new_node) {
            LOG_ERR("Failed to replace node '" << node->getName() << "': '" << node->get_type().get_name() << "' to '" << e.type.get_name() << "'");
            return;
        }
        initNodeTreeView();
        selectNode(new_node);
    });
}

void GuiActorInspector::addNode(rtti::type type, ActorNode* root) {
    if (!root) {
        root = actor->getRoot();
    }
    auto node = root->createChild(type);
    node->setName(type.get_name());
    initNodeTreeView();
    selectNode(node);
}
void GuiActorInspector::selectNode(ActorNode* node) {
    for (int i = 0; i < tree_items.size(); ++i) {
        auto item = tree_items[i];
        if (item->user_ptr != node) {
            continue;
        }

        // TODO: This is fucky but works
        item->invokeBubble(GuiEvt_Selected{item});
        tree_view->scrollTo(item);
        break;
    }
}


GuiActorInspector::GuiActorInspector() {
    setSize(gui::px(400), gui::px(600));
    setStyleClasses({ "window" });
}

void GuiActorInspector::init(Actor* actor) {
    this->actor = actor;
    initControls();
}