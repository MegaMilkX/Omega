#include "game_test.hpp"

#include <stack>

#include "platform/gl/glextutil.h"

#include "platform/platform.hpp"

#include "mesh3d/generate_primitive.hpp"

#include <algorithm>
#include <unordered_map>

#include "gpu/gpu_pipeline.hpp"

#include "math/bezier.hpp"

#include "gpu/render_bucket.hpp"


gfxm::vec3 hsv2rgb(float H, float S, float V) {
    if (H > 360 || H < 0 || S>100 || S < 0 || V>100 || V < 0) {
        // TODO: assert?
        return gfxm::vec3(0,0,0);
    }
    float s = S / 100;
    float v = V / 100;
    float C = s * v;
    float X = C * (1 - abs(fmod(H / 60.0, 2) - 1));
    float m = v - C;
    float r, g, b;
    if (H >= 0 && H < 60) {
        r = C, g = X, b = 0;
    }
    else if (H >= 60 && H < 120) {
        r = X, g = C, b = 0;
    }
    else if (H >= 120 && H < 180) {
        r = 0, g = C, b = X;
    }
    else if (H >= 180 && H < 240) {
        r = 0, g = X, b = C;
    }
    else if (H >= 240 && H < 300) {
        r = X, g = 0, b = C;
    }
    else {
        r = C, g = 0, b = X;
    }
    int R = (r + m) * 255;
    int G = (g + m) * 255;
    int B = (b + m) * 255;

    return gfxm::vec3(R / 255.0f, G / 255.0f, B / 255.0f);
}


class SpriteAtlas {
public:
    struct spr {
        gfxm::vec2 tex_min;
        gfxm::vec2 tex_max;
        gfxm::vec2 origin;
    };

    HSHARED<gpuTexture2d> texture;
    gpuBuffer vertexBuffer;
    gpuBuffer uvBuffer;

private:
    std::vector<spr> spr_sheet;
    gfxm::vec2 atlas_size;
    int sprite_count;

public:
    HSHARED<gpuTexture2d> getTexture() const {
        return texture;
    }
    const gfxm::vec2& getTextureSize() const {
        return atlas_size;
    }
    const spr& getSprite(int i) const {
        return spr_sheet[i];
    }
    int getSpriteCount() const {
        return sprite_count;
    }

