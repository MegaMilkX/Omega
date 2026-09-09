#pragma once

#include "node_text_billboard.auto.hpp"

#include "world/world.hpp"
#include "world/common_systems/scene_system.hpp"

#include "gpu/gpu_text.hpp"

#include "resource/resource.hpp"


[[cppi_class]];
class TextBillboardNode : public TActorNode<SceneSystem>, public SceneProxy {
    std::unique_ptr<gpuRenderable> renderable;
    gpuTransformBlock* transform_block = nullptr;
    //gpuMaterial* material = 0;
    ResourceRef<gpuMaterial> material;
    ResourceRef<gpuTexture2d> tex_font_atlas;
    ResourceRef<gpuTexture2d> tex_font_lookup;
    std::unique_ptr<gpuText> gpu_text;

    const float scale = .005f;
    std::shared_ptr<Font> font;
public:
    TYPE_ENABLE();
    TextBillboardNode() {
        transform_block = gpuGetDevice()->createParamBlock<gpuTransformBlock>();

        auto font = gpuGetAssetCache()->getDefaultFont();
        gpu_text.reset(new gpuText(font));
        gpu_text->setString("TextBillboard");
        gpu_text->commit(.0f, scale);
        
        ktImage imgFontAtlas;
        ktImage imgFontLookupTexture;
        font->buildAtlas(&imgFontAtlas, &imgFontLookupTexture);
        
        tex_font_atlas = ResourceManager::get()->create<gpuTexture2d>("");
        tex_font_lookup = ResourceManager::get()->create<gpuTexture2d>("");
        tex_font_atlas->setData(&imgFontAtlas);
        tex_font_lookup->setData(&imgFontLookupTexture);
        tex_font_lookup->setFilter(GPU_TEXTURE_FILTER_NEAREST);

        material = loadResource<gpuMaterial>("materials/text");

        renderable.reset(new gpuRenderable);
        renderable->setMaterial(material.get());
        renderable->setMeshDesc(gpu_text->getMeshDesc());
        renderable->attachParamBlock(transform_block);
        renderable->addSamplerOverride("texTextUVLookupTable", tex_font_lookup);
        renderable->addSamplerOverride("texFontAtlas", tex_font_atlas);
        renderable->dbg_billboard = true;
        renderable->compile();

        gpuAddTransformSync(transform_block, getTransformHandle());
        setTransformNode(getTransformHandle());
        getTransformHandle()->setInheritFlags(FTransformInherit::Position | FTransformInherit::Scale);
    }

    void setFont(const std::shared_ptr<Font>& fnt) {
        font = fnt;

        ktImage imgFontAtlas;
        ktImage imgFontLookupTexture;
        fnt->buildAtlas(&imgFontAtlas, &imgFontLookupTexture);

        tex_font_atlas->setData(&imgFontAtlas);
        tex_font_lookup->setData(&imgFontLookupTexture);
        tex_font_lookup->setFilter(GPU_TEXTURE_FILTER_NEAREST);

        gpu_text->setFont(fnt);
        gpu_text->commit(.0f, scale);

        renderable->setMeshDesc(gpu_text->getMeshDesc());
        renderable->compile();
    }
    [[cppi_decl, set("text")]]
    void setText(const std::string& text) {
        gpu_text->setString(text);
        gpu_text->commit(.0f, scale);
        renderable->setMeshDesc(gpu_text->getMeshDesc());
        renderable->compile();
    }
    [[cppi_decl, get("text")]]
    std::string getText() const {
        return gpu_text->getString();
    }

    void onDefault() override {
        setText("TextNode");
    }
    void onSpawnActorNode(SceneSystem* scn) override {
        scn->addProxy(this);
    }
    void onDespawnActorNode(SceneSystem* scn) override {
        scn->removeProxy(this);
    }

    void updateBounds() override {
        gfxm::vec3 pos = getWorldTranslation();
        setBoundingBox(
            gfxm::aabb(pos + gfxm::vec3(-1, -1, -1), pos + gfxm::vec3(1, 1, 1))
        );
        setBoundingSphere(2.f, pos);
    }
    void submit(gpuRenderBucket* bucket) override {
        bucket->add(renderable.get());
    }

    [[cppi_decl, serialize_json]]
    void toJson(nlohmann::json& j) override {
        std::string txt = gpu_text->getString();
        rtti::type_write_json(j["text"], txt);
        if (font) {
            nlohmann::json jfont = nlohmann::json();
            rtti::type_write_json(jfont["typeface"], font->getTypeface()->filename);
            rtti::type_write_json(jfont["height"], font->getHeight());
            rtti::type_write_json(jfont["dpi"], font->getDpi());
            j["font"] = jfont;
        }
    }
    [[cppi_decl, deserialize_json]]
    bool fromJson(const nlohmann::json& j) override {
        std::string txt;
        rtti::type_read_json(j["text"], txt);
        setText(txt);
        {
            auto jit = j.find("font");
            const nlohmann::json& jfont = jit.value();
            if (!jfont.is_null() && jfont.is_object()) {
                std::string str_typeface = "";
                int height = 0;
                int dpi = 0;
                rtti::type_read_json(jfont["typeface"], str_typeface);
                rtti::type_read_json(jfont["height"], height);
                rtti::type_read_json(jfont["dpi"], dpi);
                setFont(fontGet(str_typeface.c_str(), height, dpi));
            }
        }
        return true;
    }
};