#pragma once

#include "scn_render_object.hpp"

#include "resource/resource.hpp"

#include "typeface/font.hpp"
#include "gpu/render/uniform.hpp"

#include "gpu/gpu_text.hpp"

class scnTextBillboard : public scnRenderObject {
    gpuMaterial* material = 0;

    HSHARED<gpuTexture2d> tex_font_atlas;
    HSHARED<gpuTexture2d> tex_font_lookup;

    std::unique_ptr<gpuText> gpu_text;

    const float scale = .005f;
    
    void onAdded() override {

    }
    void onRemoved() override {

    }
public:
    scnTextBillboard() {
        auto font = gpuGetAssetCache()->getDefaultFont();
        gpu_text.reset(new gpuText(font));
        gpu_text->setString("TextBillboard");
        gpu_text->commit(.0f, scale);

        ktImage imgFontAtlas;
        ktImage imgFontLookupTexture;
        font->buildAtlas(&imgFontAtlas, &imgFontLookupTexture);
        
        tex_font_atlas.reset(HANDLE_MGR<gpuTexture2d>::acquire());
        tex_font_lookup.reset(HANDLE_MGR<gpuTexture2d>::acquire());
        tex_font_atlas->setData(&imgFontAtlas);
        tex_font_lookup->setData(&imgFontLookupTexture);
        tex_font_lookup->setFilter(GPU_TEXTURE_FILTER_NEAREST);

        material = gpuGetPipeline()->createMaterial();
        auto tech = material->addTechnique("VFX");
        auto pass = tech->addPass();
        pass->setShader(resGet<gpuShaderProgram>("shaders/text.glsl"));
        pass->depth_write = false;
        pass->blend_mode = GPU_BLEND_MODE::ADD;
        material->addSampler("texAlbedo", tex_font_atlas);
        material->addSampler("texTextUVLookupTable", tex_font_lookup);
        
        material->compile();

        addRenderable(new gpuRenderable);
        getRenderable(0)->setMaterial(material);
        getRenderable(0)->setMeshDesc(gpu_text->getMeshDesc());
        getRenderable(0)->attachUniformBuffer(ubuf_model);
        getRenderable(0)->compile();
    }

    void setFont(const std::shared_ptr<Font>& fnt) {
        ktImage imgFontAtlas;
        ktImage imgFontLookupTexture;
        fnt->buildAtlas(&imgFontAtlas, &imgFontLookupTexture);

        tex_font_atlas->setData(&imgFontAtlas);
        tex_font_lookup->setData(&imgFontLookupTexture);
        tex_font_lookup->setFilter(GPU_TEXTURE_FILTER_NEAREST);

        gpu_text->setFont(fnt);
        gpu_text->commit(.0f, scale);

        getRenderable(0)->setMeshDesc(gpu_text->getMeshDesc());
        getRenderable(0)->compile();
    }
    void setText(const char* text) {
        gpu_text->setString(text);
        gpu_text->commit(.0f, scale);
        getRenderable(0)->setMeshDesc(gpu_text->getMeshDesc());
        getRenderable(0)->compile();
    }
    const char* getText() const {
        return gpu_text->getString();
    }
};