    void init() {
        ktImage img;
        loadImage(&img, "light_003.png");
        texture.reset(HANDLE_MGR<gpuTexture2d>::acquire());
        texture->setData(&img);

        atlas_size = gfxm::vec2(960.f, 1152.f);

        spr_sheet = {
            { { .0f, .0f }, { 192.f, 192.f }, { 96.f, 96.f } },
            { { 192.f, .0f }, { 384.f, 192.f }, { 288.f, 96.f } },
            { { 384.f, .0f }, { 576.f, 192.f }, { 480.f, 96.f } },
            { { 576.f, .0f }, { 768.f, 192.f }, { 672.f, 96.f } },
            { { 768.f, .0f }, { 960.f, 192.f }, { 864.f, 96.f } },

            { { .0f, 192.0f }, { 192.f, 384.f }, { 96.f, 288.f } },
            { { 192.f, 192.0f }, { 384.f, 384.f }, { 288.f, 288.f } },
            { { 384.f, 192.0f }, { 576.f, 384.f }, { 480.f, 288.f } },
            { { 576.f, 192.0f }, { 768.f, 384.f }, { 672.f, 288.f } },
            { { 768.f, 192.0f }, { 960.f, 384.f }, { 864.f, 288.f } },

            { { .0f, 384.f }, { 192.f, 576.f }, { 96.f, 480.f } },
            { { 192.f, 384.f }, { 384.f, 576.f }, { 288.f, 480.f } },
            { { 384.f, 384.f }, { 576.f, 576.f }, { 480.f, 480.f } },
            { { 576.f, 384.f }, { 768.f, 576.f }, { 672.f, 480.f } },
            { { 768.f, 384.f }, { 960.f, 576.f }, { 864.f, 480.f } },

            { { .0f, 576.f }, { 192.f, 768.f }, { 96.f, 672.f } },
            { { 192.f, 576.f }, { 384.f, 768.f }, { 288.f, 672.f } },
            { { 384.f, 576.f }, { 576.f, 768.f }, { 480.f, 672.f } },
            { { 576.f, 576.f }, { 768.f, 768.f }, { 672.f, 672.f } },
            { { 768.f, 576.f }, { 960.f, 768.f }, { 864.f, 672.f } },

            { { .0f, 768.f }, { 192.f, 960.f }, { 96.f, 864.f } },
            { { 192.f, 768.f }, { 384.f, 960.f }, { 288.f, 864.f } },
            { { 384.f, 768.f }, { 576.f, 960.f }, { 480.f, 864.f } },
            { { 576.f, 768.f }, { 768.f, 960.f }, { 672.f, 864.f } },
            { { 768.f, 768.f }, { 960.f, 960.f }, { 864.f, 864.f } },

            { { .0f, 960.f }, { 192.f, 1152.f }, { 96.f, 1056.f } },
            { { 192.f, 960.f }, { 384.f, 1152.f }, { 288.f, 1056.f } },
            { { 384.f, 960.f }, { 576.f, 1152.f }, { 480.f, 1056.f } },
            { { 576.f, 960.f }, { 768.f, 1152.f }, { 672.f, 1056.f } },
            { { 768.f, 960.f }, { 960.f, 1152.f }, { 864.f, 1056.f } }
        };
        sprite_count = spr_sheet.size();

        std::vector<gfxm::vec3> vertices;
        vertices.resize(sprite_count * 4);
        std::vector<gfxm::vec2> uvs;
        uvs.resize(sprite_count * 4);

        for (int i = 0; i < sprite_count; ++i) {
            spr& s = spr_sheet[i];

            gfxm::vec2 sz = s.tex_max - s.tex_min;
            gfxm::vec2 origin = s.tex_max - s.origin;
            sz *= .01f;
            origin *= .01f;
            gfxm::vec2 min(-origin.x, -origin.y);
            gfxm::vec2 max(sz.x - origin.x, sz.y - origin.y);
            vertices[i * 4] = gfxm::vec3(min.x, min.y, 0);
            vertices[i * 4 + 1] = gfxm::vec3(max.x, min.y, 0);
            vertices[i * 4 + 2] = gfxm::vec3(min.x, max.y, 0);
            vertices[i * 4 + 3] = gfxm::vec3(max.x, max.y, 0);

            gfxm::vec2 tex_min(s.tex_min.x, atlas_size.y - s.tex_min.y);
            gfxm::vec2 tex_max(s.tex_max.x, atlas_size.y - s.tex_max.y);
            gfxm::vec2 uv_min = tex_min / atlas_size;
            gfxm::vec2 uv_max = tex_max / atlas_size;
            uvs[i * 4] = gfxm::vec2(uv_min.x, uv_max.y);
            uvs[i * 4 + 1] = gfxm::vec2(uv_max.x, uv_max.y);
            uvs[i * 4 + 2] = gfxm::vec2(uv_min.x, uv_min.y);
            uvs[i * 4 + 3] = gfxm::vec2(uv_max.x, uv_min.y);
        }

        vertexBuffer.setArrayData(vertices.data(), vertices.size() * sizeof(vertices[0]));
        uvBuffer.setArrayData(uvs.data(), uvs.size() * sizeof(uvs[0]));
    }
};

struct ParticleEmitter {
    const float particlesPerSecond = 240.0f;
    float timeCache = .0f;

    std::vector<gfxm::vec4> particlePositions;
    std::vector<gfxm::vec4> particleData;
    std::vector<gfxm::vec4> particleColors;
    std::vector<gfxm::vec4> particleSpriteData;
    std::vector<gfxm::vec4> particleSpriteUV;

    float maxLifetime = 2.0f;
    int maxParticles = 1000;
    int alive_count = 0;

    gpuBuffer posBuffer;
    gpuBuffer particleDataBuffer;
    gpuBuffer particleColorBuffer;
    gpuBuffer particleSpriteDataBuffer;
    gpuBuffer particleSpriteUVBuffer;

    gpuTexture2d texture;

    SpriteAtlas* atlas = 0;

    gpuBuffer vertexBuffer;
    gpuBuffer uvBuffer;
    gpuMeshDesc meshDesc;
    gpuInstancingDesc instDesc;
    HSHARED<gpuShaderProgram> prog;
    gpuMaterial* mat = 0;
    std::unique_ptr<gpuRenderable> renderable;

