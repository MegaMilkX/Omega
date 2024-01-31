#pragma once

#include "node_text_billboard.auto.hpp"

#include "world/world.hpp"

#include "resource/resource.hpp"
#include "render_scene/render_object/scn_text_billboard.hpp"


[[cppi_class]];
class TextBillboardNode : public gameActorNode {
    scnNode scn_node;
    scnTextBillboard scn_text;
public:
    TYPE_ENABLE();
    TextBillboardNode()
    {}

    void setFont(std::shared_ptr<Font> fnt) {
        scn_text.setFont(fnt);
    }
    void setText(const char* text) {
        scn_text.setText(text);
    }

    void onDefault() override {
        scn_text.setNode(&scn_node);
        scn_text.setText("TextNode");
    }
    void onUpdateTransform() override {
        scn_node.local_transform = getWorldTransform();
        scn_node.world_transform = getWorldTransform();
    }
    void onUpdate(RuntimeWorld* world, float dt) override {}
    void onSpawn(RuntimeWorld* world) override {
        world->getRenderScene()->addRenderObject(&scn_text);
    }
    void onDespawn(RuntimeWorld* world) override {
        world->getRenderScene()->removeRenderObject(&scn_text);
    }

    [[cppi_decl, serialize_json]]
    void toJson(nlohmann::json& j) override {
        std::string txt = scn_text.getText();
        type_write_json(j["text"], txt);
        // TODO: FONT
    }
    [[cppi_decl, deserialize_json]]
    bool fromJson(const nlohmann::json& j) override {
        std::string txt;
        type_read_json(j["text"], txt);
        scn_text.setText(txt.c_str());
        return true;
    }
};