
#include <stack>

#include "platform/gl/glextutil.h"

#include "platform/platform.hpp"

#include "game_common.hpp"
#include "common/mesh3d/generate_primitive.hpp"

#include <algorithm>
#include <unordered_map>

#include "common/render/gpu_pipeline.hpp"

#include "common/math/bezier.hpp"

#include "common/render/render_bucket.hpp"


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


void gpuDrawTextureToDefaultFrameBuffer(gpuTexture2d* texture) {
    int screen_w = 0, screen_h = 0;
    platformGetWindowSize(screen_w, screen_h);

    const char* vs = R"(
        #version 450 
        in vec3 inPosition;
        out vec2 uv_frag;
        
        void main(){
            uv_frag = vec2((inPosition.x + 1.0) / 2.0, (inPosition.y + 1.0) / 2.0);
            vec4 pos = vec4(inPosition, 1);
            gl_Position = pos;
        })";
    const char* fs = R"(
        #version 450
        in vec2 uv_frag;
        out vec4 outAlbedo;
        uniform sampler2D texAlbedo;
        void main(){
            vec4 pix = texture(texAlbedo, uv_frag);
            float a = pix.a;
            outAlbedo = vec4(pix.rgb, 1);
        })";
    gpuShaderProgram prog(vs, fs);
    float vertices[] = {
        -1.0f, -1.0f, 0.0f,     3.0f, -1.0f, 0.0f,      -1.0f, 3.0f, 0.0f
    };

    GLuint gvao;
    glGenVertexArrays(1, &gvao);
    glBindVertexArray(gvao);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        3, GL_FLOAT, GL_FALSE,
        0, (void*)0 /* offset */
    );

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0, 0, screen_w, screen_h);
    glScissor(0, 0, screen_w, screen_h);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture->getId());
    glUseProgram(prog.getId());
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glUseProgram(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &gvao);
}


class SpriteAtlas {
public:
    struct spr {
        gfxm::vec2 tex_min;
        gfxm::vec2 tex_max;
        gfxm::vec2 origin;
    };

