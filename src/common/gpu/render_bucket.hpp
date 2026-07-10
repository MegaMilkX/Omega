#pragma once

#include <set>
#include "gpu/gpu_types.hpp"
#include "gpu/types.hpp"
#include "gpu/gpu_renderable.hpp"
#include "gpu/render_cmd.hpp"


class gpuPipeline;
class gpuRenderBucket {
    gpuPipeline* pipeline = nullptr;
public:
    std::vector<std::vector<gpuRenderCmd>> commands_per_pass;
    std::vector<gpuRenderCmdLightOmni> lights_omni;
    std::vector<gpuRenderCmdLightDirect> lights_direct;
    std::set<int> layers;

    gpuRenderBucket() {}
    gpuRenderBucket(gpuPipeline* pipeline, int queue_reserve /*TODO: unused, should remove*/);
    void clear();
    void addLightOmni(const gfxm::vec3& pos, const gfxm::vec3& color, float intensity, bool shadow);
    void addLightDirect(const gfxm::vec3& dir, const gfxm::vec3& color, float intensity);
    void add(gpuRenderable* renderable);
    void sort(const DRAW_PARAMS& params);

    const std::set<int>& getLayers() const {
        return layers;
    }

    const std::vector<gpuRenderCmd>& getPassCommands(pipe_pass_id_t i) {
        return commands_per_pass[i];
    }
};