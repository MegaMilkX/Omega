#pragma once

#include <vector>
#include "shape.hpp"


class phyHeightfieldShape : public phyShape {
    std::vector<float> samples;
    int sample_count_x = 0;
    int sample_count_z = 0;
    float width = 1.f;
    float depth = 1.f;
public:
    phyHeightfieldShape()
        : phyShape(PHY_SHAPE_TYPE::HEIGHTFIELD) {}

    void init(float* data, int sample_count_x, int sample_count_z, float width, float depth);

    float getWidth() const { return width; }
    float getDepth() const { return depth; }
    float getCellWidth() const { return width / float(sample_count_x - 1); }
    float getCellDepth() const { return depth / float(sample_count_z - 1); }
    int getSampleCountX() const { return sample_count_x; }
    int getSampleCountZ() const { return sample_count_z; }
    const float* getData() const { return samples.data(); }

    gfxm::aabb calcWorldAabb(const gfxm::mat4& transform) const override;
};