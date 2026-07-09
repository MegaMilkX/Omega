#include "render_sequence.hpp"
#include <set>
#include "gpu/gpu.hpp"
#include "gpu/skinning/skinning_compute.hpp"


void gpuRenderSequence::init(std::initializer_list<std::string> pass_list) {
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
        pass_inst->framebuffer_id = i;
    }

    channels.reserve(used_channels.size());
    for (auto& name : used_channels) {
        ChannelDesc& seq_ch_desc = channels.emplace_back();
        auto pipe_ch_idx = gpuGetPipeline()->getChannelIndex(name.c_str());
        seq_ch_desc.name = name;
        seq_ch_desc.pipe_channel_index = pipe_ch_idx;
    }

    for (int i = 0; i < passes.size(); ++i) {
        gpuPassInstance* pass_inst = &passes[i];
        pass_inst->rt_chan_to_pass.resize(gpuGetPipeline()->channelCount());
        for (int j = 0; j < pass_inst->channels.size(); ++j) {
            gpuPassInstance::ChannelDesc* inst_ch_desc = &pass_inst->channels[j];
            inst_ch_desc->render_target_channel_idx = getChannelIdx(inst_ch_desc->name);
            pass_inst->rt_chan_to_pass[inst_ch_desc->render_target_channel_idx] = j;
        }

        gpuPass* pass = pass_inst->pass;
        if (pass->hasDepthTarget()) {
            int depth_idx = getChannelIdx(pass->getDepthTargetGlobalName());
            assert(depth_idx >= 0);
            pass_inst->depth_layer_name = pass->getDepthTargetGlobalName();
            pass_inst->depth_target_idx = depth_idx;
        }

        
    }

    gpuGetPipeline()->updateRenderSequence(this);
}

int gpuRenderSequence::getChannelIdx(const std::string& name) {
    // Currently the pipeline channels and render target channels are 1 to 1
    return gpuGetPipeline()->getChannelIndex(name.c_str());
    /*
    for (int i = 0; i < channels.size(); ++i) {
        auto ch = gpuGetPipeline()->getChannel(channels[i].pipe_channel_index);
        if (ch->name == name) {
            return i;
        }
    }
    return -1;*/
}

void gpuRenderSequence::run(gpuRenderBucket* bucket, gpuRenderTargetMap* target_map, const DRAW_PARAMS& params) {
    target_map->updateTargetLwts();

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
        pass_inst->pass->onDraw(pass_inst, target_map, bucket, pass_inst->pass->getId(), params);
    }
}