    gpuTexture2d texture;
    gpuBuffer vertexBuffer;
    gpuBuffer uvBuffer;

private:
    std::vector<spr> spr_sheet;
    gfxm::vec2 atlas_size;
    int sprite_count;

public:
    const gpuTexture2d* getTexture() const {
        return &texture;
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
        ktImage* img = loadImage("light_003.png");
        texture.setData(img);
        delete img;

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

    void init(SpriteAtlas* atlas) {
        this->atlas = atlas;

        ktImage* img = loadImage("particle.png");
        texture.setData(img);
        delete img;

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

        posBuffer.setArrayData(particlePositions.data(), particlePositions.size() * sizeof(particlePositions[0]));
        particleDataBuffer.setArrayData(particleData.data(), particleData.size() * sizeof(particleData[0]));
        particleColorBuffer.setArrayData(particleColors.data(), particleColors.size() * sizeof(particleColors[0]));
        particleSpriteDataBuffer.setArrayData(particleSpriteData.data(), particleSpriteData.size() * sizeof(particleSpriteData[0]));
        particleSpriteUVBuffer.setArrayData(particleSpriteUV.data(), particleSpriteUV.size() * sizeof(particleSpriteUV[0]));
    }

    void draw(const gfxm::mat4& view, const gfxm::mat4& proj) {
        float vertices[] = {
                0, 0, 0, 1.f, 0, 0,
                0, 1.f, 0, 1.f, 1.f, 0
        };
        float uvs[] = {
                .0f, .0f, 1.f, .0f,
                .0f, 1.f, 1.f, 1.f
        };
        gpuBuffer vertexBuffer;
        vertexBuffer.setArrayData(vertices, sizeof(vertices));
        gpuBuffer uvBuffer;
        uvBuffer.setArrayData(uvs, sizeof(uvs));

        const char* vs = R"(
                #version 450 
                layout (location = 0) in vec3 inPosition;
                layout (location = 1) in vec2 UV;
                layout (location = 4) in vec4 inParticlePosition;
                layout (location = 5) in vec4 inParticleData;
                layout (location = 6) in vec4 inParticleColorRGBA;
                layout (location = 7) in vec4 inParticleSpriteData;
                layout (location = 8) in vec4 inParticleSpriteUV;
                uniform mat4 matView;
                uniform mat4 matProjection;
                uniform mat4 matModel;
                out vec4 fragColor;
                out vec2 fragUV;
                void main() {
                    vec2 uv_ = vec2(
                        inParticleSpriteUV.x + (inParticleSpriteUV.z - inParticleSpriteUV.x) * UV.x,
                        inParticleSpriteUV.y + (inParticleSpriteUV.w - inParticleSpriteUV.y) * UV.y
                    );
                    fragUV = uv_;
                    vec4 positionViewSpace = matView * vec4(inParticlePosition.xyz, 1.0);
                    vec2 vertex = (inPosition.xy * inParticleSpriteData.xy - inParticleSpriteData.zw) * inParticlePosition.w;
                    positionViewSpace.xy += vertex;
                    fragColor = inParticleColorRGBA;
                    gl_Position = matProjection * positionViewSpace;
                })";
        const char* fs = R"(
                #version 450
                uniform sampler2D tex;
                in vec2 fragUV;
                in vec4 fragColor;
                out vec4 outAlbedo;
                void main(){
                    vec4 s = texture(tex, fragUV.xy);
                    outAlbedo = s * fragColor;
                })";
        gpuShaderProgram prog(vs, fs);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(4);
        glEnableVertexAttribArray(5);
        glEnableVertexAttribArray(6);
        glEnableVertexAttribArray(7);
        glEnableVertexAttribArray(8);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.getId());
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glBindBuffer(GL_ARRAY_BUFFER, uvBuffer.getId());
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

        glBindBuffer(GL_ARRAY_BUFFER, posBuffer.getId());
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 0, 0);
        glVertexAttribDivisor(4, 1);

        glBindBuffer(GL_ARRAY_BUFFER, particleDataBuffer.getId());
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 0, 0);
        glVertexAttribDivisor(5, 1);

        glBindBuffer(GL_ARRAY_BUFFER, particleColorBuffer.getId());
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 0, 0);
        glVertexAttribDivisor(6, 1);

        glBindBuffer(GL_ARRAY_BUFFER, particleSpriteDataBuffer.getId());
        glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, 0, 0);
        glVertexAttribDivisor(7, 1);

        glBindBuffer(GL_ARRAY_BUFFER, particleSpriteUVBuffer.getId());
        glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, 0, 0);
        glVertexAttribDivisor(8, 1);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);

        glUseProgram(prog.getId());
        glUniformMatrix4fv(prog.getUniformLocation("matView"), 1, GL_FALSE, (float*)&view);
        glUniformMatrix4fv(prog.getUniformLocation("matProjection"), 1, GL_FALSE, (float*)&proj);
        glUniformMatrix4fv(prog.getUniformLocation("matModel"), 1, GL_FALSE, (float*)&gfxm::mat4(1.0f));
        glUniform1i(prog.getUniformLocation("tex"), 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, atlas->texture.getId());
        
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, alive_count);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(4);
        glDisableVertexAttribArray(5);
        glDisableVertexAttribArray(6);
        glDisableVertexAttribArray(7);
        glDisableVertexAttribArray(8);
    }
};

class PolygonTrail {
public:
    static const int pointCount = 100;
    gfxm::vec3 points[pointCount];
    gfxm::vec3 origin;
    gfxm::vec3 position;
    float thickness = .1f;
    gpuTexture2d texture;
    float distanceTraveled = .0f;
    float hue_ = .0f;