    void init(SpriteAtlas* atlas) {
        this->atlas = atlas;

        ktImage img;
        loadImage(&img, "textures/particles/particle_star.png");
        texture.setData(&img);

        particlePositions.resize(maxParticles);
        particleData.resize(maxParticles);
        particleColors.resize(maxParticles);
        particleSpriteData.resize(maxParticles);
        particleSpriteUV.resize(maxParticles);
        for (int i = 0; i < maxParticles; ++i) {
            particlePositions[i] = gfxm::vec4(
                (rand() % 100 * 0.01f - 0.5f) * 10.0f,
                (rand() % 100 * 0.01f) * 10.0f,
                (rand() % 100 * 0.01f - 0.5f) * 10.0f,
                .0f
            );
            float lifetime = rand() % 100 * 0.01f * maxLifetime;
            particleData[i] = gfxm::vec4(
                .0f, .0f, .0f, lifetime
            );
            int sprite_id = (atlas->getSpriteCount() - 1) * (lifetime / maxLifetime);
            auto spr = atlas->getSprite(sprite_id);
            particleSpriteData[i] = gfxm::vec4(
                spr.tex_max.x - spr.tex_min.x,
                spr.tex_max.y - spr.tex_min.y,
                (spr.tex_max.x) - spr.origin.x,
                (spr.tex_max.y) - spr.origin.y
            );
            particleSpriteData[i] *= .01f;
            particleSpriteUV[i] = gfxm::vec4(
                spr.tex_min.x / atlas->getTextureSize().x,
                (atlas->getTextureSize().y - spr.tex_max.y) / atlas->getTextureSize().y,
                spr.tex_max.x / atlas->getTextureSize().x,
                (atlas->getTextureSize().y - spr.tex_min.y) / atlas->getTextureSize().y
            );
        }

        //---
        prog = resGet<gpuShaderProgram>("shaders/particle.glsl");

        float vertices[] = { 0, 0, 0, 1.f, 0, 0,    0, 1.f, 0, 1.f, 1.f, 0 };
        float uvs[] = { .0f, .0f, 1.f, .0f,   .0f, 1.f, 1.f, 1.f };
        vertexBuffer.setArrayData(vertices, sizeof(vertices));
        uvBuffer.setArrayData(uvs, sizeof(uvs));
        
        meshDesc.setAttribArray(VFMT::Position_GUID, &vertexBuffer);
        meshDesc.setAttribArray(VFMT::UV_GUID, &uvBuffer);
        meshDesc.setVertexCount(4);
        meshDesc.setDrawMode(MESH_DRAW_TRIANGLE_STRIP);

        instDesc.setInstanceAttribArray(VFMT::ParticlePosition_GUID, &posBuffer);
        instDesc.setInstanceAttribArray(VFMT::ParticleData_GUID, &particleDataBuffer);
        instDesc.setInstanceAttribArray(VFMT::ParticleColorRGBA_GUID, &particleColorBuffer);
        instDesc.setInstanceAttribArray(VFMT::ParticleSpriteData_GUID, &particleSpriteDataBuffer);
        instDesc.setInstanceAttribArray(VFMT::ParticleSpriteUV_GUID, &particleSpriteUVBuffer);

        mat = gpuGetPipeline()->createMaterial();
        mat->addSampler("tex", atlas->texture);
        auto tech = mat->addTechnique("VFX");
        auto pass = tech->addPass();
        pass->setShader(prog);
        pass->blend_mode = GPU_BLEND_MODE::ADD;
        pass->depth_write = 0;
        //pass->cull_faces = false;
        mat->compile();

        renderable.reset(new gpuRenderable(mat, &meshDesc, &instDesc));
    }

