#pragma once

#include <memory>
#include "mode.hpp"
#include "../brush.hpp"
#include "../brush_tip.hpp"
#include "gui/elements/viewport/gui_viewport.hpp"
#include "gpu/renderable/decal.hpp"
#include "gpu/material/vfx_material.hpp"
#include "gpu/texture/texture2d.hpp"
#include "resource_manager/resource_ref.hpp"


class TerrainSculptMode : public TerrainEditMode {
    // Cursor preview
    std::unique_ptr<gpuDecalRenderable> renderable2;
    ResourceRef<VFXMaterial> material2;
    ResourceRef<gpuTexture2d> brush_tip_texture;
    bool brush_hit = false;
    gfxm::vec3 brush_pos;
    float brush_radius = 10.f;
    float brush_scale = .25f; // reference for resizing
    constexpr static float MIN_BRUSH_RADIUS = .5f;
    constexpr static float MAX_BRUSH_RADIUS = 200.f;

    // Brush
    std::vector<float> brush_scratch;
    std::vector<uint8_t> brush_tip_scratch;
public:
    std::unique_ptr<TerrainBrush> current_brush;
    std::unique_ptr<TerrainBrush> secondary_brush;
    std::unique_ptr<BrushTip> brush_tip;

    TerrainSculptMode(TerrainSceneSpace* space)
    : TerrainEditMode("Sculpt Mode", space) {
        {
            material2 = createResource<VFXMaterial>("");
            material2->compile();
            renderable2.reset(new gpuDecalRenderable(material2.get(), 0, "MyBrush"));
            renderable2->setExtents(gfxm::vec3(brush_radius * 2.f, 100.f, brush_radius * 2.f));
            renderable2->setTransform(gfxm::mat4(1.f));
        }
        current_brush.reset(new DrawTerrainBrush);
        secondary_brush.reset(new SmoothTerrainBrush);
        brush_tip.reset(new RadialBrushTip);
        
        subscribe([this](const GuiEvt_MouseMove& e) {
            e.consume = false;
            guiScheduleTick(this, 0, GUI_TICK_CUSTOM);
        });
        subscribe([this](const GuiEvt_KeyDown& e) {
            switch (e.vkey) {
            case 90: // Z key
                if(brush_hit) {
                    viewport->setCameraPivot(brush_pos);
                }
                return;
            }
            e.consume = false;
            e.invoke_next();
        });
        subscribe([this](const GuiEvt_Pull& e) {
            if (e.btn != GUI_MOUSE_LEFT) {
                return;
            }
            if (!brush_hit) {
                return;
            }
            applyBrush(brush_pos.x, brush_pos.z);
        });
        subscribe([this](const GuiEvt_Scroll& e) {
            if (!guiIsModifierKeyPressed(GUI_KEY_CONTROL)) {
                e.consume = false;
                return;
            }
            float modifier = brush_radius;
            int offset = 1;
            if (e.value < .0f) {
                offset = -1;
                modifier = MAX_BRUSH_RADIUS + .5f - brush_radius;
            }
            brush_scale = gfxm::_max(.0f, gfxm::_min(1.f, brush_scale + e.value * .00025f));
            brush_radius = gfxm::lerp(MIN_BRUSH_RADIUS, MAX_BRUSH_RADIUS, brush_scale * brush_scale);
            renderable2->setExtents(gfxm::vec3(brush_radius * 2.f, 100.f, brush_radius * 2.f));
        });
        
        // Brush tip preview texture
        {
            const int w = 128;
            const int h = 128;
            const int r = 2;
            std::vector<uint8_t> mask(w * h);
            brush_tip->rasterize(mask.data(), w, h, gfxm::vec2(0), 0);

            std::vector<uint8_t> outline(w * h, 0);
            auto is_inside = [&](int x, int y) {
                return x >= 0 && y >= 0 && x < w && y < h && mask[x + y * w] > 0;
            };
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    if (!is_inside(x, y)) {
                        continue;
                    }
                    bool edge = false;
                    for (int dy = -r; dy <= r; ++dy) {
                        for (int dx = -r; dx <= r; ++dx) {
                            if (dx * dx + dy * dy <= r * r
                                && !is_inside(x + dx, y + dy)
                            ) {
                                edge = true;
                                break;
                            }
                        }
                    }
                    if (edge) {
                        outline[(w - 1 - x) + y * w] = 255;
                    }
                }
            }
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    int a = outline[x + y * w];
                    int b = mask[(w - 1 - x) + y * w];
                    outline[x + y * w] = gfxm::_min(int(a + b), 255);
                }
            }

            std::vector<uint8_t> rgba(w * h * 4);
            for (int i = 0; i < w * h; ++i) {
                rgba[i * 4] = outline[i];
                rgba[i * 4 + 1] = outline[i];
                rgba[i * 4 + 2] = outline[i];
                rgba[i * 4 + 3] = outline[i];
            }
            brush_tip_texture = createResource<gpuTexture2d>("");
            brush_tip_texture->setData(rgba.data(), w, h, 4, IMAGE_CHANNEL_UNSIGNED_BYTE);
            brush_tip_texture->setFilter(GPU_TEXTURE_FILTER_NEAREST);
            material2->setBaseTexture(brush_tip_texture);
            material2->setBackfaceCulling(true);            
            material2->setDepthTest_(false);
            material2->setDepthWrite(false);
            material2->setBlendingMode(GPU_BLEND_MODE::ADD);
        }
    }

    void onTick(float dt, GUI_TICK_ID id) override;
    void queryGeometry(const GeometryQuery& q) override {
        if(brush_hit) {
            q.bucket->add(renderable2.get());
        }
    }
    
    void applyBrush(float ptx, float ptz);
};