    void init() {
        ktImage* img = loadImage("trail.jpg");
        texture.setData(img);
        delete img;

        points[0] = position;
        for (int i = 1; i < pointCount; ++i) {
            points[i] = points[0];
        }
    }
    void update(float dt) {
        static float time = .0f;
        time += dt * 3.0f;
        gfxm::vec3 prevPos = position;
        position = gfxm::vec3(sinf(time) * 1.0f, sinf(time * .15f) * 0.8f + 1.0f, cosf(time) * 1.0f);
        position += origin;
        distanceTraveled += gfxm::length(position - prevPos);
        
        for (int i = 1; i < pointCount; ++i) {
            float len = gfxm::sqrt(gfxm::_max(.0f, gfxm::_min(1.f, gfxm::length(points[i - 1] - points[i]))));
            points[i] = gfxm::lerp(points[i], points[i - 1], len/*0.15f*/);
        }
        points[0] = position;

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
        for (int i = 0; i < pointCount - 1; ++i) {
            gfxm::vec3 trailN = gfxm::normalize(points[i + 1] - points[i]);
            vertices[i * 2] = points[i] + gfxm::cross(gfxm::vec3(cam[2]), trailN) * thickness * 0.5f;
            vertices[i * 2 + 1] = points[i] - gfxm::cross(gfxm::vec3(cam[2]), trailN) * thickness * 0.5f;
        }
        gfxm::vec3 trailN = gfxm::normalize(points[pointCount - 1] - points[pointCount - 2]);
        int lastId = pointCount - 1;
        vertices[lastId * 2] = points[lastId] + gfxm::normalize(gfxm::cross(gfxm::vec3(cam[2]), trailN)) * 0.5f;
        vertices[lastId * 2 + 1] = points[lastId] - gfxm::normalize(gfxm::cross(gfxm::vec3(cam[2]), trailN)) * 0.5f;

        for (int i = 0; i < pointCount; ++i) {
            float u = (float)i / (float)(pointCount - 1) * 10.0f - distanceTraveled / 10.0f;
            uvs[i * 2] = gfxm::vec2(u, .0f);
            uvs[i * 2 + 1] = gfxm::vec2(u, 1.f);
        }

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
            color[ia + 3] = a;

            color[ib] = col.x * 255;
            color[ib + 1] = col.y * 255;
            color[ib + 2] = col.z * 255;
            color[ib + 3] = a;
        }

        gpuBuffer vertexBuffer;
        vertexBuffer.setArrayData(vertices.data(), vertices.size() * sizeof(vertices[0]));
        gpuBuffer uvBuffer;
        uvBuffer.setArrayData(uvs.data(), uvs.size() * sizeof(uvs[0]));
        gpuBuffer colorBuffer;
        colorBuffer.setArrayData(color.data(), color.size() * sizeof(color[0]));

        const char* vs = R"(
            #version 450 
            layout (location = 0) in vec3 vertexPosition;
            layout (location = 1) in vec2 UV;
            layout (location = 2) in vec4 colorRGBA;
            uniform mat4 matView;
            uniform mat4 matProjection;
            uniform mat4 matModel;
            out vec4 fragColor;
            out vec2 fragUV;
            void main() {
                fragUV = UV;
                fragColor = colorRGBA;
                
                gl_Position = matProjection * matView * matModel * vec4(vertexPosition, 1.0);
            })";
        const char* fs = R"(
            #version 450
            uniform sampler2D tex;
            in vec4 fragColor;
            in vec2 fragUV;
            out vec4 outAlbedo;
            void main(){
                vec4 s = texture(tex, fragUV.xy);
                outAlbedo = vec4(fragColor.xyz, fragColor.a * s.x);
            })";
        gpuShaderProgram prog(vs, fs);

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

        glUseProgram(prog.getId());
        glUniformMatrix4fv(prog.getUniformLocation("matView"), 1, GL_FALSE, (float*)&view);
        glUniformMatrix4fv(prog.getUniformLocation("matProjection"), 1, GL_FALSE, (float*)&proj);
        glUniformMatrix4fv(prog.getUniformLocation("matModel"), 1, GL_FALSE, (float*)&gfxm::mat4(1.0f));
        glUniform1i(prog.getUniformLocation("tex"), 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture.getId());
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
    gpuTexture2d texture;