    void update(float dt) {
        timeCache += dt;
        int nParticlesToEmit = (int)(particlesPerSecond * timeCache);
        if (nParticlesToEmit > 0) {
            timeCache = .0f;
        }

        static float time = .0f;
        time += dt;
        float emitterRotationRadius = sinf(time * .05f) * 20.0f;
        gfxm::vec3 emitterPos = gfxm::vec3(sinf(time) * emitterRotationRadius, .0f, cosf(time) * emitterRotationRadius);
        for (int i = 0; i < nParticlesToEmit; ++i) {
            if (maxParticles == alive_count) {
                break;
            }
            int new_particle_id = alive_count;

            //float val = (time - 10.0f * (float)(int)(time / 10.0f)) / 10.0f;
            //gfxm::vec3 pos = gfxm::vec3(0, 0, 0);
            
            gfxm::vec3 pos = bezierCubic(
                gfxm::vec3(-5.0f, .0f, .0f),
                gfxm::vec3(-5.0f, .0f, -10.0f),
                gfxm::vec3(-7.0f, 8.f, -20.0f),
                gfxm::vec3(4.0f, 3.f, 20.0f),
                rand() % 100 * 0.01f
            );

            particlePositions[new_particle_id] = gfxm::vec4(
                pos,
                .0f
            );
            particleData[new_particle_id] = gfxm::vec4(
                (rand() % 100 * 0.01f - 0.5f) * 1.1f,
                (rand() % 100 * 0.01f) * 2.f,
                (rand() % 100 * 0.01f - 0.5f) * 1.1f,
                .0f
            );
            float& lifetime = particleData[new_particle_id].w;
            lifetime = .0f;

            int sprite_id = (atlas->getSpriteCount() - 1) * (lifetime / maxLifetime);
            auto spr = atlas->getSprite(sprite_id);
            particleSpriteData[new_particle_id] = gfxm::vec4(
                spr.tex_max.x - spr.tex_min.x,
                spr.tex_max.y - spr.tex_min.y,
                (spr.tex_max.x) - spr.origin.x,
                (spr.tex_max.y) - spr.origin.y
            );
            particleSpriteData[new_particle_id] *= .01f;
            particleSpriteUV[new_particle_id] = gfxm::vec4(
                spr.tex_min.x / atlas->getTextureSize().x,
                (atlas->getTextureSize().y - spr.tex_max.y) / atlas->getTextureSize().y,
                spr.tex_max.x / atlas->getTextureSize().x,
                (atlas->getTextureSize().y - spr.tex_min.y) / atlas->getTextureSize().y
            );

            nParticlesToEmit--;
            alive_count++;
        }
        for (int i = 0; i < alive_count; ++i) {
            float& lifetime = particleData[i].w;

            float& size = particlePositions[i].w;
            size = (lifetime / maxLifetime) * 1.0f;

            gfxm::vec3 pos = particlePositions[i];
            pos += gfxm::vec3(particleData[i]) * dt;
            particlePositions[i] = gfxm::vec4(pos, size);

            gfxm::vec3 col = hsv2rgb(250.0f * (1.0f - lifetime / maxLifetime), 100.0f, 100.0f);
            float alpha = 1;// (1.0f - lifetime / maxLifetime);
            particleColors[i] = gfxm::vec4(1, 1, 1, alpha);;// gfxm::vec4(col, (1.0f - lifetime / maxLifetime));

            int sprite_id = (atlas->getSpriteCount() - 1) * (lifetime / maxLifetime);
            sprite_id %= atlas->getSpriteCount();
            auto spr = atlas->getSprite(sprite_id);
            particleSpriteData[i] = gfxm::vec4(
                spr.tex_max.x - spr.tex_min.x,
                spr.tex_max.y - spr.tex_min.y,
                (spr.tex_max.x) - spr.origin.x,
                (spr.tex_max.y) - spr.origin.y
            );
            particleSpriteData[i] *= .01f;
            particleSpriteUV[i] = gfxm::vec4(
                spr.tex_min.x / atlas->getTextureSize().x,
                (atlas->getTextureSize().y - spr.tex_max.y) / atlas->getTextureSize().y,
                spr.tex_max.x / atlas->getTextureSize().x,
                (atlas->getTextureSize().y - spr.tex_min.y) / atlas->getTextureSize().y
            );

            lifetime += dt;

            if (lifetime > maxLifetime) {
                int last_alive = alive_count - 1;
                alive_count--;

                particleData[i] = particleData[last_alive];
                particlePositions[i] = particlePositions[last_alive];
                particleColors[i] = particleColors[last_alive];
                particleSpriteData[i] = particleSpriteData[last_alive];
                particleSpriteUV[i] = particleSpriteUV[last_alive];
            }
        }

        instDesc.setInstanceCount(alive_count);
        posBuffer.setArrayData(particlePositions.data(), particlePositions.size() * sizeof(particlePositions[0]));
        particleDataBuffer.setArrayData(particleData.data(), particleData.size() * sizeof(particleData[0]));
        particleColorBuffer.setArrayData(particleColors.data(), particleColors.size() * sizeof(particleColors[0]));
        particleSpriteDataBuffer.setArrayData(particleSpriteData.data(), particleSpriteData.size() * sizeof(particleSpriteData[0]));
        particleSpriteUVBuffer.setArrayData(particleSpriteUV.data(), particleSpriteUV.size() * sizeof(particleSpriteUV[0]));
    }

