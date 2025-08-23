#include "gpu/gpu.hpp"

#include "platform/platform.hpp"
#include "gpu/render_bucket.hpp"
#include "gpu/gpu_cube_map.hpp"
#include "gpu/common_resources.hpp"

#include "reflection/reflection.hpp"

static build_config::gpuPipelineCommon* s_pipeline = 0;
//static gpuRenderBucket* s_renderBucket = 0;
static GLuint s_global_vao = 0;
static gpuShaderProgram* prog_present_rgb = 0;
static gpuShaderProgram* prog_present_rrr = 0;
static gpuShaderProgram* prog_present_ggg = 0;
static gpuShaderProgram* prog_present_bbb = 0;
static gpuShaderProgram* prog_present_aaa = 0;
static gpuShaderProgram* prog_present_depth = 0;
gpuShaderProgram* gpu_prog_sample_cubemap = 0;

static gpuRenderTarget* s_default_render_target = 0;

std::unique_ptr<gpuAssetCache> asset_cache;

#include "readwrite/rw_gpu_material.hpp"
#include "readwrite/rw_gpu_texture_2d.hpp"
#include "readwrite/rw_gpu_cube_map.hpp"
#include "readwrite/rw_gpu_shader_program.hpp"
#include "readwrite/rw_gpu_mesh.hpp"

#include "resource_cache/res_cache_gpu_material.hpp"
#include "resource_cache/res_cache_shader_program.hpp"
#include "resource_cache/res_cache_texture_2d.hpp"
#include "resource_cache/res_cache_cube_map.hpp"
#include "resource_cache/res_cache_gpu_mesh.hpp"

#include "gpu_util.hpp"