public:
    gfxm::vec3 origin;

    void init() {
        ktImage* img = loadImage("icon_sprite_test.png");
        texture.setData(img);
        delete img;
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

        const char* vs = R"(
            #version 450 
            layout (location = 0) in vec3 vertexPosition;
            layout (location = 1) in vec2 UV;
            layout (location = 2) in vec4 colorRGBA;
            uniform mat4 matView;
            uniform mat4 matProjection;
            uniform mat4 matModel;
            out vec4 fragColor;
            out vec2 fragUV;
            void main() {
                fragUV = UV;
                fragColor = colorRGBA;

                mat4 billboardViewModel = matView * matModel;
                billboardViewModel[0] = vec4(1,0,0,0);
                billboardViewModel[1] = vec4(0,1,0,0);
                billboardViewModel[2] = vec4(0,0,1,0);  
                
                gl_Position = matProjection * billboardViewModel * vec4(vertexPosition, 1.0);
            })";
        const char* fs = R"(
            #version 450
            uniform sampler2D tex;
            in vec4 fragColor;
            in vec2 fragUV;
            out vec4 outAlbedo;
            void main(){
                vec4 s = texture(tex, fragUV.xy);
                outAlbedo = fragColor * s;
            })";
        gpuShaderProgram prog(vs, fs);
        
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

        glUseProgram(prog.getId());
        glUniformMatrix4fv(prog.getUniformLocation("matView"), 1, GL_FALSE, (float*)&view);
        glUniformMatrix4fv(prog.getUniformLocation("matProjection"), 1, GL_FALSE, (float*)&proj);
        glUniformMatrix4fv(prog.getUniformLocation("matModel"), 1, GL_FALSE, (float*)&model);
        glUniform1i(prog.getUniformLocation("tex"), 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture.getId());
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
    SpriteAtlas* atlas = 0;
    int sprite_id = 0;

public:
    gfxm::vec3 origin;

    void init(SpriteAtlas* atlas) {
        this->atlas = atlas;
    }
    void update(float dt) {
        static float time = .0f;
        
        sprite_id = (int)(time / (1.0f / 15.0f)) % atlas->getSpriteCount();

        // TODO
        
        time += dt;
    }
    void draw(const gfxm::mat4& view, const gfxm::mat4& proj) {
        const char* vs = R"(
            #version 450 
            layout (location = 0) in vec3 vertexPosition;
            layout (location = 1) in vec2 UV;
            uniform mat4 matView;
            uniform mat4 matProjection;
            uniform mat4 matModel;
            out vec2 fragUV;
            void main() {
                fragUV = UV;

                mat4 billboardViewModel = matView * matModel;
                billboardViewModel[0] = vec4(1,0,0,0);
                billboardViewModel[1] = vec4(0,1,0,0);
                billboardViewModel[2] = vec4(0,0,1,0);  
                
                gl_Position = matProjection * billboardViewModel * vec4(vertexPosition, 1.0);
            })";
        const char* fs = R"(
            #version 450
            uniform sampler2D tex;
            in vec2 fragUV;
            out vec4 outAlbedo;
            void main(){
                vec4 s = texture(tex, fragUV.xy);
                outAlbedo = s;
            })";
        gpuShaderProgram prog(vs, fs);

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

        glUseProgram(prog.getId());
        glUniformMatrix4fv(prog.getUniformLocation("matView"), 1, GL_FALSE, (float*)&view);
        glUniformMatrix4fv(prog.getUniformLocation("matProjection"), 1, GL_FALSE, (float*)&proj);
        glUniformMatrix4fv(prog.getUniformLocation("matModel"), 1, GL_FALSE, (float*)&model);
        glUniform1i(prog.getUniformLocation("tex"), 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, atlas->texture.getId());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glDrawArrays(GL_TRIANGLE_STRIP, 4 * sprite_id, 4);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
    }
};

class DecalScreenSpace {
    gpuTexture2d texture;
    gpuTexture2d* texture_depth = 0;
    gfxm::vec3 origin;
    gfxm::quat rotation;

    gfxm::vec3 boxSize;