    void draw(gpuRenderBucket* bucket) {
        bucket->add(renderable.get());
    }
};

#include "FastNoiseSIMD/FastNoiseSIMD.h"
#include <random>

#include "particle_emitter/particle_emitter_master.hpp"

class PolygonTrail {
public:
    HSHARED<gpuShaderProgram> prog;
    static const int pointCount = 100;
    gfxm::vec3 points[pointCount];
    gfxm::vec3 origin;
    gfxm::vec3 head_pos;
    gfxm::vec3 tail_pos;
    float thickness = .2f;
    RHSHARED<gpuTexture2d> texture;
    const float max_segment_distance = .025f;
    float distanceTraveled = .0f;
    float total_distance_traveled = .0f;
    float uv_offset;
    gfxm::vec3 prev_position;
    float hue_ = .0f;

    void init() {
        prog = resGet<gpuShaderProgram>("shaders/trail.glsl");

        texture = resGet<gpuTexture2d>("trail.jpg");

        prev_position = head_pos;
        tail_pos = head_pos;
        points[0] = head_pos;
        for (int i = 1; i < pointCount; ++i) {
            points[i] = points[0];
        }
    }
    void update(float dt) {
        static float time = .0f;
        time += dt * 3.f;
        gfxm::vec3 prevPos = head_pos;
        head_pos = gfxm::vec3(sinf(time) * 1.0f, sinf(time * .15f) * 0.8f + 1.0f, cosf(time) * 1.0f);
        head_pos += origin;
        distanceTraveled += gfxm::length(head_pos - prevPos);
        total_distance_traveled += gfxm::length(head_pos - prevPos);
        uv_offset = total_distance_traveled / max_segment_distance;
        points[0] = head_pos;
        if (distanceTraveled >= max_segment_distance) {
            distanceTraveled = .0f;
            
            tail_pos = points[pointCount - 1];
            for (int i = pointCount - 2; i > 0; --i) {
                points[i] = points[i - 1];
            }
            
            prev_position = head_pos;
        }
        points[pointCount - 1] = gfxm::lerp(tail_pos, points[pointCount - 2], distanceTraveled / max_segment_distance);
        /*
        for (int i = pointCount - 1; i > 0; --i) {
            float len = gfxm::sqrt(gfxm::_max(.0f, gfxm::_min(1.f, gfxm::length(points[i - 1] - points[i]))));
            points[i] = gfxm::lerp(points[i], points[i - 1], len);
        }
        points[0] = position;*/

        hue_ += dt * 20.0f;
        if (hue_ > 300.0f) {
            hue_ -= 600.0f;
        }
    }
    void draw(const gfxm::mat4& view, const gfxm::mat4& proj) {
        std::vector<gfxm::vec3> vertices;
        vertices.resize(pointCount * 2);
        std::vector<gfxm::vec2> uvs;
        uvs.resize(pointCount * 2);
        std::vector<unsigned char> color;
        color.resize(pointCount * 2 * 4);

        gfxm::mat4 cam = gfxm::inverse(view);
        float half_thickness = thickness * .5f;
        for (int i = 0; i < pointCount - 1; ++i) {
            gfxm::vec3 trailN = gfxm::normalize(points[i + 1] - points[i]);
            gfxm::vec3 camN = gfxm::normalize(gfxm::vec3(cam[3]) - points[i]);
            vertices[i * 2] = points[i] + gfxm::normalize(gfxm::cross(camN, trailN)) * half_thickness;
            vertices[i * 2 + 1] = points[i] - gfxm::normalize(gfxm::cross(camN, trailN)) * half_thickness;
        }
        gfxm::vec3 trailN = gfxm::normalize(points[pointCount - 1] - points[pointCount - 2]);
        int lastId = pointCount - 1;
        gfxm::vec3 camN = gfxm::normalize(gfxm::vec3(cam[3]) - points[lastId]);
        vertices[lastId * 2] = points[lastId] + gfxm::normalize(gfxm::cross(gfxm::vec3(cam[2]), trailN)) * half_thickness;
        vertices[lastId * 2 + 1] = points[lastId] - gfxm::normalize(gfxm::cross(gfxm::vec3(cam[2]), trailN)) * half_thickness;

        uvs[0] = gfxm::vec2(.0f, .0f);
        uvs[1] = gfxm::vec2(.0f, 1.f);
        float div_point_count = 1.0f / (float)(pointCount - 2);
        for (int i = 1; i < pointCount - 1; ++i) {
            //float u = (float)i / (float)(pointCount - 1) * 10.0f - distanceTraveled / 10.0f;
            //float u = (float)i / (float)(pointCount - 1);
            //float u = (float)i + (distanceTraveled / max_segment_distance);
            float u = ((float)i - 1.0f + (distanceTraveled / max_segment_distance)) * div_point_count;
            uvs[i * 2] = gfxm::vec2(u, .0f);
            uvs[i * 2 + 1] = gfxm::vec2(u, 1.f);
        }
        float u = uvs[(pointCount - 2) * 2].x + (1.0f - distanceTraveled / max_segment_distance) * div_point_count;
        uvs[(pointCount - 1) * 2] = gfxm::vec2(u, .0f);
        uvs[(pointCount - 1) * 2 + 1] = gfxm::vec2(u, 1.f);

        for (int i = 0; i < pointCount; ++i) {
            unsigned char a = 255 * (1.0f - (float)i / (float)(pointCount - 1));
            float d = i / (float)(pointCount - 1);
            float hue = gfxm::lerp(fabsf(hue_), fabsf(hue_) + 50.0f, d);
            gfxm::vec3 col = hsv2rgb(hue, 100.0f, 100.0f);
            int ia = i * 4 * 2;
            int ib = ia + 4;
            color[ia] = col.x * 255;
            color[ia + 1] = col.y * 255;
            color[ia + 2] = col.z * 255;
            color[ia + 3] = 255.f;

            color[ib] = col.x * 255;
            color[ib + 1] = col.y * 255;
            color[ib + 2] = col.z * 255;
            color[ib + 3] = 255.f;
        }

        gpuBuffer vertexBuffer;
        vertexBuffer.setArrayData(vertices.data(), vertices.size() * sizeof(vertices[0]));
        gpuBuffer uvBuffer;
        uvBuffer.setArrayData(uvs.data(), uvs.size() * sizeof(uvs[0]));
        gpuBuffer colorBuffer;
        colorBuffer.setArrayData(color.data(), color.size() * sizeof(color[0]));

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.getId());
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glBindBuffer(GL_ARRAY_BUFFER, uvBuffer.getId());
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

