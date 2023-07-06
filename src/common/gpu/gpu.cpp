#include "gpu/gpu.hpp"

#include "platform/platform.hpp"
#include "gpu/render_bucket.hpp"
#include "gpu/gpu_cube_map.hpp"

#include "reflection/reflection.hpp"

static build_config::gpuPipelineCommon* s_pipeline = 0;
//static gpuRenderBucket* s_renderBucket = 0;
static gpuShaderProgram* prog_present = 0;
gpuShaderProgram* gpu_prog_sample_cubemap = 0;
static GLuint fullscreen_triangle_vao = 0;
static GLuint fullscreen_triangle_vbo = 0;
static GLuint cube_map_cube_vao = 0;
static GLuint cube_map_cube_vbo = 0;

static gpuRenderTarget* s_default_render_target = 0;

static RHSHARED<gpuShaderProgram> prog_pbr_compose;
static RHSHARED<gpuShaderProgram> prog_pbr_light;
static RHSHARED<gpuShaderProgram> prog_shadowmap;
static RHSHARED<gpuShaderProgram> prog_skybox;
static RHSHARED<gpuCubeMap> sky_cube_map;
static HSHARED<gpuCubeMap> cube_map_shadow;

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

bool gpuInit(build_config::gpuPipelineCommon* pp) {
    s_pipeline = pp;
    //s_renderBucket = new gpuRenderBucket(pp, 10000);

    type_register<gpuMesh>("gpuMesh")
        .custom_serialize_json([](nlohmann::json& j, void* object) {
            writeGpuMeshJson(j, (gpuMesh*)object);
        })
        .custom_deserialize_json([](nlohmann::json& j, void* object) {
            readGpuMeshJson(j, (gpuMesh*)object);
        });
    type_register<gpuMaterial>("gpuMaterial")
        .custom_serialize_json([](nlohmann::json& j, void* object) {
            writeGpuMaterialJson(j, (gpuMaterial*)object);
        })
        .custom_deserialize_json([](nlohmann::json& j, void* object) {
            readGpuMaterialJson(j, (gpuMaterial*)object);
        });
    type_register<gpuTexture2d>("gpuTexture2d")
        .custom_serialize_json([](nlohmann::json& j, void* object) {
            writeGpuTexture2dJson(j, (gpuTexture2d*)object);
        })
        .custom_deserialize_json([](nlohmann::json& j, void* object) {
            readGpuTexture2dJson(j, (gpuTexture2d*)object);
        });
    type_register<gpuCubeMap>("gpuCubeMap")
        .custom_serialize_json([](nlohmann::json& j, void* object) {
            writeGpuCubeMapJson(j, (gpuCubeMap*)object);
        })
        .custom_deserialize_json([](nlohmann::json& j, void* object) {
            readGpuCubeMapJson(j, (gpuCubeMap*)object);
        });
    //type_register<gpuCubeMap>("gpuCubeMap");
    type_register<gpuShaderProgram>("gpuShaderProgram")
        .custom_serialize_json([](nlohmann::json& j, void* object) {
            writeGpuShaderProgramJson(j, (gpuShaderProgram*)object);
        })
        .custom_deserialize_json([](nlohmann::json& j, void* object) {
            readGpuShaderProgramJson(j, (gpuShaderProgram*)object);
        });


    resAddCache<gpuShaderProgram>(new resCacheShaderProgram);
    resAddCache<gpuTexture2d>(new resCacheTexture2d);
    resAddCache<gpuMaterial>(new resCacheGpuMaterial(pp));
    resAddCache<gpuMesh>(new resCacheGpuMesh());

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
        prog_present = new gpuShaderProgram(vs, fs);
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
        float vertices[] = {
            -1.0f, -1.0f, 0.0f,     3.0f, -1.0f, 0.0f,      -1.0f, 3.0f, 0.0f
        };

        glGenVertexArrays(1, &fullscreen_triangle_vao);
        glBindVertexArray(fullscreen_triangle_vao);
        glGenBuffers(1, &fullscreen_triangle_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, fullscreen_triangle_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            0,
            3, GL_FLOAT, GL_FALSE,
            0, (void*)0 /* offset */
        );

        glBindVertexArray(0);
    }
    {
        static const GLfloat vertices[] = {
            -1.0f,-1.0f,-1.0f,
            -1.0f, 1.0f, 1.0f,
            -1.0f,-1.0f, 1.0f,
            1.0f, 1.0f,-1.0f,
            -1.0f, 1.0f,-1.0f,
            -1.0f,-1.0f,-1.0f,
            1.0f,-1.0f, 1.0f,
            1.0f,-1.0f,-1.0f,
            -1.0f,-1.0f,-1.0f,
            1.0f, 1.0f,-1.0f,
            -1.0f,-1.0f,-1.0f,
            1.0f,-1.0f,-1.0f,
            -1.0f,-1.0f,-1.0f,
            -1.0f, 1.0f,-1.0f,
            -1.0f, 1.0f, 1.0f,
            1.0f,-1.0f, 1.0f,
            -1.0f,-1.0f,-1.0f,
            -1.0f,-1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,
            1.0f,-1.0f, 1.0f,
            -1.0f,-1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f,-1.0f,
            1.0f,-1.0f,-1.0f,
            1.0f,-1.0f,-1.0f,
            1.0f,-1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f,-1.0f,
            1.0f, 1.0f,-1.0f,
            1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f,-1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f,-1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f
        };

        glGenVertexArrays(1, &cube_map_cube_vao);
        glBindVertexArray(cube_map_cube_vao);
        glGenBuffers(1, &cube_map_cube_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, cube_map_cube_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            0,
            3, GL_FLOAT, GL_FALSE,
            0, (void*)0 /* offset */
        );

        glBindVertexArray(0);
    }

    {
        int screen_w = 0, screen_h = 0;
        platformGetWindowSize(screen_w, screen_h);
        s_default_render_target = new gpuRenderTarget(screen_w, screen_h);
        pp->initRenderTarget(s_default_render_target);
    }

    {
        prog_pbr_compose = resGet<gpuShaderProgram>("shaders/postprocess/pbr_compose.glsl");
        prog_pbr_light = resGet<gpuShaderProgram>("shaders/postprocess/pbr_light.glsl");
        prog_shadowmap = resGet<gpuShaderProgram>("shaders/shadowmap.glsl");
        prog_skybox = resGet<gpuShaderProgram>("shaders/postprocess/skybox.glsl");
    }

    {
        cube_map_shadow.reset_acquire();
        cube_map_shadow->reserve(1024, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT);
    }

    {
        sky_cube_map.reset_acquire();
        ktImage posx;
        ktImage negx;
        ktImage posy;
        ktImage negy;
        ktImage posz;
        ktImage negz;
        if (!loadImage(&posx, "cubemaps/Yokohama3/posx.jpg", false)) {
            LOG_ERR("Failed to load cube map part");
        }
        if (!loadImage(&negx, "cubemaps/Yokohama3/negx.jpg", false)) {
            LOG_ERR("Failed to load cube map part");
        }
        if (!loadImage(&posy, "cubemaps/Yokohama3/posy.jpg", false)) {
            LOG_ERR("Failed to load cube map part");
        }
        if (!loadImage(&negy, "cubemaps/Yokohama3/negy.jpg", false)) {
            LOG_ERR("Failed to load cube map part");
        }
        if (!loadImage(&posz, "cubemaps/Yokohama3/posz.jpg", false)) {
            LOG_ERR("Failed to load cube map part");
        }
        if (!loadImage(&negz, "cubemaps/Yokohama3/negz.jpg", false)) {
            LOG_ERR("Failed to load cube map part");
        }
        sky_cube_map->build(
            &posx, &negx, &posy, &negy, &posz, &negz
        );
    }

    return true;
}
void gpuCleanup() {
    delete s_default_render_target;
    s_default_render_target = 0;

    glDeleteBuffers(1, &fullscreen_triangle_vbo);
    glDeleteVertexArrays(1, &fullscreen_triangle_vao);
    delete gpu_prog_sample_cubemap;
    delete prog_present;
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

void gpuBindMeshBinding(const gpuMeshShaderBinding* binding) {
    for (auto& a : binding->attribs) {
        if (!a.buffer) {
            assert(false);
            continue;
        }
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, a.buffer->getId()));
        glEnableVertexAttribArray(a.location);
        glVertexAttribPointer(
            a.location, a.count, a.gl_type, a.normalized, a.stride, (void*)0
        );
        if (a.is_instance_array) {
            glVertexAttribDivisor(a.location, 1);
        }
    }
    if (binding->index_buffer) {
        GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, binding->index_buffer->getId()));
    }
}
void gpuDrawMeshBinding(const gpuMeshShaderBinding* b) {
    GLenum mode;
    switch (b->draw_mode) {
    case MESH_DRAW_POINTS: mode = GL_POINTS; break;
    case MESH_DRAW_LINES: mode = GL_LINES; break;
    case MESH_DRAW_LINE_STRIP: mode = GL_LINE_STRIP; break;
    case MESH_DRAW_LINE_LOOP: mode = GL_LINE_LOOP; break;
    case MESH_DRAW_TRIANGLES: mode = GL_TRIANGLES; break;
    case MESH_DRAW_TRIANGLE_STRIP: mode = GL_TRIANGLE_STRIP; break;
    case MESH_DRAW_TRIANGLE_FAN: mode = GL_TRIANGLE_FAN; break;
    default: assert(false);
    };
    if (b->index_buffer) {
        glDrawElements(mode, b->index_count, GL_UNSIGNED_INT, 0);
    } else {
        glDrawArrays(mode, 0, b->vertex_count);
    }
}
void gpuDrawMeshBindingInstanced(const gpuMeshShaderBinding* binding, int instance_count) {
    GLenum mode;
    switch (binding->draw_mode) {
    case MESH_DRAW_POINTS: mode = GL_POINTS; break;
    case MESH_DRAW_LINES: mode = GL_LINES; break;
    case MESH_DRAW_LINE_STRIP: mode = GL_LINE_STRIP; break;
    case MESH_DRAW_LINE_LOOP: mode = GL_LINE_LOOP; break;
    case MESH_DRAW_TRIANGLES: mode = GL_TRIANGLES; break;
    case MESH_DRAW_TRIANGLE_STRIP: mode = GL_TRIANGLE_STRIP; break;
    case MESH_DRAW_TRIANGLE_FAN: mode = GL_TRIANGLE_FAN; break;
    default: assert(false);
    };
    if (binding->index_buffer) {
        glDrawElementsInstanced(mode, binding->index_count, GL_UNSIGNED_INT, 0, instance_count);
    } else {
        glDrawArraysInstanced(mode, 0, binding->vertex_count, instance_count);
    }
}