    gpuBuffer vertexBuffer;
    gpuMeshDesc meshDesc;
    std::unique_ptr<gpuShaderProgram> prog;
    gpuRenderMaterial* material = 0;
    gpuUniformBuffer* ubufModel;
    gpuUniformBuffer* ubufDecal;
    gpuRenderable renderable;
public:
    void init(gpuTexture2d* texture_depth) {
        boxSize = gfxm::vec3(7.0f, 2.0f, 7.0f);

        ktImage* img = loadImage("fire_circle.png");
        texture.setData(img);
        delete img;

        this->texture_depth = texture_depth;

        //
        float width = boxSize.x;
        float height = boxSize.y;
        float depth = boxSize.z;
        float w = width * .5f;
        float h = height * .5f;
        float d = depth * .5f;

        int screen_w = 0, screen_h = 0;
        platformGetWindowSize(screen_w, screen_h);

        float vertices[] = {
            -w, -h,  d,   w, -h,  d,    w,  h,  d,
             w,  h,  d,  -w,  h,  d,   -w, -h,  d,

             w, -h,  d,   w, -h, -d,    w,  h, -d,
             w,  h, -d,   w,  h,  d,    w, -h,  d,

             w, -h, -d,  -w, -h, -d,   -w,  h, -d,
            -w,  h, -d,   w,  h, -d,    w, -h, -d,

            -w, -h, -d,  -w, -h,  d,   -w,  h,  d,
            -w,  h,  d,  -w,  h, -d,   -w, -h, -d,

            -w,  h,  d,   w,  h,  d,    w,  h, -d,
             w,  h, -d,  -w,  h, -d,   -w,  h,  d,

            -w, -h, -d,   w, -h, -d,    w, -h,  d,
             w, -h,  d,  -w, -h,  d,   -w, -h, -d
        };

        vertexBuffer.setArrayData(vertices, sizeof(vertices));

        meshDesc.setAttribArray(VFMT::Position_GUID, &vertexBuffer);
        meshDesc.setDrawMode(MESH_DRAW_TRIANGLES);
        meshDesc.setVertexCount(36);

        const char* vs = R"(
            #version 450 
            layout (location = 0) in vec3 inPosition;
            layout (location = 1) in vec2 inUV;
            layout (location = 2) in vec4 inColorRGBA;
            layout(std140) uniform bufCamera3d {
                mat4 matProjection;
                mat4 matView;
            };
            layout(std140) uniform bufModel {
                mat4 matModel;
            };
            out vec4 fragColor;
            out vec2 fragUV;
            out mat4 fragProjection;
            out mat4 fragView;
            out mat4 fragModel;
            void main() {
                fragUV = inUV;
                fragColor = inColorRGBA; 
                fragProjection = matProjection;
                fragView = matView;
                fragModel = matModel;         

                gl_Position = matProjection * matView * matModel * vec4(inPosition, 1.0);
            })";
        const char* fs = R"(
            #version 450
            layout(std140) uniform bufDecal {
                uniform vec3 boxSize;
                uniform vec2 screenSize;
            };            
            uniform sampler2D tex;
            uniform sampler2D tex_depth;
            in vec4 fragColor;
            in vec2 fragUV;
            in mat4 fragProjection;
            in mat4 fragView;
            in mat4 fragModel;
            out vec4 outAlbedo;

            float contains(vec3 pos, vec3 bottom_left, vec3 top_right) {
                vec3 s = step(bottom_left, pos) - step(top_right, pos);
                return s.x * s.y * s.z;
            }

            vec3 worldPosFromDepth(float depth, vec2 uv, mat4 proj, mat4 view) {
                float z = depth * 2.0 - 1.0;

                vec4 clipSpacePosition = vec4(uv * 2.0 - 1.0, z, 1.0);
                vec4 viewSpacePosition = inverse(proj) * clipSpacePosition;

                viewSpacePosition /= viewSpacePosition.w;
                
                vec4 worldSpacePosition = inverse(view) * viewSpacePosition;

                return worldSpacePosition.xyz;
            }