        glBindBuffer(GL_ARRAY_BUFFER, colorBuffer.getId());
        glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);

        glUseProgram(prog->getId());
        glUniformMatrix4fv(prog->getUniformLocation("matView"), 1, GL_FALSE, (float*)&view);
        glUniformMatrix4fv(prog->getUniformLocation("matProjection"), 1, GL_FALSE, (float*)&proj);
        gfxm::mat4 lval = gfxm::mat4(1.f);
        glUniformMatrix4fv(prog->getUniformLocation("matModel"), 1, GL_FALSE, (float*)&lval);
        glUniform1i(prog->getUniformLocation("tex"), 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture->getId());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, pointCount * 2);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
    }
};

class SpriteBillboard {
    HSHARED<gpuShaderProgram> prog;
    RHSHARED<gpuTexture2d> texture;

public:
    gfxm::vec3 origin;

    void init() {
        prog = resGet<gpuShaderProgram>("shaders/sprite_billboard.glsl");
        texture = resGet<gpuTexture2d>("icon_sprite_test.png");
    }
    void update(float dt) {
    }
    void draw(const gfxm::mat4& view, const gfxm::mat4& proj) {
        float vertices[] = {
                -.5f, -.5f, 0, .5f, -.5f, 0,
                -.5f, .5f, 0, .5f, .5f, 0
        };
        float uvs[] = {
                .0f, .0f, 1.f, .0f,
                .0f, 1.f, 1.f, 1.f
        };
        char colors[] = {
            255, 255, 255, 255,     255, 255, 255, 255,
            255, 255, 255, 255,     255, 255, 255, 255
        };
        gpuBuffer vertexBuffer;
        vertexBuffer.setArrayData(vertices, sizeof(vertices));
        gpuBuffer uvBuffer;
        uvBuffer.setArrayData(uvs, sizeof(uvs));
        gpuBuffer colorBuffer;
        colorBuffer.setArrayData(colors, sizeof(colors));        
        
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.getId());
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glBindBuffer(GL_ARRAY_BUFFER, uvBuffer.getId());
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

