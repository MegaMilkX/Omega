#pragma once

#include "gui/elements/element.hpp"
#include "gui/elements/tree_view.hpp"
#include "gui_engine/inspector.hpp"
#include "gui_engine/actor_node_selector.hpp"
#include "world/actor.hpp"


class GuiActorInspector : public GuiElement {
    Actor* actor = nullptr;

    GuiActorNodeSelector* node_type_selector = nullptr;

    GuiTreeView* tree_view = nullptr;

    GuiInspector* node_inspector = nullptr;
    std::unique_ptr<rtti::PropSnapshot> node_snap;
    std::unique_ptr<rtti::PropSnapshot> node_snap_delta;

    void initNodeView(ActorNode* node);
    void initNodeTreeView(GuiElement* elem, ActorNode* node);
    void initControls();

    void addNode(rtti::type type);
public:
    GuiActorInspector();

    void init(Actor* actor);
};

