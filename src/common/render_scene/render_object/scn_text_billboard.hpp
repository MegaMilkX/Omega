#pragma once

#include "scn_render_object.hpp"

#include "resource/resource.hpp"

#include "typeface/font.hpp"
#include "gpu/render/uniform.hpp"

#include "gpu/gpu_text.hpp"

class scnTextBillboard : public scnRenderObject {
    gpuMaterial* material = 0;
    gpuUniformBuffer* ubufText = 0;

    HSHARED<gpuTexture2d> tex_font_atlas;
    HSHARED<gpuTexture2d> tex_font_lookup;

    std::unique_ptr<gpuText> gpu_text;
    
    void onAdded() override {

    }
    void onRemoved() override {

    }
public:
    scnTextBillboard(Font* font) {
        gpu_text.reset(new gpuText(font));
        gpu_text->setString("PlayerName");
        gpu_text->commit(.0f, 0.01f);

        ktImage imgFontAtlas;
        ktImage imgFontLookupTexture;
        font->buildAtlas(&imgFontAtlas, &imgFontLookupTexture);
        
        tex_font_atlas.reset(HANDLE_MGR<gpuTexture2d>::acquire());
        tex_font_lookup.reset(HANDLE_MGR<gpuTexture2d>::acquire());
        tex_font_atlas->setData(&imgFontAtlas);
        tex_font_lookup->setData(&imgFontLookupTexture);
        tex_font_lookup->setFilter(GPU_TEXTURE_FILTER_NEAREST);

        ubufText = gpuGetPipeline()->createUniformBuffer(UNIFORM_BUFFER_TEXT);
        ubufText->setInt(ubufText->getDesc()->getUniform("lookupTextureWidth"), tex_font_lookup->getWidth());

        material = gpuGetPipeline()->createMaterial();
        auto tech = material->addTechnique("VFX");
        auto pass = tech->addPass();
        pass->setShader(resGet<gpuShaderProgram>("shaders/text.glsl"));
        material->addSampler("texAlbedo", tex_font_atlas);
        material->addSampler("texTextUVLookupTable", tex_font_lookup);
        material->addUniformBuffer(ubufText);
        material->compile();

        addRenderable(new gpuRenderable);
        getRenderable(0)->setMaterial(material);
        getRenderable(0)->setMeshDesc(gpu_text->getMeshDesc());
        getRenderable(0)->attachUniformBuffer(ubuf_model);
        getRenderable(0)->attachUniformBuffer(ubufText);
        getRenderable(0)->compile();
    }
};