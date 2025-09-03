#pragma once

#include "node_text_billboard.auto.hpp"

#include "world/world.hpp"
#include "render_scene/render_scene.hpp"

#include "resource/resource.hpp"
#include "render_scene/render_object/scn_text_billboard.hpp"


[[cppi_class]];
class TextBillboardNode : public TActorNode<scnRenderScene> {
    scnTextBillboard scn_text;
    std::shared_ptr<Font> font;
public:
    TYPE_ENABLE();
    TextBillboardNode()
    {
        scn_text.setTransformNode(getTransformHandle());
    }

    void setFont(const std::shared_ptr<Font>& fnt) {
        font = fnt;
        scn_text.setFont(fnt);
    }
    [[cppi_decl, set("text")]]
    void setText(const std::string& text) {
        scn_text.setText(text.c_str());
    }
    [[cppi_decl, get("text")]]
    std::string getText() const {
        return scn_text.getText();
    }

    void onDefault() override {
        scn_text.setText("TextNode");
    }
    void onUpdateTransform() override {}
    void onUpdate(RuntimeWorld* world, float dt) override {}
    void onSpawn(scnRenderScene* scn) override {
        scn->addRenderObject(&scn_text);
    }
    void onDespawn(scnRenderScene* scn) override {
        scn->removeRenderObject(&scn_text);
    }

    [[cppi_decl, serialize_json]]
    void toJson(nlohmann::json& j) override {
        std::string txt = scn_text.getText();
        type_write_json(j["text"], txt);
        if (font) {
            nlohmann::json jfont = nlohmann::json();
            type_write_json(jfont["typeface"], font->getTypeface()->filename);
            type_write_json(jfont["height"], font->getHeight());
            type_write_json(jfont["dpi"], font->getDpi());
            j["font"] = jfont;
        }
    }
    [[cppi_decl, deserialize_json]]
    bool fromJson(const nlohmann::json& j) override {
        std::string txt;
        type_read_json(j["text"], txt);
        scn_text.setText(txt.c_str());
        {
            auto jit = j.find("font");
            const nlohmann::json& jfont = jit.value();
            if (!jfont.is_null() && jfont.is_object()) {
                std::string str_typeface = "";
                int height = 0;
                int dpi = 0;
                type_read_json(jfont["typeface"], str_typeface);
                type_read_json(jfont["height"], height);
                type_read_json(jfont["dpi"], dpi);
                setFont(fontGet(str_typeface.c_str(), height, dpi));
            }
        }
        return true;
    }
};