#pragma once


#include "gpu/gpu_pipeline.hpp"
#include "gpu/render/uniform.hpp"
#include "platform/platform.hpp"
#include "gpu/pass/gpu_pass.hpp"
#include "gpu/pass/gpu_deferred_geometry_pass.hpp"
#include "gpu/pass/gpu_translucent_pass.hpp"
#include "gpu/pass/wireframe_pass.hpp"
#include "gpu/pass/environment_ibl_pass.hpp"
#include "gpu/pass/environment_ssr_pass.hpp"
#include "gpu/pass/gpu_deferred_light_pass.hpp"
#include "gpu/pass/gpu_deferred_compose_pass.hpp"
#include "gpu/pass/gpu_skybox_pass.hpp"
#include "gpu/pass/blur_pass.hpp"
#include "gpu/pass/test_posteffect_pass.hpp"
#include "gpu/pass/fog_pass.hpp"
#include "gpu/pass/velocity_map_pass.hpp"
#include "gpu/pass//ssao_pass.hpp"
#include "gpu/pass/blit_pass.hpp"
#include "gpu/pass/clear_pass.hpp"

#include "gpu/param_block/common_block.hpp"


#define GPU_CH_ALBEDO               "Albedo"
#define GPU_CH_POSITION             "Position"
#define GPU_CH_NORMAL               "Normal"
#define GPU_CH_METALNESS            "Metalness"
#define GPU_CH_ROUGHNESS            "Roughness"
#define GPU_CH_AMBIENT_OCCLUSION    "AmbientOcclusion"
#define GPU_CH_LIGHTNESS            "Lightness"
#define GPU_CH_OBJECT_OUTLINE       "ObjectOutline"
#define GPU_CH_DOF_MASK             "DOFMask"
#define GPU_CH_FINAL                "Final"
#define GPU_CH_DEPTH                "Depth"
#define GPU_CH_DEPTH_OVERLAY        "DepthOverlay"


class gpuPipelineDefault : public gpuPipeline {
    gpuCommonBlock* common_block = nullptr;

    gpuUniformBuffer* ubufShadowmapCamera3d = 0;

    int loc_shadowmap_projection = -1;
    int loc_shadowmap_view = -1;

public:
    gpuPipelineDefault();
    ~gpuPipelineDefault();

    void init() override;
    void resolveRenderableRole(GPU_Role t, GPU_INTERMEDIATE_RENDERABLE_CONTEXT& ctx, const gpuMaterial* renderable) override;
    void resolveRenderableEffect(GPU_Effect t, GPU_INTERMEDIATE_RENDERABLE_CONTEXT& ctx, const gpuMaterial* renderable) override;

    void setShadowmapCamera(const gfxm::mat4& projection, const gfxm::mat4& view);
    void setCamera3d(const gfxm::mat4& projection, const gfxm::mat4& view);
    void setCamera3dPrev(const gfxm::mat4& projection, const gfxm::mat4& view);
    void setViewportSize(float width, float height);
    void setViewportRectRatio(const gfxm::vec4& rc);
    void setTime(float t);
    void setGamma(float gamma);
    void setExposure(float exposure);
};