            void main(){
                vec4 frag_coord = gl_FragCoord;
                float frag_u = frag_coord.x / screenSize.x;
                float frag_v = frag_coord.y / screenSize.y;
                vec4 depth_sample = texture(tex_depth, vec2(frag_u, frag_v));
                
                vec3 world_pos = worldPosFromDepth(depth_sample.x, vec2(frag_u, frag_v), fragProjection, fragView);
                vec4 decal_pos = inverse(fragModel) * vec4(world_pos, 1);
                if(contains(decal_pos.xyz, -boxSize * .5, boxSize * .5) < 1.0) {
                    discard;
                }
                vec2 decal_uv = vec2(decal_pos.x / boxSize.x + .5, decal_pos.z / boxSize.z + .5);
                vec4 decal_sample = texture(tex, decal_uv);
                float alpha = 1.0 - abs(decal_pos.y / boxSize.y * 2.0);
                outAlbedo = vec4(decal_sample.xyz, decal_sample.a * alpha);
            })";
        prog.reset(new gpuShaderProgram(vs, fs));

        material = gpuGetPipeline()->createMaterial();
        auto tech = material->addTechnique("Decals");
        auto pass = tech->addPass();
        pass->setShader(prog.get());
        pass->depth_write = 0;
        pass->blend_mode = GPU_BLEND_MODE::ADD;
        material->addSampler("tex", &texture);
        material->addSampler("tex_depth", texture_depth);
        material->compile();

        ubufModel = gpuGetPipeline()->createUniformBuffer(UNIFORM_BUFFER_MODEL);
        ubufDecal = gpuGetPipeline()->createUniformBuffer(UNIFORM_BUFFER_DECAL);

        renderable.attachUniformBuffer(ubufModel);
        renderable.attachUniformBuffer(ubufDecal);
        renderable.setMaterial(material);
        renderable.setMeshDesc(&meshDesc);
        renderable.compile();
    }
    void update(float dt) {
        static float time = .0f;
        time += dt;

        origin = gfxm::vec3(4, 0, 3);
        //origin += gfxm::vec3(sinf(time * .5f), 0, cosf(time * .5f));

        rotation = gfxm::angle_axis(time, gfxm::vec3(0, 1, 0));

        gfxm::mat4 model
            = gfxm::translate(gfxm::mat4(1.0f), origin) * gfxm::to_mat4(rotation);
        ubufModel->setMat4(ubufModel->getDesc()->getUniform("matModel"), model);
        ubufDecal->setVec3(ubufDecal->getDesc()->getUniform("boxSize"), boxSize);

        int screen_w = 0, screen_h = 0;
        platformGetWindowSize(screen_w, screen_h);
        gfxm::vec2 screenSize(screen_w, screen_h);
        ubufDecal->setVec2(ubufDecal->getDesc()->getUniform("screenSize"), screenSize);
    }
    void draw() {
        gpuDrawRenderable(&renderable);
    }
};



