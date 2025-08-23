#pragma once

#include "scn_render_object.hpp"



class scnRenderScene;
class scnLightOmni : public scnRenderObject {
public:
    gfxm::vec3  position;
    gfxm::vec3  color;
    float       radius;
    float       intensity;
    bool        enable_shadows;

    void onAdded() override {}
    void onRemoved() override {}
};