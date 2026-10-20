#include "render_sequence.hpp"
#include <set>
#include "gpu/gpu.hpp"
#include "gpu/skinning/skinning_compute.hpp"


void gpuRenderSequence::_updateFramebufferSizes() {
    for (int i = 0; i < passes.size(); ++i) {
        auto& pass = passes[i];
        for (int j = 0; j < pass.framebuffers.size(); ++j) {
            auto& fb = pass.framebuffers[j];
            fb->updateSize();
        }
    }
}


void gpuRenderSequence::init(const std::string& dbg_name, std::initializer_list<std::string> pass_list) {
    this->dbg_name = dbg_name;

    passes.clear();
    auto pipeline = gpuGetPipeline();
    for (auto pass_name : pass_list) {
        pipe_pass_id_t id = pipeline->getPassId(pass_name.c_str());
        if (id < 0) {
            continue;
        }
        gpuPass* pass = pipeline->getPass(id);
        if (pass->hasAnyFlags(PASS_FLAG_DISABLED)) {
            continue;
        }
        passes.push_back(gpuPassInstance{ pass });
    }

    /*
    // Enforces the original pipeline order. Probably a bad idea
    std::sort(passes.begin(), passes.end(), [](const gpuPassInstance& a, const gpuPassInstance& b)->bool {
        return a.pass->getId() < b.pass->getId();
    });*/

    std::set<std::string> used_channels;
    for (int i = 0; i < passes.size(); ++i) {
        gpuPassInstance* pass_inst = &passes[i];
        gpuPass* pass = pass_inst->pass;
        pass_inst->channels.resize(pass->channelCount());
        for (int j = 0; j < pass->channelCount(); ++j) {
            auto ch_desc = pass->getChannelDesc(j);
            pass_inst->channels[j].name = ch_desc->pipeline_channel_name;
            used_channels.insert(ch_desc->pipeline_channel_name);
        }
        if (pass->hasDepthTarget()) {
            used_channels.insert(pass->getDepthTargetGlobalName());
        }
    }

    channels.reserve(used_channels.size());
    for (auto& name : used_channels) {
        ChannelDesc& seq_ch_desc = channels.emplace_back();
        auto pipe_ch_idx = gpuGetPipeline()->getChannelIndex(name.c_str());
        seq_ch_desc.name = name;
        seq_ch_desc.pipe_channel_index = pipe_ch_idx;
    }
}


void gpuRenderSequence::run(gpuRenderBucket* bucket, gpuRenderTarget* target, const DRAW_PARAMS& params, int fb_idx) {
    gpuDbgScopedGroup dbg_grp(dbg_name);

    _updateFramebufferSizes();
    {
        // Update lwt markers for render target so that we know which buffers have been actually written to last
        for (int i = 0; i < lwt_array.size(); ++i) {
            target->channels[i]->lwt = lwt_array[i];
        }
    }

    const gfxm::mat4& view = params.view;
    const gfxm::mat4& projection = params.projection;
    const int vp_x = params.viewport_x;
    const int vp_y = params.viewport_y;
    const int vp_width = params.viewport_width;
    const int vp_height = params.viewport_height;

    auto pipeline = gpuGetPipeline();
    pipeline->setCamera3d(projection, view);
    pipeline->setCamera3dPrev(projection, params.view_prev);
    pipeline->setViewportSize(vp_width, vp_height);
    pipeline->setViewportRectRatio(gfxm::vec4(params.vp_rect_ratio.min.x, params.vp_rect_ratio.min.y, params.vp_rect_ratio.max.x, params.vp_rect_ratio.max.y));
    pipeline->setTime(params.time);
    gpuGetPipeline()->updateParamBlocks();

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

    gpuGetPipeline()->bindUniformBuffers();
    gpuGetPipeline()->bindParamBlocks();
    for (int i = 0; i < passes.size(); ++i) {
        gpuPassInstance* pass_inst = &passes[i];
        if (pass_inst->pass->hasAnyFlags(PASS_FLAG_DISABLED | PASS_FLAG_NO_DRAW)) {
            continue;
        }
        if (pass_inst->framebuffers.empty()) {
            continue;
        }
        if (fb_idx == -1) {
            for (int j = 0; j < pass_inst->framebuffers.size(); ++j) {
                pass_inst->pass->onDraw(pass_inst, target, bucket, pass_inst->pass->getId(), j, params);
            }
        } else {
            pass_inst->pass->onDraw(pass_inst, target, bucket, pass_inst->pass->getId(), fb_idx, params);
        }
    }
}