void drawPass(gpuPipeline* pipe, gpuRenderTarget* target, gpuRenderBucket* bucket, gpuPipelineTechnique* pipe_tech, gpuPipelinePass* pipe_pass) {
    auto framebuffer_id = pipe_pass->getFrameBufferId();
    assert(framebuffer_id >= 0);
    gpuFrameBufferBind(target->framebuffers[framebuffer_id].get());

    glViewport(0, 0, target->getWidth(), target->getHeight());
    glScissor(0, 0, target->getWidth(), target->getHeight());

    auto group = bucket->getTechniqueGroup(pipe_tech->getId());
    for (int i = group.start; i < group.end;) { // all commands of the same technique
        auto& cmd = bucket->commands[i];
        int material_end = cmd.next_material_id;

        const gpuMaterial* material = cmd.renderable->getMaterial();
        //material->bindSamplers();
        material->bindUniformBuffers();

        for (; i < material_end;) { // iterate over commands with the same material
            auto material_tech = cmd.renderable->getMaterial()->getTechniqueByPipelineId(cmd.id.getTechnique());
            auto pass = material_tech->getPass(cmd.id.getPass());
            pass->depth_test ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
            pass->stencil_test ? glEnable(GL_STENCIL_TEST) : glDisable(GL_STENCIL_TEST);
            pass->cull_faces ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
            pass->depth_write ? glDepthMask(GL_TRUE) : glDepthMask(GL_FALSE);
            switch (pass->blend_mode) {
            case GPU_BLEND_MODE::NORMAL:
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                break;
            case GPU_BLEND_MODE::ADD:
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                break;
            case GPU_BLEND_MODE::MULTIPLY:
                glBlendFunc(GL_DST_COLOR, GL_ZERO);
                break;
            default:
                assert(false);
            }
            pass->bindSamplers();
            for (int pobid = 0; pobid < pass->passOutputBindingCount(); ++pobid) {
                auto& pob = pass->getPassOutputBinding(pobid);
                glActiveTexture(GL_TEXTURE0 + pob.texture_slot);
                auto& texture = target->textures[pipe_pass->getTargetSamplerTextureIndex(pob.strid)];
                glBindTexture(GL_TEXTURE_2D, texture->getId());
            }
            pass->bindDrawBuffers();/*
            GLenum draw_buffers[] = {
                GL_COLOR_ATTACHMENT0 + 0,
                GL_COLOR_ATTACHMENT0 + 1,
                GL_COLOR_ATTACHMENT0 + 2,
                GL_COLOR_ATTACHMENT0 + 3,
                GL_COLOR_ATTACHMENT0 + 4,
                0,
                0,
                0,
                0,
            };
            glDrawBuffers(GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS, draw_buffers);*/
            pass->bindShaderProgram();

            int pass_end = cmd.next_pass_id;
            for (; i < pass_end; ++i) { // iterate over commands with the same shader(pass)
                auto& cmd = bucket->commands[i];
                if (cmd.instance_count > 0) { // TODO: possible instance count mismatch in cmd
                    cmd.renderable->bindUniformBuffers();
                    gpuBindMeshBinding(cmd.binding);
                    gpuDrawMeshBindingInstanced(cmd.binding, cmd.renderable->getInstancingDesc()->getInstanceCount());
                } else {
                    cmd.renderable->bindUniformBuffers();
                    gpuBindMeshBinding(cmd.binding);
                    gpuDrawMeshBinding(cmd.binding);
                }
            }
        }
    }
};

