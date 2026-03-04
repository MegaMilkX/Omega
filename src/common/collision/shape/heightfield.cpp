#include "heightfield.hpp"



void phyHeightfieldShape::init(float* data, int sample_count_x, int sample_count_z, float width, float depth) {
    samples.resize(sample_count_x * sample_count_z);
    memcpy(&samples[0], data, sizeof(samples[0]) * sample_count_x * sample_count_z);
    this->sample_count_x = sample_count_x;
    this->sample_count_z = sample_count_z;
    this->width = width;
    this->depth = depth;
}

gfxm::aabb phyHeightfieldShape::calcWorldAabb(const gfxm::mat4& transform) const {
    gfxm::aabb box;
    float cell_width = width / float(sample_count_x - 1);
    float cell_depth = depth / float(sample_count_z - 1);

    float y = samples[0 + 0 * sample_count_x];
    gfxm::vec4 pt4(.0f * cell_width, y, .0f * cell_depth, 1.f);
    gfxm::vec3 pt3 = transform * pt4;
    box.from = pt3;
    box.to = pt3;

    for (int z = 0; z < sample_count_z; ++z) {
        for (int x = 0; x < sample_count_x; ++x) {
            float y = samples[x + z * sample_count_x];
            gfxm::vec4 pt4(x * cell_width, y, z * cell_depth, 1.f);
            gfxm::vec3 pt3 = transform * pt4;
            gfxm::expand_aabb(box, pt3);
        }
    }

    return box;
}