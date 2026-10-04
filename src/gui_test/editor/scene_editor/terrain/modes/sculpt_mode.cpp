#include "sculpt_mode.hpp"

#include "../terrain_space.hpp"


void TerrainSculptMode::onTick(float dt, GUI_TICK_ID id) {
    if (id != GUI_TICK_CUSTOM) {
        return;
    }

    brush_hit = getSpace()->hitTest(brush_pos);

    gfxm::mat4 t = gfxm::translate(gfxm::mat4(1.f), brush_pos);
    renderable2->setTransform(t);
}

void TerrainSculptMode::applyBrush(float ptx, float ptz) {
    float MULTIPLIER = 1.f;
    if (guiIsModifierKeyPressed(GUI_KEY_CONTROL)) {
        MULTIPLIER = -1.f;
    }

    auto params = getSpace()->getTerrainParams();
    const int CELL_SEGMENTS_X   = params.cell_segments_x;
    const int CELL_SEGMENTS_Z   = params.cell_segments_z;
    const float CELL_WIDTH        = params.cell_width;
    const float CELL_DEPTH        = params.cell_depth;

    const float BRUSH_STRENGTH = .2f;

    const float QUAD_WIDTH = CELL_WIDTH / CELL_SEGMENTS_X;
    const float QUAD_DEPTH = CELL_DEPTH / CELL_SEGMENTS_Z;

    gfxm::ivec2 icmin(
        floorf((ptx - brush_radius) / CELL_WIDTH),
        floorf((ptz - brush_radius) / CELL_DEPTH)
    );
    gfxm::ivec2 icmax(
        floorf((ptx + brush_radius) / CELL_WIDTH),
        floorf((ptz + brush_radius) / CELL_DEPTH)
    );
        
    // Paint
    gfxm::vec2 fmin(ptx - brush_radius, ptz - brush_radius);
    gfxm::vec2 fmax(ptx + brush_radius, ptz + brush_radius);
    gfxm::vec2 qmin(ceilf(fmin.x / QUAD_WIDTH), ceilf(fmin.y / QUAD_DEPTH));
    gfxm::vec2 qmax(floorf(fmax.x / QUAD_WIDTH), floorf(fmax.y / QUAD_DEPTH));
    const int region_width = qmax.x + 1 - qmin.x;
    const int region_depth = qmax.y + 1 - qmin.y;
    //LOG_DBG("REGION: " << region_width << ", " << region_depth);

    brush_tip_scratch.resize(region_width * region_depth);
    brush_tip->rasterize(
        brush_tip_scratch.data(), region_width, region_depth,
        gfxm::fract(gfxm::vec2(ptx / QUAD_WIDTH, ptz / QUAD_DEPTH)), .0f
    );
        
    brush_scratch.resize(region_width * region_depth);
    // Copy existing heights into the brush region
    for (int icz = icmin.y; icz <= icmax.y; ++icz) {
        for (int icx = icmin.x; icx <= icmax.x; ++icx) {
            TerrainCell* cell = getSpace()->getCell(icx, icz);
            if (!cell) {
                continue;
            }
            float* points = cell->points.data();

            // Region bounds in terms of cell-local points
            const int rminx = qmin.x - CELL_SEGMENTS_X * icx;
            const int rminz = qmin.y - CELL_SEGMENTS_Z * icz;
            const int rmaxx = qmax.x - CELL_SEGMENTS_X * icx;
            const int rmaxz = qmax.y - CELL_SEGMENTS_Z * icz;
            // How much the region overflows on each side
            const int rminx_of = gfxm::_max(0, -rminx);
            const int rminz_of = gfxm::_max(0, -rminz);
            const int rmaxx_of = gfxm::_max(0, rmaxx + 1 - CELL_SEGMENTS_X);
            const int rmaxz_of = gfxm::_max(0, rmaxz + 1 - CELL_SEGMENTS_Z);

            const int rxl = region_width - rminx_of - rmaxx_of;
            const int rzl = region_depth - rminz_of - rmaxz_of;

            // TODO: Fix negative rxl/rzl properly
            if (rxl < 0 || rzl < 0) {
                continue;
            }

            for (int pz = 0; pz < rzl; ++pz) {
                const int rpz = rminz_of + pz;
                const int cpz = rminz + rminz_of + pz;
                const int rpx = rminx_of;
                const int cpx = rminx + rminx_of;
                memcpy(
                    &brush_scratch[rpx + rpz * region_width],
                    &points[cpx + cpz * CELL_SEGMENTS_X],
                    rxl * sizeof(brush_scratch[0])
                );/*
                for (int px = 0; px < rxl; ++px) {
                    const int rpx = rminx_of + px;
                    const int cpx = rminx + rminx_of + px;
                    brush_scratch[rpx + rpz * region_width]
                        = points[cpx + cpz * CELL_SEGMENTS_X];
                }*/
            }
        }
    }

    TerrainBrushContext ctx {
        .region = brush_scratch.data(),
        .mask = brush_tip_scratch.data(),
        .width = region_width,
        .height = region_depth,
        .radius = brush_radius,
        .strength = BRUSH_STRENGTH * MULTIPLIER,
    };
    if (guiIsModifierKeyPressed(GUI_KEY_SHIFT)) {
        secondary_brush->apply(ctx);
    } else {
        current_brush->apply(ctx);
    }

    // TODO: Already know the exact cells, can avoid lookup
    for (int icz = icmin.y; icz <= icmax.y; ++icz) {
        for (int icx = icmin.x; icx <= icmax.x; ++icx) {
            TerrainCell* cell = getSpace()->getCell(icx, icz);
            if (!cell) {
                continue;
            }
            float* points = cell->points.data();

            // Region bounds in terms of cell-local points
            const int rminx = qmin.x - CELL_SEGMENTS_X * icx;
            const int rminz = qmin.y - CELL_SEGMENTS_Z * icz;
            const int rmaxx = qmax.x - CELL_SEGMENTS_X * icx;
            const int rmaxz = qmax.y - CELL_SEGMENTS_Z * icz;
            //LOG_DBG("LOCAL: [" << rminx << ", " << rminz << ", " << rmaxx << ", " << rmaxz << "]");
            // How much the region overflows on each side
            const int rminx_of = gfxm::_max(0, -rminx);
            const int rminz_of = gfxm::_max(0, -rminz);
            const int rmaxx_of = gfxm::_max(0, rmaxx + 1 - CELL_SEGMENTS_X);
            const int rmaxz_of = gfxm::_max(0, rmaxz + 1 - CELL_SEGMENTS_Z);
            //LOG_DBG("OVERFLOW: [" << rminx_of << ", " << rminz_of << ", " << rmaxx_of << ", " << rmaxz_of << "]");

            const int rxl = region_width - rminx_of - rmaxx_of;
            const int rzl = region_depth - rminz_of - rmaxz_of;
            for (int pz = 0; pz < rzl; ++pz) {
                const int rpz = rminz_of + pz;
                const int cpz = rminz + rminz_of + pz;
                for (int px = 0; px < rxl; ++px) {
                    const int rpx = rminx_of + px;
                    const int cpx = rminx + rminx_of + px;
                    points[cpx + cpz * CELL_SEGMENTS_X]
                        = brush_scratch[rpx + rpz * region_width];
                }
            }

            getSpace()->markCellDirty(icx, icz);
        }
    }

    guiScheduleTick(this, 0, GUI_TICK_CUSTOM);
}