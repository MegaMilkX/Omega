#include "gpu/gpu.hpp"

#include "platform/platform.hpp"
#include "gpu/render_bucket.hpp"
#include "gpu/gpu_cube_map.hpp"
#include "gpu/common_resources.hpp"
#include "gpu/skinning/skinning_compute.hpp"

#include "reflection/reflection.hpp"

static build_config::gpuPipelineCommon* s_pipeline = 0;

static gpuRenderTarget* s_default_render_target = 0;

std::unique_ptr<gpuAssetCache> asset_cache;

static gpuDevice* s_device = nullptr;

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
    gpuUtilInit();

    type_register<gpuMesh>("gpuMesh")
        .custom_serialize_json([](nlohmann::json& j, const void* object) {
            writeGpuMeshJson(j, (gpuMesh*)object);
        })
        .custom_deserialize_json([](const nlohmann::json& j, void* object) {
            readGpuMeshJson(j, (gpuMesh*)object);
        });
    type_register<gpuMaterial>("gpuMaterial")
        .custom_serialize_json([](nlohmann::json& j, const void* object) {
            writeGpuMaterialJson(j, (gpuMaterial*)object);
        })
        .custom_deserialize_json([](const nlohmann::json& j, void* object) {
            readGpuMaterialJson(j, (gpuMaterial*)object);
        });
    type_register<gpuTexture2d>("gpuTexture2d")
        .custom_serialize_json([](nlohmann::json& j, const void* object) {
            writeGpuTexture2dJson(j, (gpuTexture2d*)object);
        })
        .custom_deserialize_json([](const nlohmann::json& j, void* object) {
            readGpuTexture2dJson(j, (gpuTexture2d*)object);
        });
    type_register<gpuCubeMap>("gpuCubeMap")
        .custom_serialize_json([](nlohmann::json& j, const void* object) {
            writeGpuCubeMapJson(j, (gpuCubeMap*)object);
        })
        .custom_deserialize_json([](const nlohmann::json& j, void* object) {
            readGpuCubeMapJson(j, (gpuCubeMap*)object);
        });
    type_register<gpuShaderProgram>("gpuShaderProgram")
        .custom_serialize_json([](nlohmann::json& j, const void* object) {
            writeGpuShaderProgramJson(j, (gpuShaderProgram*)object);
        })
        .custom_deserialize_json([](const nlohmann::json& j, void* object) {
            readGpuShaderProgramJson(j, (gpuShaderProgram*)object);
        });

    s_device = new gpuDevice();
    s_pipeline = new build_config::gpuPipelineCommon;

    resAddCache<gpuShaderProgram>(new resCacheShaderProgram);
    resAddCache<gpuTexture2d>(new resCacheTexture2d);
    resAddCache<gpuMesh>(new resCacheGpuMesh());

    initCommonResources();

    s_pipeline->init();
    //s_renderBucket = new gpuRenderBucket(pp, 10000);

    resAddCache<gpuMaterial>(new resCacheGpuMaterial(s_pipeline));

    {
        int screen_w = 0, screen_h = 0;
        platformGetWindowSize(screen_w, screen_h);
        s_default_render_target = new gpuRenderTarget(screen_w, screen_h);
        s_pipeline->initRenderTarget(s_default_render_target);
    }

    asset_cache.reset(new gpuAssetCache);

    gpuInitSkinning();

    return true;
}

void gpuCleanup() {
    gpuCleanupSkinning();

    delete s_device;
    s_device = nullptr;

    asset_cache.reset(0);

    delete s_default_render_target;
    s_default_render_target = 0;

    cleanupCommonResources();

    gpuUtilCleanup();
}

gpuDevice* gpuGetDevice() {
    return s_device;
}

build_config::gpuPipelineCommon* gpuGetPipeline() {
    return s_pipeline;
}

gpuRenderTarget* gpuGetDefaultRenderTarget() {
    return s_default_render_target;
}


static TransformDirtyList_T<gpuTransformBlock> transform_dirty_list;
void gpuAddTransformSync(gpuTransformBlock* block, HTransform node) {
    if (block->ticket) {
        gpuRemoveTransformSync(block);
    }
    auto t = transform_dirty_list.createTicket(block);
    node->attachTicket(t);
    block->ticket = t;
    block->transform_node = node;
}
void gpuRemoveTransformSync(gpuTransformBlock* block) {
    block->transform_node->detachTicket(block->ticket);
    transform_dirty_list.destroyTicket(block->ticket);
    block->ticket = nullptr;
    block->transform_node = HTransform();
}

#include "gpu_util.hpp"


#include "debug_draw/debug_draw.hpp"
void gpuDraw(gpuRenderBucket* bucket, gpuRenderTarget* target, const DRAW_PARAMS& params) {
    target->updateDirty();

    for (int i = 0; i < transform_dirty_list.dirtyCount(); ++i) {
        auto d = transform_dirty_list.getDirty(i);
        auto block = static_cast<gpuTransformBlock*>(d->user_ptr);
        block->setTransform(block->transform_node->getWorldTransform());
    }
    transform_dirty_list.clearDirty();

    const gfxm::mat4& view = params.view;
    const gfxm::mat4& projection = params.projection;
    int vp_x = params.viewport_x;
    int vp_y = params.viewport_y;
    int vp_width = params.viewport_width;
    int vp_height = params.viewport_height;
    s_pipeline->setCamera3d(projection, view);
    s_pipeline->setCamera3dPrev(projection, params.view_prev);
    s_pipeline->setViewportSize(vp_width, vp_height);
    s_pipeline->setViewportRectRatio(gfxm::vec4(params.vp_rect_ratio.min.x, params.vp_rect_ratio.min.y, params.vp_rect_ratio.max.x, params.vp_rect_ratio.max.y));
    s_pipeline->setTime(params.time);
    gpuGetPipeline()->updateParamBlocks();

    gpuRunSkinTasks();

    glDisable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glDisable(GL_LINE_SMOOTH);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);

    glViewport(vp_x, vp_y, vp_width, vp_height);
    glScissor(vp_x, vp_y, vp_width, vp_height);

    s_pipeline->draw(target, bucket, params);
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
    auto shared = gpuGetDevice()->getSharedResources();
    gpuShaderProgram* prog_present = shared->getPresentProgram(RT_OUTPUT_RGB);
    switch(output_mode) {
    case RT_OUTPUT_AUTO: {
        if (internalFormat == GL_RED) {
            prog_present = shared->getPresentProgram(RT_OUTPUT_RGB);
        } else if (internalFormat == GL_DEPTH_COMPONENT) {
            prog_present = shared->getPresentProgram(RT_OUTPUT_DEPTH);
        }
        break;
    }
    default:
        prog_present = shared->getPresentProgram(output_mode);
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
        target_channel.textures[target_channel.lwt].get(),
        target->depth_texture,
        target->default_output_mode,
        rc_ratio
    );
}

void gpuDrawTextureToFramebuffer(gpuTexture2d* texture, GLuint framebuffer, int* vp) {
    GLint internalFormat = texture->getInternalFormat();
    auto shared = gpuGetDevice()->getSharedResources();
    gpuShaderProgram* prog_present = shared->getPresentProgram(RT_OUTPUT_RGB);
    if (internalFormat == GL_RED) {
        prog_present = shared->getPresentProgram(RT_OUTPUT_RRR);
    } else if (internalFormat == GL_DEPTH_COMPONENT) {
        prog_present = shared->getPresentProgram(RT_OUTPUT_DEPTH);
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