bool gpuInit() {
    glGenVertexArrays(1, &s_global_vao);

    gpuUtilInit();

    type_register<gpuMesh>("gpuMesh")
        .custom_serialize_json([](nlohmann::json& j, void* object) {
            writeGpuMeshJson(j, (gpuMesh*)object);
        })
        .custom_deserialize_json([](const nlohmann::json& j, void* object) {
            readGpuMeshJson(j, (gpuMesh*)object);
        });
    type_register<gpuMaterial>("gpuMaterial")
        .custom_serialize_json([](nlohmann::json& j, void* object) {
            writeGpuMaterialJson(j, (gpuMaterial*)object);
        })
        .custom_deserialize_json([](const nlohmann::json& j, void* object) {
            readGpuMaterialJson(j, (gpuMaterial*)object);
        });
    type_register<gpuTexture2d>("gpuTexture2d")
        .custom_serialize_json([](nlohmann::json& j, void* object) {
            writeGpuTexture2dJson(j, (gpuTexture2d*)object);
        })
        .custom_deserialize_json([](const nlohmann::json& j, void* object) {
            readGpuTexture2dJson(j, (gpuTexture2d*)object);
        });
    type_register<gpuCubeMap>("gpuCubeMap")
        .custom_serialize_json([](nlohmann::json& j, void* object) {
            writeGpuCubeMapJson(j, (gpuCubeMap*)object);
        })
        .custom_deserialize_json([](const nlohmann::json& j, void* object) {
            readGpuCubeMapJson(j, (gpuCubeMap*)object);
        });
    type_register<gpuShaderProgram>("gpuShaderProgram")
        .custom_serialize_json([](nlohmann::json& j, void* object) {
            writeGpuShaderProgramJson(j, (gpuShaderProgram*)object);
        })
        .custom_deserialize_json([](const nlohmann::json& j, void* object) {
            readGpuShaderProgramJson(j, (gpuShaderProgram*)object);
        });


    s_pipeline = new build_config::gpuPipelineCommon;

    resAddCache<gpuShaderProgram>(new resCacheShaderProgram);
    resAddCache<gpuTexture2d>(new resCacheTexture2d);
    resAddCache<gpuMesh>(new resCacheGpuMesh());

    initCommonResources();

    s_pipeline->init();;
    //s_renderBucket = new gpuRenderBucket(pp, 10000);

    resAddCache<gpuMaterial>(new resCacheGpuMaterial(s_pipeline));


    {
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
        uniform sampler2D Depth;
        void main(){
            vec4 pix = texture(texAlbedo, uv_frag);
            float a = pix.a;
            outAlbedo = vec4(pix.rgb, 1);
	        gl_FragDepth = texture(Depth, uv_frag).x;
        })";
        prog_present_rgb = new gpuShaderProgram(vs, fs);
    }
    {
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
        uniform sampler2D Depth;
        void main(){
            vec4 pix = texture(texAlbedo, uv_frag);
            float a = pix.a;
            outAlbedo = vec4(pix.rrr, 1);
	        gl_FragDepth = texture(Depth, uv_frag).x;
        })";
        prog_present_rrr = new gpuShaderProgram(vs, fs);
    }
    {
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
        uniform sampler2D Depth;
        void main(){
            vec4 pix = texture(texAlbedo, uv_frag);
            float a = pix.a;
            outAlbedo = vec4(pix.ggg, 1);
	        gl_FragDepth = texture(Depth, uv_frag).x;
        })";
        prog_present_ggg = new gpuShaderProgram(vs, fs);
    }
    {
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
        uniform sampler2D Depth;
        void main(){
            vec4 pix = texture(texAlbedo, uv_frag);
            float a = pix.a;
            outAlbedo = vec4(pix.bbb, 1);
	        gl_FragDepth = texture(Depth, uv_frag).x;
        })";
        prog_present_bbb = new gpuShaderProgram(vs, fs);
    }
    {
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
        uniform sampler2D Depth;
        void main(){
            vec4 pix = texture(texAlbedo, uv_frag);
            float a = pix.a;
            outAlbedo = vec4(pix.aaa, 1);
	        gl_FragDepth = texture(Depth, uv_frag).x;
        })";
        prog_present_aaa = new gpuShaderProgram(vs, fs);
    }
    {
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
        uniform sampler2D Depth;

        float LinearizeDepth(float depth, float near, float far)
        {
            float z = depth * 2.0 - 1.0; // Back to NDC
            return (2.0 * near * far) / (far + near - z * (far - near));
        }

        void main(){
            vec4 pix = texture(texAlbedo, uv_frag);
            float a = pix.a;
            float zNear = .01;
            float zFar = 1000.;
            float dist = LinearizeDepth(pix.r, zNear, zFar);
            float depth = dist / (zFar - zNear);
            depth = sqrt(depth);
            outAlbedo = vec4(vec3(1) - vec3(depth), 1);
	        gl_FragDepth = texture(Depth, uv_frag).x;
        })";
        // TODO: zNear and zFar are hardcoded here, needs update
        prog_present_depth = new gpuShaderProgram(vs, fs);
    }
    {
        const char* vs = R"(
            #version 450 
            in vec3 inPosition;
            out vec3 local_pos;
            uniform mat4 matView;

            mat4 makeProjectionMatrix(float fov, float aspect, float znear, float zfar) {
                float tanHalfFovy = tan(fov / 2.0f);

                mat4 m = mat4(0.0);
                m[0][0] = 1.0f / (aspect * tanHalfFovy);
                m[1][1] = 1.0f / (tanHalfFovy);
                m[2][2] = -(zfar + znear) / (zfar - znear);
                m[2][3] = -1.0;
                m[3][2] = -(2.0 * zfar * znear) / (zfar - znear);
                return m;
            }

            const float PI = 3.14159265359;
        
            void main(){
                const mat4 matProjection = makeProjectionMatrix(
                    PI * .5, 1.0, 0.1, 10.0
                );
            
                local_pos = inPosition;
                gl_Position = matProjection * matView * vec4(inPosition, 1.0);
            })";
        const char* fs = R"(
            #version 450
            out vec4 outAlbedo;
            in vec3 local_pos;
            uniform sampler2D texAlbedo;
            const vec2 invAtan = vec2(0.1591, 0.3183);
            vec2 sampleSphericalMap(vec3 v) {
                vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
                uv *= invAtan;
                uv += 0.5;
                return uv;
            }
            void main() {
                vec2 uv = sampleSphericalMap(normalize(local_pos));
                vec3 color = texture(texAlbedo, uv).rgb;
                outAlbedo = vec4(color, 1.0);
            })";
        gpu_prog_sample_cubemap = new gpuShaderProgram(vs, fs);
    }

    {
        int screen_w = 0, screen_h = 0;
        platformGetWindowSize(screen_w, screen_h);
        s_default_render_target = new gpuRenderTarget(screen_w, screen_h);
        s_pipeline->initRenderTarget(s_default_render_target);
    }

    asset_cache.reset(new gpuAssetCache);

    return true;
}
void gpuCleanup() {
    asset_cache.reset(0);

    delete s_default_render_target;
    s_default_render_target = 0;

    delete gpu_prog_sample_cubemap;
    delete prog_present_rgb;
    delete prog_present_rrr;
    delete prog_present_ggg;
    delete prog_present_bbb;
    delete prog_present_aaa;
    delete prog_present_depth;

    cleanupCommonResources();

    gpuUtilCleanup();

    glDeleteVertexArrays(1, &s_global_vao);
}