void GameCommon::Draw(float dt) {
    gpuClearQueue();

    static float current_time = .0f;
    current_time += dt;
    static float angle = .0f;
    gfxm::quat q = gfxm::angle_axis(angle, gfxm::vec3(0, 1, 0));
    gfxm::mat4 model = gfxm::to_mat4(q);
    angle += 0.01f;

    collision_debug_draw->draw();

    gpuDrawRenderable(renderable_plane.get());
    gpuDrawRenderable(&scene_mesh->renderable);
    gpuDrawRenderable(renderable.get());
    gpuDrawRenderable(renderable2.get());
    gpuDrawRenderable(renderable_text.get());

    gfxm::vec3 loco_vec = inputCharaTranslation->getVec3();
    gfxm::mat3 loco_rot;
    loco_rot[2] = gfxm::normalize(cam.getView() * gfxm::vec4(0, 0, 1, 0));
    loco_rot[1] = gfxm::vec3(0, 1, 0);
    loco_rot[0] = gfxm::cross(loco_rot[1], loco_rot[2]);
    loco_vec = loco_rot * loco_vec;
    loco_vec.y = .0f;
    loco_vec = gfxm::normalize(loco_vec);

    chara.setDesiredLocomotionVector(loco_vec);
    if (inputCharaUse->isJustPressed()) {
        chara.actionUse();
    }
    chara.update(dt);
    chara.draw();

    cam.setTarget(chara.getWorldTransform() * gfxm::vec4(0, 1.6f, 0, 1));
    cam.update(dt);

    // Collision
    collider_d.position += inputBoxTranslation->getVec3() * dt;
    collider_d.rotation = gfxm::euler_to_quat(inputBoxRotation->getVec3() * dt) * collider_d.rotation;

    collision_world.update();
    collision_world.debugDraw();
    collision_debug_draw->flushDrawData();

    // TODO: Can't add many of the same renderable
    // cause renderable holds the uniform buffers
    // POSSIBLE SOLUTION: separate into gpuRenderable and gpuRenderableInstance
    /*
    for (int i = 0; i < 10; ++i) {
        for (int j = 0; j < 10; ++j) {
            for (int k = 0; k < 10; ++k) {
                gfxm::vec3 translation(i * 3, j * 3, -k * 3);
                bucket.add(&renderable, gfxm::translate(gfxm::mat4(1.0f), translation));
            }
        }
    }*/

    model_3d->draw();

    gfxm::vec3 shpere_pos = gfxm::vec3(sinf(angle) * 1.5f, 1, cosf(angle) * 1.5f);
    scene_mesh->transform = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(0, 1, -2));
    scene_mesh->update();
    renderable2_ubuf->setMat4(renderable2_ubuf->getDesc()->getUniform("matModel"), gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(-3, 1, 0)));
    renderable_plane_ubuf->setMat4(renderable_plane_ubuf->getDesc()->getUniform("matModel"), gfxm::mat4(1.0f));
    renderable_text_ubuf->setMat4(renderable_text_ubuf->getDesc()->getUniform("matModel"), gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(0, 1.5f, 0)) * gfxm::scale(gfxm::mat4(1.0f), gfxm::vec3(0.01f, 0.01f, 0.01f)));

    gpuFrameBufferBind(frame_buffer.get());
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gpuFrameBufferUnbind();

    ubufCam3d->setMat4(
        ubufCam3dDesc->getUniform("matProjection"),
        cam.getProjection()
    );
    ubufCam3d->setMat4(
        ubufCam3dDesc->getUniform("matView"),
        cam.getInverseView()
    );
    ubufTime->setFloat(
        ubufTimeDesc->getUniform("fTime"),
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
    // SCREEN SPACE DECAL TEST
    {
        auto init = [this](DecalScreenSpace& d)->int {
            d.init(tex_depth.get());
            return 0;
        };
        static DecalScreenSpace decal;
        static int once = init(decal);

        decal.update(dt);
        decal.draw();
    }
    
    gpuDraw();
    
    GLuint gvao;
    glGenVertexArrays(1, &gvao);
    glBindVertexArray(gvao);
    {
        gpuFrameBufferBind(fb_color.get());

        //
        
        gpuFrameBufferBind(frame_buffer.get());
        // PARTICLES TEST
        {
            auto init = [](ParticleEmitter& e)->int {
                e.init(&sprite_atlas);
                return 0;
            };
            static ParticleEmitter emitter;
            static int once = init(emitter);

            emitter.update(dt);
            emitter.draw(cam.getInverseView(), cam.getProjection());
        }
        // TRAIL TEST
        {
            auto init = [](PolygonTrail& t)->int {
                t.init();
                return 0;
            };
            static PolygonTrail trail;
            static int once = init(trail);

            trail.origin = chara.getTranslation();
            trail.update(dt);
            trail.draw(cam.getInverseView(), cam.getProjection());
        }
        // SPRITE TEST
        {
            auto init = [](SpriteBillboard& b)->int {
                b.init();
                return 0;
            };
            static SpriteBillboard bb;
            static int once = init(bb);

            bb.origin = gfxm::vec3(.0f, 1.f, 3.5f);
            bb.update(dt);
            bb.draw(cam.getInverseView(), cam.getProjection());
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
            sprite.draw(cam.getInverseView(), cam.getProjection());
        }

        // GUI TEST?
        {
            guiLayout();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glEnable(GL_SCISSOR_TEST);
            glDisable(GL_DEPTH_TEST);

            int screen_w = 0, screen_h = 0;
            platformGetWindowSize(screen_w, screen_h);
            glViewport(0, 0, screen_w, screen_h);
            glScissor(0, 0, screen_w, screen_h);
            guiDraw();
        }
    }
    gpuFrameBufferUnbind();

    //gpuDrawTextureToDefaultFrameBuffer(tex_albedo.get());

    glDeleteVertexArrays(1, &gvao);
}