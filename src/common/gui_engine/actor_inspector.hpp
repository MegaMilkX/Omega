#pragma once

#include "gui/elements/element.hpp"
#include "gui/elements/tree_view.hpp"
#include "gui/elements/menu_list.hpp"
#include "gui_engine/inspector.hpp"
#include "gui_engine/actor_node_selector.hpp"
#include "world/actor.hpp"


class GuiActorInspector : public GuiElement {
    Actor* actor = nullptr;

    GuiTreeView* tree_view = nullptr;
    std::vector<GuiTreeItem*> tree_items; // for simpler node search

    GuiInspector* node_inspector = nullptr;
    std::unique_ptr<rtti::PropSnapshot> node_snap;
    std::unique_ptr<rtti::PropSnapshot> node_snap_delta;

    void initNodeView(ActorNode* node);
    void initNodeTreeViewImpl(GuiElement* elem, ActorNode* node);
    void initNodeTreeView();
    void initControls();

    void openNodeSelector(GuiElement* scope, ActorNode* root);

    void addNode(rtti::type type, ActorNode* root);
    void selectNode(ActorNode* node);
public:
    GuiActorInspector();

    void init(Actor* actor);
};

