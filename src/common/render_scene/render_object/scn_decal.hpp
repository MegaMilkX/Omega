#pragma once

#include "scn_render_object.hpp"

#include "resource/resource.hpp"

#include "platform/platform.hpp"


class scnRenderScene;
class scnDecal : public scnRenderObject {
    TYPE_ENABLE(scnRenderObject);

    friend scnRenderScene;

    gpuMaterial* material = 0;
    gpuUniformBuffer* ubufDecal = 0;
    gpuBuffer vertexBuffer;
    gpuMeshDesc meshDesc;

    // TODO: Does not need to be held by each decal?
    int rgba_uloc = 0;

    void onAdded() override {
        
    }
    void onRemoved() override {

    }
public:
    scnDecal() {
        HSHARED<gpuTexture2d> texture = resGet<gpuTexture2d>("textures/decals/pentagram.png");
        HSHARED<gpuShaderProgram> shader = resGet<gpuShaderProgram>("shaders/decal.glsl");

        gfxm::vec3 boxSize = gfxm::vec3(1.0f, 1.0f, 1.0f);
        float width = boxSize.x;
        float height = boxSize.y;
        float depth = boxSize.z;
        float w = width * .5f;
        float h = height * .5f;
        float d = depth * .5f;
        float vertices[] = {
            -w, -h,  d,     w,  h,  d,      w, -h,  d,
             w,  h,  d,    -w, -h,  d,     -w,  h,  d,

             w, -h,  d,     w,  h, -d,      w, -h, -d,
             w,  h, -d,     w, -h,  d,      w,  h,  d,

             w, -h, -d,    -w,  h, -d,     -w, -h, -d,
            -w,  h, -d,     w, -h, -d,      w,  h, -d,

            -w, -h, -d,    -w,  h,  d,     -w, -h,  d,
            -w,  h,  d,    -w, -h, -d,     -w,  h, -d,

            -w,  h,  d,     w,  h, -d,      w,  h,  d,
             w,  h, -d,    -w,  h,  d,     -w,  h, -d,

            -w, -h, -d,     w, -h,  d,      w, -h, -d,
             w, -h,  d,    -w, -h, -d,     -w, -h,  d
        };
        vertexBuffer.setArrayData(vertices, sizeof(vertices));
        meshDesc.setAttribArray(VFMT::Position_GUID, &vertexBuffer);
        meshDesc.setDrawMode(MESH_DRAW_TRIANGLES);
        meshDesc.setVertexCount(36);

        material = gpuGetPipeline()->createMaterial();
        auto tech = material->addTechnique("Decals");
        auto pass = tech->addPass();
        pass->setShader(shader);
        pass->depth_write = 0;
        //pass->depth_test = 0;
        //pass->cull_faces = 0;
        pass->blend_mode = GPU_BLEND_MODE::ADD;
        material->addPassOutputSampler("Depth");
        material->addSampler("tex", texture);
        material->compile();        

        ubufDecal = gpuGetPipeline()->createUniformBuffer(UNIFORM_BUFFER_DECAL);
        ubufDecal->setVec3(ubufDecal->getDesc()->getUniform("boxSize"), boxSize);
        int screen_w = 0, screen_h = 0;
        platformGetWindowSize(screen_w, screen_h);
        gfxm::vec2 screenSize(screen_w, screen_h);
        ubufDecal->setVec2(ubufDecal->getDesc()->getUniform("screenSize"), screenSize);
        rgba_uloc = ubufDecal->getDesc()->getUniform("RGBA");
        ubufDecal->setVec4(rgba_uloc, gfxm::vec4(1, 1, 1, 1));

        addRenderable(new gpuRenderable);
        getRenderable(0)->attachUniformBuffer(ubufDecal);
        getRenderable(0)->attachUniformBuffer(ubuf_model);
        getRenderable(0)->setMaterial(material);
        getRenderable(0)->setMeshDesc(&meshDesc);
        getRenderable(0)->compile();
    }

    void setTexture(HSHARED<gpuTexture2d> texture) {
        material->addSampler("tex", texture);
        material->compile();
    }
    void setBoxSize(float x, float y, float z) {
        setBoxSize(gfxm::vec3(x, y, z));
    }
    void setBoxSize(const gfxm::vec3& boxSize) {
        ubufDecal->setVec3(ubufDecal->getDesc()->getUniform("boxSize"), boxSize);

        float width = boxSize.x;
        float height = boxSize.y;
        float depth = boxSize.z;
        float w = width * .5f;
        float h = height * .5f;
        float d = depth * .5f;
        float vertices[] = {
            -w, -h,  d,     w,  h,  d,      w, -h,  d,
             w,  h,  d,    -w, -h,  d,     -w,  h,  d,

             w, -h,  d,     w,  h, -d,      w, -h, -d,
             w,  h, -d,     w, -h,  d,      w,  h,  d,

             w, -h, -d,    -w,  h, -d,     -w, -h, -d,
            -w,  h, -d,     w, -h, -d,      w,  h, -d,

            -w, -h, -d,    -w,  h,  d,     -w, -h,  d,
            -w,  h,  d,    -w, -h, -d,     -w,  h, -d,

            -w,  h,  d,     w,  h, -d,      w,  h,  d,
             w,  h, -d,    -w,  h,  d,     -w,  h, -d,

            -w, -h, -d,     w, -h,  d,      w, -h, -d,
             w, -h,  d,    -w, -h, -d,     -w, -h,  d
        };
        vertexBuffer.setArrayData(vertices, sizeof(vertices));
    }
    void setColor(const gfxm::vec4& rgba) {
        ubufDecal->setVec4(rgba_uloc, rgba);
    }
    void setBlending(GPU_BLEND_MODE mode) {
        // TODO: Very bad, should at least get technique by name
        material->getTechniqueByLocalId(0)->getPass(0)->blend_mode = mode;
    }

    static void reflect() {
        type_register<scnDecal>("scnDecal")
            .parent<scnRenderObject>();
    }
};