void drawTechnique(gpuPipeline* pipeline, gpuRenderTarget* target, gpuRenderBucket* bucket, gpuPipelineTechnique* pipe_tech) {
    for (int i = 0; i < pipe_tech->passCount(); ++i) {
        auto pipe_pass = pipe_tech->getPass(i);
        drawPass(pipeline, target, bucket, pipe_tech, pipe_pass);
    }
}


void gpuDrawShadowCubeMap(gpuRenderTarget* target, gpuRenderBucket* bucket, const gfxm::vec3& eye, gpuCubeMap* cubemap) {
    gfxm::mat4 proj = gfxm::perspective(gfxm::radian(90.0f), 1.0f, 0.1f, 1000.0f);
    gfxm::mat4 views[] =
    {
        gfxm::lookAt(eye, eye + gfxm::vec3(1.0f,  0.0f,  0.0f), gfxm::vec3(0.0f, -1.0f,  0.0f)),
        gfxm::lookAt(eye, eye + gfxm::vec3(-1.0f,  0.0f,  0.0f), gfxm::vec3(0.0f, -1.0f,  0.0f)),
        gfxm::lookAt(eye, eye + gfxm::vec3(0.0f,  1.0f,  0.0f), gfxm::vec3(0.0f,  0.0f,  1.0f)),
        gfxm::lookAt(eye, eye + gfxm::vec3(0.0f, -1.0f,  0.0f), gfxm::vec3(0.0f,  0.0f, -1.0f)),
        gfxm::lookAt(eye, eye + gfxm::vec3(0.0f,  0.0f,  1.0f), gfxm::vec3(0.0f, -1.0f,  0.0f)),
        gfxm::lookAt(eye, eye + gfxm::vec3(0.0f,  0.0f, -1.0f), gfxm::vec3(0.0f, -1.0f,  0.0f))
    };

    const int side = 1024;

    GLuint capFbo;
    glGenFramebuffers(1, &capFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, capFbo);
    // Disable writes to the color buffer
    //glDrawBuffer(GL_NONE);
    // Disable reads from the color buffer
    //glReadBuffer(GL_NONE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X, cubemap->getId(), 0);
    GLenum draw_buffers[GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    glDrawBuffers(GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS, draw_buffers);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERR("Shadow cube map fbo not complete!");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    glViewport(0, 0, side, side);
    glScissor(0, 0, side, side);
    
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ZERO);
    glDepthMask(GL_TRUE);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, capFbo);

    for (unsigned s = 0; s < 6; ++s) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + s, cubemap->getId(), 0);
        GLenum err = glGetError();
        assert(err == GL_NO_ERROR);
        glClear(GL_DEPTH_BUFFER_BIT);

        s_pipeline->setShadowmapCamera(proj, views[s]);
        
        static auto pipe_tech = s_pipeline->findTechnique("ShadowCubeMap");
        for (int j = 0; j < pipe_tech->passCount(); ++j) {
            auto pipe_pass = pipe_tech->getPass(j);
            
            auto group = bucket->getTechniqueGroup(pipe_tech->getId());
            for (int i = group.start; i < group.end;) { // all commands of the same technique
                auto& cmd = bucket->commands[i];
                int material_end = cmd.next_material_id;

                const gpuMaterial* material = cmd.renderable->getMaterial();
                //material->bindSamplers();
                material->bindUniformBuffers();
                
                for (; i < material_end;) { // iterate over commands with the same material
                    auto material_tech = cmd.renderable->getMaterial()->getTechniqueByPipelineId(cmd.id.getTechnique());
                    auto pass = material_tech->getPass(cmd.id.getPass());
                    pass->depth_test ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
                    pass->stencil_test ? glEnable(GL_STENCIL_TEST) : glDisable(GL_STENCIL_TEST);
                    pass->cull_faces ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
                    pass->depth_write ? glDepthMask(GL_TRUE) : glDepthMask(GL_FALSE);
                    switch (pass->blend_mode) {
                    case GPU_BLEND_MODE::NORMAL:
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                        break;
                    case GPU_BLEND_MODE::ADD:
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                        break;
                    case GPU_BLEND_MODE::MULTIPLY:
                        glBlendFunc(GL_DST_COLOR, GL_ZERO);
                        break;
                    default:
                        assert(false);
                    }
                    pass->bindSamplers();
                    for (int pobid = 0; pobid < pass->passOutputBindingCount(); ++pobid) {
                        auto& pob = pass->getPassOutputBinding(pobid);
                        glActiveTexture(GL_TEXTURE0 + pob.texture_slot);
                        auto& texture = target->textures[pipe_pass->getTargetSamplerTextureIndex(pob.strid)];
                        glBindTexture(GL_TEXTURE_2D, texture->getId());
                    }
                    pass->bindDrawBuffers();
                    pass->bindShaderProgram();

                    int pass_end = cmd.next_pass_id;
                    for (; i < pass_end; ++i) { // iterate over commands with the same shader(pass)
                        auto& cmd = bucket->commands[i];
                        if (cmd.instance_count > 0) { // TODO: possible instance count mismatch in cmd
                            cmd.renderable->bindUniformBuffers();
                            gpuBindMeshBinding(cmd.binding);
                            gpuDrawMeshBindingInstanced(cmd.binding, cmd.renderable->getInstancingDesc()->getInstanceCount());
                        } else {
                            cmd.renderable->bindUniformBuffers();
                            gpuBindMeshBinding(cmd.binding);
                            gpuDrawMeshBinding(cmd.binding);
                        }
                    }
                    glUseProgram(0);
                }
            }
        }
    }

    glCullFace(GL_BACK);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &capFbo);
}

