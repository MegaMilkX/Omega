#include "game_ui.hpp"

#include <stdint.h>
#include "platform/gl/glextutil.h"

#include "gpu/gpu.hpp"
#include "gpu/gpu_shader_program.hpp"
#include "gpu/gpu_buffer.hpp"
#include "gpu/gpu_text.hpp"
#include "gpu/gpu_texture_2d.hpp"
#include "gpu/gpu_renderable.hpp"
#include "typeface/font.hpp"

static gpuShaderProgram* prog = 0;

void gameuiInit() {
    const char* vs_src = R"(
            #version 450 
            layout (location = 0) in vec3 inPosition;
            layout (location = 1) in vec2 inUV;
            layout (location = 2) in float inTextUVLookup;
            layout (location = 3) in vec3 inColorRGB;
            out vec2 uv_frag;
            out vec4 col_frag;

            uniform sampler2D texTextUVLookupTable;

            uniform mat4 matProjection;
            uniform mat4 matView;
            uniform mat4 matModel;
        
            void main(){
                vec2 uv_ = texelFetch(texTextUVLookupTable, ivec2(inTextUVLookup, 0), 0).xy;
                uv_frag = uv_;
                col_frag = vec4(inColorRGB, 1.0);        

                vec3 pos3 = inPosition;
	            pos3.x = round(pos3.x);
	            pos3.y = round(pos3.y);

                vec3 scale = vec3(length(matModel[0]),
                        length(matModel[1]),
                        length(matModel[2])); 
                
                mat4 scaleHack;
                scaleHack[0] = vec4(1, 0, 0, 0);
                scaleHack[1] = vec4(0, 1, 0, 0);
                scaleHack[2] = vec4(0, 0, 1, 0);
                scaleHack[3] = vec4(0, 0, 0, 1);
	            vec4 pos = matProjection * matView * matModel * scaleHack * vec4(inPosition, 1);
                gl_Position = pos;
            })";
    const char* fs_src = R"(
            #version 450
            in vec2 uv_frag;
            in vec4 col_frag;
            out vec4 outAlbedo;

            uniform sampler2D texAlbedo;
            uniform vec4 color;
            void main(){
                float c = texture(texAlbedo, uv_frag).x;
                outAlbedo = vec4(1, 1, 1, c) * col_frag * color;
            })";
    prog = new gpuShaderProgram(vs_src, fs_src);
}
void gameuiCleanup() {
    delete prog;
}

struct SamplerSet {
    std::map<std::string, const gpuTexture2d*> textures;

    const gpuTexture2d*& operator[](const std::string& name) {
        return textures[name];
    }
};
struct SamplerSlotPair {
    const gpuTexture2d* texture;
    int slot;
};
struct SamplerBindings {
    int active_slots;
    std::vector<SamplerSlotPair> textures;
};
void gpuMakeSamplerBindings(const gpuShaderProgram* prog, const SamplerSet* samplers, SamplerBindings* bindings) {
    bindings->active_slots = prog->getSamplerCount();

    for (int i = 0; i < prog->getSamplerCount(); ++i) {
        auto& name = prog->getSamplerName(i);
        auto& it = samplers->textures.find(name);
        if (it == samplers->textures.end()) {
            continue;
        }
        bindings->textures.push_back(SamplerSlotPair{ it->second, i });
    }
}

void gpuBindSamplers(const SamplerBindings* bindings) {
    for (int i = 0; i < bindings->textures.size(); ++i) {
        glActiveTexture(GL_TEXTURE0 + bindings->textures[i].slot);
        glBindTexture(GL_TEXTURE_2D, bindings->textures[i].texture->getId());
    }
}

void gameuiDraw() {
    Font* font_under = fontGet("fonts/Platinum Sign Under.ttf", 32);
    Font* font_over = fontGet("fonts/Platinum Sign Over.ttf", 32);
    gpuText gpu_text_under(font_under);
    gpuText gpu_text_over(font_over);

    gpu_text_under.setString("HEALTH 100");
    gpu_text_under.commit(0, 1);
    gpu_text_over.setString("HEALTH 100");
    gpu_text_over.commit(0, 1);
    auto text_mesh_under = gpu_text_under.getMeshDesc();
    auto text_mesh_over = gpu_text_over.getMeshDesc();

    ktImage atlas;
    ktImage lookup;
    gpuTexture2d tex_atlas_under;
    gpuTexture2d tex_lookup_under;
    gpuTexture2d tex_atlas_over;
    gpuTexture2d tex_lookup_over;
    font_under->buildAtlas(&atlas, &lookup);
    tex_atlas_under.setData(&atlas);
    tex_lookup_under.setData(&lookup);
    font_over->buildAtlas(&atlas, &lookup);
    tex_atlas_over.setData(&atlas);
    tex_lookup_over.setData(&lookup);

    SamplerSet samplers_under;
    samplers_under["texAlbedo"] = &tex_atlas_under;
    samplers_under["texTextUVLookupTable"] = &tex_lookup_under;
    SamplerSet samplers_over;
    samplers_over["texAlbedo"] = &tex_atlas_over;
    samplers_over["texTextUVLookupTable"] = &tex_lookup_over;

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    //glBindFramebuffer(GL_FRAMEBUFFER, 0);

    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    
    glUseProgram(prog->getId());
    gfxm::mat4 model = gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(300, 200, 0));
    gfxm::mat4 view(1.0f);
    gfxm::mat4 proj = gfxm::ortho(0.f, 1920.f, .0f, 1080.f, -10.f, 10.f);
    prog->setUniformMatrix4("matModel", model);
    prog->setUniformMatrix4("matView", view);
    prog->setUniformMatrix4("matProjection", proj);

    gpuMeshShaderBinding mesh_binding_under;
    gpuMeshShaderBinding mesh_binding_over;
    gpuMakeMeshShaderBinding(&mesh_binding_under, prog, text_mesh_under, 0);
    gpuMakeMeshShaderBinding(&mesh_binding_over, prog, text_mesh_over, 0);
    SamplerBindings sampler_bindings_under;
    SamplerBindings sampler_bindings_over;
    gpuMakeSamplerBindings(prog, &samplers_under, &sampler_bindings_under);
    gpuMakeSamplerBindings(prog, &samplers_over, &sampler_bindings_over);

    glClear(GL_DEPTH_BUFFER_BIT);

    prog->setUniform4f("color", gfxm::vec4(0, 0, 0, 1));
    gpuBindSamplers(&sampler_bindings_under);
    gpuBindMeshBinding(&mesh_binding_under);
    gpuDrawMeshBinding(&mesh_binding_under);

    prog->setUniform4f("color", gfxm::vec4(1, .75f, 0, 1));
    gpuBindSamplers(&sampler_bindings_over);
    gpuBindMeshBinding(&mesh_binding_over);
    gpuDrawMeshBinding(&mesh_binding_over);

    glDeleteVertexArrays(1, &vao);
}
