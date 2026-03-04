#include "gpu_pass.hpp"


void gpuPass::sortCommands(gpuRenderCmd* commands, size_t count, const DRAW_PARAMS& params) {
    switch (sort_mode) {
    case GPU_SORT_MODE::NONE:
        break;
    case GPU_SORT_MODE::STATE_CHANGE: {
        std::sort(commands, commands + count, [](const gpuRenderCmd& a, const gpuRenderCmd& b)->bool {
            if (a.program_id == b.program_id) {
                if (a.state_id == b.state_id) {
                    return a.sampler_set_id < b.sampler_set_id;
                }
                return a.state_id < b.state_id;
            }
            return a.program_id < b.program_id;
        });
        break;
    }
    case GPU_SORT_MODE::FRONT_TO_BACK: {
        const gfxm::mat4 cam_transform = gfxm::inverse(params.view);
        const gfxm::vec3 cam_forward = -cam_transform[2];
        const gfxm::vec3 cam_pos = cam_transform[3];
        for (int i = 0; i < count; ++i) {
            auto& cmd = commands[i];
            cmd.depth = gfxm::dot(cam_forward, cmd.renderable->getSortHint() - cam_pos);
        }
        std::sort(commands, commands + count, [](const gpuRenderCmd& a, const gpuRenderCmd& b)->bool {
            return a.depth < b.depth;
        });
        break;
    }
    case GPU_SORT_MODE::BACK_TO_FRONT: {
        const gfxm::mat4 cam_transform = gfxm::inverse(params.view);
        const gfxm::vec3 cam_forward = -cam_transform[2];
        const gfxm::vec3 cam_pos = cam_transform[3];
        for (int i = 0; i < count; ++i) {
            auto& cmd = commands[i];
            cmd.depth = gfxm::dot(cam_forward, cmd.renderable->getSortHint() - cam_pos);
        }
        std::sort(commands, commands + count, [](const gpuRenderCmd& a, const gpuRenderCmd& b)->bool {
            return a.depth > b.depth;
        });
        break;
    }
    default:
        assert(false);
    }
}