#include "debug_draw/debug_draw.hpp"
void gpuDraw(gpuRenderBucket* bucket, gpuRenderTarget* target, const gfxm::mat4& view, const gfxm::mat4& projection) {
    glDisable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glDisable(GL_LINE_SMOOTH);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);

    glViewport(0, 0, target->getWidth(), target->getHeight());
    glScissor(0, 0, target->getWidth(), target->getHeight());

    //glClearColor(0.129f, 0.586f, 0.949f, 1.0f);
    glClearColor(0.f, 0.f, 0.f, 1.0f);
    for (int i = 0; i < target->framebuffers.size(); ++i) {
        gpuFrameBufferBind(target->framebuffers[i].get());
        GLenum draw_buffers[] = {
            GL_COLOR_ATTACHMENT0 + 0,
            GL_COLOR_ATTACHMENT0 + 1,
            GL_COLOR_ATTACHMENT0 + 2,
            GL_COLOR_ATTACHMENT0 + 3,
            GL_COLOR_ATTACHMENT0 + 4,
            GL_COLOR_ATTACHMENT0 + 5,
            GL_COLOR_ATTACHMENT0 + 6,
            GL_COLOR_ATTACHMENT0 + 7,
            GL_COLOR_ATTACHMENT0 + 8,
        };
        glDrawBuffers(GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS, draw_buffers);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }
    gpuFrameBufferUnbind();

    bucket->sort();
    s_pipeline->bindUniformBuffers();
    
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    GLuint gvao;
    glGenVertexArrays(1, &gvao);
    glBindVertexArray(gvao);

    auto unbindTextures = []() {
        for (int i = 0; i < 16; ++i) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    };
    
    {
        static auto pipe_tech = s_pipeline->findTechnique("Normal");
        drawTechnique(s_pipeline, target, bucket, pipe_tech);
        unbindTextures();
    }
    {
        static auto pipe_tech = s_pipeline->findTechnique("Debug");
        drawTechnique(s_pipeline, target, bucket, pipe_tech);
        unbindTextures();
    }
    {
        static auto pipe_tech = s_pipeline->findTechnique("GUI");
        drawTechnique(s_pipeline, target, bucket, pipe_tech);
        unbindTextures();
    }
    {
        static auto pipe_tech = s_pipeline->findTechnique("LightPass");
        static auto pipe_pass = pipe_tech->getPass(0);
        auto framebuffer_id = pipe_pass->getFrameBufferId();
        assert(framebuffer_id >= 0);

        struct OmniLight {
            gfxm::vec3 pos;
            gfxm::vec3 color;
            float intensity;
        };
        std::list<OmniLight> lights;
        static float t = .0f;
        t += .001f;
        OmniLight light;
        light.pos = gfxm::vec3(cosf(t) * 2.f, 2.f, sinf(t) * 2.f - 2.f);
        //light.color = gfxm::vec3(0, 1, .1);
        light.color = gfxm::vec3(.2, 1., 0.4);
        light.intensity = 20.f;
        lights.push_back(light);

        light.pos = gfxm::vec3(cosf(t + gfxm::pi) * 2.f, 2.f, sinf(t + gfxm::pi) * 2.f - 2.f);
        //light.color = gfxm::vec3(1, 0, .3);
        light.color = gfxm::vec3(.4, 0.2, 1.);
        lights.push_back(light);

        light.pos = gfxm::inverse(view)[3];
        light.color = gfxm::vec3(1, 0.2, 0.1);
        lights.push_back(light);

        for (auto& l : lights) {
            glBindVertexArray(gvao);
            gpuDrawShadowCubeMap(target, bucket, l.pos, cube_map_shadow.get());
            glBindVertexArray(0);

            gpuFrameBufferBind(target->framebuffers[framebuffer_id].get());
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            glEnable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);
            glDisable(GL_SCISSOR_TEST);
            glDisable(GL_STENCIL_TEST);
            glDisable(GL_LINE_SMOOTH);
            glDepthMask(GL_TRUE);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0, 0, 0, 0, 0, 0, 0, 0, 0, };
            glDrawBuffers(GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS, draw_buffers);

            int slot = prog_pbr_light->getDefaultSamplerSlot("Albedo");
            if (slot != -1) {
                glActiveTexture(GL_TEXTURE0 + slot);
                glBindTexture(GL_TEXTURE_2D, target->textures[pipe_pass->getTargetSamplerTextureIndex(string_id("Albedo"))]->getId());
            }
            slot = prog_pbr_light->getDefaultSamplerSlot("Position");
            if (slot != -1) {
                glActiveTexture(GL_TEXTURE0 + slot);
                glBindTexture(GL_TEXTURE_2D, target->textures[pipe_pass->getTargetSamplerTextureIndex(string_id("Position"))]->getId());
            }
            slot = prog_pbr_light->getDefaultSamplerSlot("Normal");
            if (slot != -1) {
                glActiveTexture(GL_TEXTURE0 + slot);
                glBindTexture(GL_TEXTURE_2D, target->textures[pipe_pass->getTargetSamplerTextureIndex(string_id("Normal"))]->getId());
            }
            slot = prog_pbr_light->getDefaultSamplerSlot("Metalness");
            if (slot != -1) {
                glActiveTexture(GL_TEXTURE0 + slot);
                glBindTexture(GL_TEXTURE_2D, target->textures[pipe_pass->getTargetSamplerTextureIndex(string_id("Metalness"))]->getId());
            }
            slot = prog_pbr_light->getDefaultSamplerSlot("Roughness");
            if (slot != -1) {
                glActiveTexture(GL_TEXTURE0 + slot);
                glBindTexture(GL_TEXTURE_2D, target->textures[pipe_pass->getTargetSamplerTextureIndex(string_id("Roughness"))]->getId());
            }
            slot = prog_pbr_light->getDefaultSamplerSlot("Emission");
            if (slot != -1) {
                glActiveTexture(GL_TEXTURE0 + slot);
                glBindTexture(GL_TEXTURE_2D, target->textures[pipe_pass->getTargetSamplerTextureIndex(string_id("Emission"))]->getId());
            }
            slot = prog_pbr_light->getDefaultSamplerSlot("ShadowCubeMap");
            if (slot != -1) {
                glActiveTexture(GL_TEXTURE0 + slot);
                GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP, cube_map_shadow->getId()));
            }

            glUseProgram(prog_pbr_light->getId());
            prog_pbr_light->setUniform3f("camPos", gfxm::inverse(view)[3]);
            glViewport(0, 0, target->getWidth(), target->getHeight());
            glScissor(0, 0, target->getWidth(), target->getHeight());
            prog_pbr_light->setUniform3f("lightPos", l.pos);
            prog_pbr_light->setUniform3f("lightColor", l.color);
            prog_pbr_light->setUniform1f("lightIntensity", l.intensity);
            gpuDrawFullscreenTriangle();
            // NOTE: If we do not unbind the program here, the driver will complain about the shadow sampler
            // texture format when it is changed, but the program bound is still this one
            glUseProgram(0);
        }
        
        glBindVertexArray(0);
        
        gpuFrameBufferUnbind();
        unbindTextures();
    }
    {
        static auto pipe_tech = s_pipeline->findTechnique("PBRCompose");
        static auto pipe_pass = pipe_tech->getPass(0);
        auto framebuffer_id = pipe_pass->getFrameBufferId();
        assert(framebuffer_id >= 0);
        gpuFrameBufferBind(target->framebuffers[framebuffer_id].get());

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0, 0, 0, 0, 0, 0, 0, 0, 0, };
        glDrawBuffers(GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS, draw_buffers);

        int slot = prog_pbr_compose->getDefaultSamplerSlot("Albedo");
        if (slot != -1) {
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(GL_TEXTURE_2D, target->textures[pipe_pass->getTargetSamplerTextureIndex(string_id("Albedo"))]->getId());
        }
        slot = prog_pbr_compose->getDefaultSamplerSlot("Lightness");
        if (slot != -1) {
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(GL_TEXTURE_2D, target->textures[pipe_pass->getTargetSamplerTextureIndex(string_id("Lightness"))]->getId());
        }
        slot = prog_pbr_compose->getDefaultSamplerSlot("Emission");
        if (slot != -1) {
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(GL_TEXTURE_2D, target->textures[pipe_pass->getTargetSamplerTextureIndex(string_id("Emission"))]->getId());
        }
        
        glUseProgram(prog_pbr_compose->getId());
        gpuDrawFullscreenTriangle();
        glBindVertexArray(0);

        gpuFrameBufferUnbind();
        unbindTextures();
    }

    {
        static auto pipe_tech = s_pipeline->findTechnique("Skybox");
        static auto pipe_pass = pipe_tech->getPass(0);
        auto framebuffer_id = pipe_pass->getFrameBufferId();
        assert(framebuffer_id >= 0);
        gpuFrameBufferBind(target->framebuffers[framebuffer_id].get());
        
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glEnable(GL_CULL_FACE);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LEQUAL);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0, 0, 0, 0, 0, 0, 0, 0, 0, };
        glDrawBuffers(GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS, draw_buffers);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, sky_cube_map->getId());

        glUseProgram(prog_skybox->getId());
        prog_skybox->setUniformMatrix4("matProjection", projection);
        prog_skybox->setUniformMatrix4("matView", view);

        gpuDrawCubeMapCube();

        glBindVertexArray(0);
        gpuFrameBufferUnbind();
        unbindTextures();
    }
    glDepthFunc(GL_LEQUAL);
    glBindVertexArray(gvao);
    {
        static auto pipe_tech = s_pipeline->findTechnique("Decals");
        drawTechnique(s_pipeline, target, bucket, pipe_tech);
        unbindTextures();
    }
    {
        static auto pipe_tech = s_pipeline->findTechnique("VFX");
        drawTechnique(s_pipeline, target, bucket, pipe_tech);
        unbindTextures();
    }
    glBindVertexArray(0);

    glDeleteVertexArrays(1, &gvao);

    //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void gpuDrawFullscreenTriangle() {
    glBindVertexArray(fullscreen_triangle_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}
void gpuDrawCubeMapCube() {
    glBindVertexArray(cube_map_cube_vao);
    glDrawArrays(GL_TRIANGLES, 0, 12 * 3);
}

void gpuDrawTextureToDefaultFrameBuffer(gpuTexture2d* texture, gpuTexture2d* depth) {
    int screen_w = 0, screen_h = 0;
    platformGetWindowSize(screen_w, screen_h);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0, 0, screen_w, screen_h);
    glScissor(0, 0, screen_w, screen_h);
    
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

void gpuDrawToDefaultFrameBuffer(gpuRenderTarget* target) {
    assert(target->textures.size());
    assert(target->default_output_texture >= 0 && target->default_output_texture < target->textures.size());
    gpuDrawTextureToDefaultFrameBuffer(target->textures[target->default_output_texture].get(), target->depth_texture);
}