build_config::gpuPipelineCommon* gpuGetPipeline() {
    return s_pipeline;
}

gpuRenderTarget* gpuGetDefaultRenderTarget() {
    return s_default_render_target;
}
/*
void gpuDrawRenderable(gpuRenderable* r) {
    s_renderBucket->add(r);
}

void gpuClearQueue() {
    s_renderBucket->clear();
}*/

#include "gpu_util.hpp"


#include "debug_draw/debug_draw.hpp"
void gpuDraw(gpuRenderBucket* bucket, gpuRenderTarget* target, const DRAW_PARAMS& params) {
    target->updateDirty();

    glDisable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glDisable(GL_LINE_SMOOTH);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);

    const gfxm::mat4& view = params.view;
    const gfxm::mat4& projection = params.projection;
    int vp_x = params.viewport_x;
    int vp_y = params.viewport_y;
    int vp_width = params.viewport_width;
    int vp_height = params.viewport_height;

    glViewport(vp_x, vp_y, vp_width, vp_height);
    glScissor(vp_x, vp_y, vp_width, vp_height);

    s_pipeline->setCamera3d(projection, view);
    s_pipeline->setViewportSize(vp_width, vp_height);
    s_pipeline->setViewportRectRatio(gfxm::vec4(params.vp_rect_ratio.min.x, params.vp_rect_ratio.min.y, params.vp_rect_ratio.max.x, params.vp_rect_ratio.max.y));
    s_pipeline->setTime(params.time);

    bucket->sort();
    s_pipeline->bindUniformBuffers();

    glBindVertexArray(s_global_vao);

    auto unbindTextures = []() {
        for (int i = 0; i < 16; ++i) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    };

    for (int i = 0; i < s_pipeline->passCount(); ++i) {
        auto pass = s_pipeline->getPass(i);
        
        if (pass->hasAnyFlags(PASS_FLAG_DISABLED | PASS_FLAG_NO_DRAW)) {
            continue;
        }

        glBindVertexArray(s_global_vao);
        pass->onDraw(target, bucket, i, params);
        //unbindTextures();
    }
    /*
    for (int i = 0; i < s_pipeline->techniqueCount(); ++i) {
        auto tech = s_pipeline->getTechnique(i);
        for (int j = 0; j < tech->passCount(); ++j) {
            auto pass = tech->getPass(j);
            glBindVertexArray(s_global_vao);
            pass->onDraw(target, bucket, tech->getId(), params);
            //unbindTextures();
        }
    }*/

    glBindVertexArray(0);

    s_pipeline->setCamera3dPrev(projection, view);
}

void gpuDrawLightmapSample(
    gpuMeshShaderBinding** bindings, int count, const DRAW_PARAMS& params
) {
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    s_pipeline->setCamera3d(params.projection, params.view);
    s_pipeline->setViewportSize(params.viewport_width, params.viewport_height);
    s_pipeline->bindUniformBuffers();

    glDisable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glDisable(GL_LINE_SMOOTH);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBlendFunc(GL_ONE, GL_ZERO);
    glDepthMask(GL_TRUE);
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_DEPTH_TEST);
    //glDepthFunc(GL_LESS);
    glCullFace(GL_BACK);
    //glEnable(GL_CULL_FACE);

    glViewport(params.viewport_x, params.viewport_y, params.viewport_width, params.viewport_height);
    glScissor(params.viewport_x, params.viewport_y, params.viewport_width, params.viewport_height);

    for (int i = 0; i < count; ++i) {
        auto binding = bindings[i];
        gpuBindMeshBinding(binding);
        gpuDrawMeshBinding(binding);
    }

    glBindVertexArray(0);
    glDeleteVertexArrays(1, &vao);
}