        glBindBuffer(GL_ARRAY_BUFFER, colorBuffer.getId());
        glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);

        gfxm::mat4 model = gfxm::translate(gfxm::mat4(1.0f), origin);

        glUseProgram(prog->getId());
        glUniformMatrix4fv(prog->getUniformLocation("matView"), 1, GL_FALSE, (float*)&view);
        glUniformMatrix4fv(prog->getUniformLocation("matProjection"), 1, GL_FALSE, (float*)&proj);
        glUniformMatrix4fv(prog->getUniformLocation("matModel"), 1, GL_FALSE, (float*)&model);
        glUniform1i(prog->getUniformLocation("tex"), 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture->getId());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
    }
};

class SpriteBillboardAnimated {
    HSHARED<gpuShaderProgram> prog;
    SpriteAtlas* atlas = 0;
    int sprite_id = 0;

public:
    gfxm::vec3 origin;

    void init(SpriteAtlas* atlas) {
        prog = resGet<gpuShaderProgram>("shaders/sprite_billboard_animated.glsl");
        this->atlas = atlas;
    }
    void update(float dt) {
        static float time = .0f;
        
        sprite_id = (int)(time / (1.0f / 15.0f)) % atlas->getSpriteCount();

        // TODO
        
        time += dt;
    }
    void draw(const gfxm::mat4& view, const gfxm::mat4& proj) {
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, atlas->vertexBuffer.getId());
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glBindBuffer(GL_ARRAY_BUFFER, atlas->uvBuffer.getId());
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);

        gfxm::mat4 model = gfxm::translate(gfxm::mat4(1.0f), origin);

        glUseProgram(prog->getId());
        glUniformMatrix4fv(prog->getUniformLocation("matView"), 1, GL_FALSE, (float*)&view);
        glUniformMatrix4fv(prog->getUniformLocation("matProjection"), 1, GL_FALSE, (float*)&proj);
        glUniformMatrix4fv(prog->getUniformLocation("matModel"), 1, GL_FALSE, (float*)&model);
        glUniform1i(prog->getUniformLocation("tex"), 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, atlas->texture->getId());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glDrawArrays(GL_TRIANGLE_STRIP, 4 * sprite_id, 4);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
    }
};

