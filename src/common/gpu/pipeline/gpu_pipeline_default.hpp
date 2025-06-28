#pragma once


#include "gpu/gpu_pipeline.hpp"
#include "gpu/render/uniform.hpp"
#include "platform/platform.hpp"
#include "gpu/pass/gpu_pass.hpp"
#include "gpu/pass/gpu_deferred_geometry_pass.hpp"
#include "gpu/pass/wireframe_pass.hpp"
#include "gpu/pass/environment_ibl_pass.hpp"
#include "gpu/pass/gpu_deferred_light_pass.hpp"
#include "gpu/pass/gpu_deferred_compose_pass.hpp"
#include "gpu/pass/gpu_skybox_pass.hpp"
#include "gpu/pass/blur_pass.hpp"
#include "gpu/pass/test_posteffect_pass.hpp"
#include "gpu/pass/fog_pass.hpp"
#include "gpu/pass//ssao_pass.hpp"
#include "gpu/pass/blit_pass.hpp"
#include "gpu/pass/clear_pass.hpp"


class gpuPipelineDefault : public gpuPipeline {
    gpuUniformBuffer* ubufCommon = 0;
    gpuUniformBuffer* ubufShadowmapCamera3d = 0;
    gpuUniformBuffer* ubufTime = 0;
    gpuUniformBuffer* ubufModel = 0;
    gpuUniformBuffer* ubufDecal = 0;
    int loc_projection = -1;
    int loc_view = -1;
    int loc_camera_pos = -1;
    int loc_screenSize = -1;
    int loc_zNear = -1;
    int loc_zFar = -1;
    int loc_vp_rect_ratio = -1;
    int loc_time = -1;
    int loc_gamma = -1;
    int loc_exposure = -1;

    int loc_shadowmap_projection = -1;
    int loc_shadowmap_view = -1;
    int loc_model = -1;
    int loc_boxSize = -1;
    int loc_color = -1;
public:
    gpuPipelineDefault();
    ~gpuPipelineDefault();

    void init() override;

    void setShadowmapCamera(const gfxm::mat4& projection, const gfxm::mat4& view);
    void setCamera3d(const gfxm::mat4& projection, const gfxm::mat4& view);
    void setViewportSize(float width, float height);
    void setViewportRectRatio(const gfxm::vec4& rc);
    void setTime(float t);
    void setGamma(float gamma);
    void setExposure(float exposure);
    void setModelTransform(const gfxm::mat4& model);
    void setDecalData(const gfxm::vec3& boxSize, const gfxm::vec4& color, const gfxm::vec2& vp_sz);
};