void gpuDrawUVWires(
    gpuMeshShaderBinding** bindings, int count, const DRAW_PARAMS& params
) {
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    s_pipeline->setCamera3d(params.projection, params.view);
    s_pipeline->setViewportSize(params.viewport_width, params.viewport_height);
    s_pipeline->bindUniformBuffers();

    glDisable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glDisable(GL_LINE_SMOOTH);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBlendFunc(GL_ONE, GL_ZERO);
    glDepthFunc(GL_LESS);
    glCullFace(GL_BACK);

    glViewport(params.viewport_x, params.viewport_y, params.viewport_width, params.viewport_height);
    glScissor(params.viewport_x, params.viewport_y, params.viewport_width, params.viewport_height);


    for (int i = 0; i < count; ++i) {
        auto binding = bindings[i];
        gpuBindMeshBinding(binding);
        gpuDrawMeshBinding(binding);
    }

    glBindVertexArray(0);
    glDeleteVertexArrays(1, &vao);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void gpuDrawTextureToDefaultFrameBuffer(gpuTexture2d* texture, gpuTexture2d* depth, RT_OUTPUT output_mode, const gfxm::rect& rc_ratio) {
    GLint internalFormat = texture->getInternalFormat();
    gpuShaderProgram* prog_present = prog_present_rgb;
    switch(output_mode) {
    case RT_OUTPUT_AUTO: {
        if (internalFormat == GL_RED) {
            prog_present = prog_present_rrr;
        } else if (internalFormat == GL_DEPTH_COMPONENT) {
            prog_present = prog_present_depth;
        }
        break;
    }
    case RT_OUTPUT_RGB: { prog_present = prog_present_rgb; break; }
    case RT_OUTPUT_RRR: { prog_present = prog_present_rrr; break; }
    case RT_OUTPUT_GGG: { prog_present = prog_present_ggg; break; }
    case RT_OUTPUT_BBB: { prog_present = prog_present_bbb; break; }
    case RT_OUTPUT_AAA: { prog_present = prog_present_aaa; break; }
    case RT_OUTPUT_DEPTH: { prog_present = prog_present_depth; break; }
    default:
        assert(false);
    }

    int screen_w = 0, screen_h = 0;
    platformGetWindowSize(screen_w, screen_h);
    gfxm::rect rc = gfxm::rect(
        screen_w * rc_ratio.min.x, screen_h * rc_ratio.min.y,
        screen_w * rc_ratio.max.x, screen_h * rc_ratio.max.y
    );

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(rc.min.x, rc.min.y, rc.max.x - rc.min.x, rc.max.y - rc.min.y);
    glScissor(rc.min.x, rc.min.y, rc.max.x - rc.min.x, rc.max.y - rc.min.y);
    //glViewport(0, 0, screen_w, screen_h);
    //glScissor(0, 0, screen_w, screen_h);
    
    glActiveTexture(GL_TEXTURE0 + prog_present->getDefaultSamplerSlot("texAlbedo"));
    glBindTexture(GL_TEXTURE_2D, texture->getId());
    if (depth) {
        glActiveTexture(GL_TEXTURE0 + prog_present->getDefaultSamplerSlot("Depth"));
        glBindTexture(GL_TEXTURE_2D, depth->getId());
    }
    glUseProgram(prog_present->getId());
    gpuDrawFullscreenTriangle();
    glUseProgram(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindVertexArray(0);
}

void gpuDrawToDefaultFrameBuffer(gpuRenderTarget* target, const gfxm::rect& rc_ratio) {
    assert(target->getPipeline());
    assert(target->layers.size());
    assert(target->default_output_texture >= 0 && target->default_output_texture < target->layers.size());

    auto& target_channel = target->layers[target->default_output_texture];
    const gpuPipeline::RenderChannel* pipeline_channel =
        target->getPipeline()->getChannel(target->default_output_texture);

    gpuDrawTextureToDefaultFrameBuffer(
        target_channel.textures[pipeline_channel->lwt].get(),
        target->depth_texture,
        target->default_output_mode,
        rc_ratio
    );
}

void gpuDrawTextureToFramebuffer(gpuTexture2d* texture, GLuint framebuffer, int* vp) {
    GLint internalFormat = texture->getInternalFormat();
    gpuShaderProgram* prog_present = prog_present_rgb;
    if (internalFormat == GL_RED) {
        prog_present = prog_present_rrr;
    } else if (internalFormat == GL_DEPTH_COMPONENT) {
        prog_present = prog_present_depth;
    }
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    glViewport(vp[0], vp[1], vp[2], vp[3]);
    glScissor(vp[0], vp[1], vp[2], vp[3]);
    
    glActiveTexture(GL_TEXTURE0 + prog_present->getDefaultSamplerSlot("texAlbedo"));
    glBindTexture(GL_TEXTURE_2D, texture->getId());
    glUseProgram(prog_present->getId());
    gpuDrawFullscreenTriangle();
    glUseProgram(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindVertexArray(0);
}

gpuAssetCache* gpuGetAssetCache() {
    return asset_cache.get();
}