#include "debug_draw/debug_draw.hpp"
void GameTest::draw(float dt) {
    LocalPlayer* local_player = dynamic_cast<LocalPlayer*>(playerGetPrimary());
    assert(local_player);
    Viewport* viewport = local_player->getViewport();
    assert(viewport);
    gpuRenderBucket* render_bucket = viewport->getRenderBucket();
    gpuRenderTarget* render_target = viewport->getRenderTarget();

    assert(render_bucket);
    assert(render_target);

    static float current_time = .0f;
    current_time += dt;
    static float angle = .0f;
    gfxm::quat q = gfxm::angle_axis(angle, gfxm::vec3(0, 1, 0));
    gfxm::mat4 model = gfxm::to_mat4(q);
    angle += 0.01f;

    {
        gfxm::vec4          positions_new[TEST_INSTANCE_COUNT];
        static float        random_distr[TEST_INSTANCE_COUNT];
        auto fill_array_rand = [](float* arr, size_t count)->int {
            for (int i = 0; i < count; ++i) { arr[i] = (rand() % 100) * 0.01f; }
            return 0;
        };
        static int once = fill_array_rand(random_distr, TEST_INSTANCE_COUNT);

        for (int i = 0; i < TEST_INSTANCE_COUNT; ++i) {
            gfxm::vec3 radial_pos 
                = gfxm::vec3(sinf(angle * random_distr[i] + random_distr[i] * gfxm::pi * 2), .0f, cosf(angle * random_distr[i] + random_distr[i] * gfxm::pi * 2))
                * (30.0f + i * 0.5f);
            positions_new[i] = gfxm::vec4(radial_pos.x, sinf(angle + i * 0.1f) * 5.0f + 2.5f, radial_pos.z, 0);
        }
        inst_pos_buffer.setArrayData(positions_new, sizeof(positions_new));
    }
    //render_bucket->add(renderable_plane.get());
    render_bucket->add(renderable.get());
    render_bucket->add(renderable2.get());
    
    gfxm::mat4 matrix
        = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(-3, 1, 0))
        * gfxm::to_mat4(gfxm::angle_axis(angle, gfxm::vec3(0, 1, 0)));
    renderable2->setTransform(matrix);
    renderable_plane->setTransform(gfxm::mat4(1.f));

    static gfxm::mat4 projection = gfxm::perspective(gfxm::radian(65), 16.0f / 9.0f, 0.01f, 1000.0f);
    static gfxm::mat4 view(1.0f);
    projection = viewport->getProjection();
    view = viewport->getViewTransform();

    ubufTime->setFloat(
        gpuGetPipeline()->getUniformBufferDesc(UNIFORM_BUFFER_TIME)->getUniform("fTime"),
        current_time
    );

    static SpriteAtlas sprite_atlas;
    // INIT SPRITE ATLAS
    {
        auto init = [](SpriteAtlas& a)->int {
            a.init();
            return 0;
        };
        static int once = init(sprite_atlas);
    }
    // PARTICLES TEST
    {
        auto init = [](ParticleEmitter& e)->int {
            e.init(&sprite_atlas);
            return 0;
        };
        static ParticleEmitter emitter;
        static int once = init(emitter);

        emitter.update(dt);
        emitter.draw(render_bucket);
    }


    // Lights test 
    {
        static float t = .0f;
        t += .01f;
        gfxm::vec3 a = gfxm::vec3(cosf(t) * 15.f, 2.f, sinf(t) * 15.f);
        gfxm::vec3 b = gfxm::vec3(cosf(t + gfxm::pi) * 15.f, 2.f, sinf(t + gfxm::pi) * 15.f);
        //gfxm::vec3 a = gfxm::vec3(3.0f, 3.0f, -12 + std::fmodf(t, 12.f) * 4.f);
        //gfxm::vec3 b = gfxm::vec3(-3.0f, 3.0f, -12 + std::fmodf(t + 6.f, 12.f) * 4.f);
        
        render_bucket->addLightOmni(
            gfxm::vec3(-9, 0, 6) + a,
            gfxm::vec3(1.0, 0.4, .2), //gfxm::vec3(.2, 1., 0.4),
            80.f * 1.f
        );
        render_bucket->addLightOmni(
            gfxm::vec3(-9, 0, 6) + b,
            gfxm::vec3(.4, 0.2, 1.),
            80.f * 1.f
        );/*
        render_bucket->addLightOmni(
            gfxm::vec3(-7, 2, 3),
            gfxm::vec3(.4, 0.2, 1.),
            80.f * 20.f
        );*/
        /*
        render_bucket->addLightDirect(
            gfxm::normalize(gfxm::vec3(-1, -1, 1)),
            gfxm::vec3(1, 1, 1),
            10.f
        );*/
        
        /*
        render_bucket->addLightOmni(
            tps_camera_actor.getRoot()->getTranslation(),
            gfxm::vec3(1.f, 1.f, 1.f),
            40.f
        );*/
    }

    
    GameBase::draw(dt);
    
    // TODO: Move all this to a custom render pass when they are implemented to support custom passes
    /*
    GLuint gvao;
    glGenVertexArrays(1, &gvao);
    glBindVertexArray(gvao);
    {
        glEnable(GL_DEPTH_TEST);
        render_target->bindFrameBuffer("VFX", 0);
        
        // TRAIL TEST
        {
            auto init = [](PolygonTrail& t)->int {
                t.init();
                return 0;
            };
            static PolygonTrail trail;
            static int once = init(trail);

            trail.origin = chara->getTranslation();
            trail.update(dt);
            trail.draw(view, projection);
        }
        // SPRITE TEST
        {
            auto init = [](SpriteBillboard& b)->int {
                b.init();
                return 0;
            };
            static SpriteBillboard bb;
            static int once = init(bb);

            bb.origin = gfxm::vec3(-5.0f, 1.f, 3.5f);
            bb.update(dt);
            bb.draw(view, projection);
        }
        // ANIMATED SPRITE TEST
        {
            auto init = [](SpriteBillboardAnimated& s)->int {
                s.init(&sprite_atlas);
                return 0;
            };
            static SpriteBillboardAnimated sprite;
            static int once = init(sprite);

            sprite.origin = gfxm::vec3(7.f, 1.f, .0f);
            sprite.update(dt);
            sprite.draw(view, projection);
        }

    }
    gpuFrameBufferUnbind();

    render_target->bindFrameBuffer("PostDbg", 0);
    gameuiDraw();

    glDeleteVertexArrays(1, &gvao);*/
}