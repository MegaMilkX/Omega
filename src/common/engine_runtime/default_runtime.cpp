#include "default_runtime.hpp"

#include <format>

#include "platform/platform.hpp"
#include "gpu/gpu.hpp"
#include "input/input.hpp"
#include "util/timer.hpp"
#include "player/player.hpp"
#include "audio/audio.hpp"

// TODO: Should not be here
#include "debug_draw/debug_draw.hpp"
// ========================

// TODO: questionable
#include "gui/gui.hpp"
// ==================

#include "resource_manager/resource_manager.hpp"


DefaultRuntime::DefaultRuntime(IGameInstance* game)
: game_instance(game) {
    registerComponent(&render_views);
}

void DefaultRuntime::onDisplayChanged(int w, int h) {
    gpuGetDefaultRenderTarget()->setSize(w, h);
    game_instance->onViewportResize(w, h);
}

void DefaultRuntime::run() {
    // Init
    {
        if (game_instance) {
            game_instance->init(this);
        }

        {
            stats_label = new GuiLabel("FPS: -");
            stats_label->setStyleClasses({"perf-stats"});
            //guiGetRootHost()->insert(stats_label);
            guiGetRoot()->pushBack(stats_label);

            guiGetStyleSheet()
                .add("perf-stats", {
                    gui::background_color(0x99000000),
                    gui::border_radius(0, 0, gui::em(1), 0),
                    gui::font_size(22),
                    gui::font_file("fonts/nimbusmono-bold.otf")
                })
                /*.add("tree-view", {
                    gui::background_color(0xFF000000),
                    gui::font_size(22),
                    gui::font_file("fonts/nimbusmono-bold.otf")
                })*/;
        }
    }

    timer timer_;
    timer timer_render;
    timer timer_gpu_wait;
    float dt = 1.f / 60.f;
    float total_time = .0f;
    while (platformIsRunning()) {
        timer_.start();

        // TODO: This or the callback? Choose one
        int screen_w, screen_h;
        platformGetWindowSize(screen_w, screen_h);
        {
            auto rt = gpuGetDefaultRenderTarget();
            if (rt) {
                rt->setSize(screen_w, screen_h);
            }
        }

        platformPollMessages();
        inputUpdate(dt);

        if (game_instance) {
            game_instance->update(dt);
        }

        {
            float leftover_perc = (stats.frame_time - stats.frame_time_no_vsync) / stats.frame_time * 100.f;
            stats_label->setCaption(
                std::format(
                    "Frame time (no vsync): \t{:.3f}ms\n"
                    "Leftover: \t\t\t\t{:.3f}ms\t{:.2f}%\n"
                    "CPU draw: \t\t\t\t{:.3f}ms\n"
                    "GPU wait: \t\t\t\t{:.3f}ms\n"
                    "Frame time: \t\t\t{:.3f}ms\n"
                    "Collision: \t\t\t\t{:.3f}ms\n"
                    "Audio: \t\t\t\t\t{:.3f}ms\n"
                    "FPS: \t\t\t\t\t{:.1f}",
                    stats.frame_time_no_vsync * 1000.f,
                    (stats.frame_time - stats.frame_time_no_vsync) * 1000.f, leftover_perc,
                    stats.cpu_draw_time * 1000.f,
                    stats.gpu_wait_time * 1000.f,
                    stats.frame_time * 1000.f,
                    stats.collision_time * 1000.f,
                    audioGetStats().buffer_update_time.load() * 1000.f,
                    stats.fps
                ).c_str()
            );
        }

        guiPollMessages();
        guiLayout();
        guiDraw();

        if (game_instance) {
            game_instance->draw(dt);
        }

        // Render viewports
        timer_render.start();
        for (int i = 0; i < render_views.size(); ++i) {
            EngineRenderView* rv = render_views[i];
            
            Camera* cam = rv->getCamera();
            if (!cam) {
                continue;
            }
            gpuRenderTarget* target = rv->getRenderTarget();
            gpuRenderBucket* bucket = rv->getRenderBucket();            

            if (scnRenderScene* scn = cam->getScene()) {
                scn->draw(bucket);
            }
            if (VisibilitySystem* vis_sys = cam->getVisibilitySystem()) {
                vis_sys->collectVisible(
                    VisibilityQuery(rv->getProjection(), cam->getViewTransform(), 0),
                    bucket
                );
            }

            DRAW_PARAMS params = {
                .view = rv->getViewTransform(),
                .projection = rv->getProjection(),
                .vp_rect_ratio = rv->getRect(),
                .viewport_x = (int)(target->getWidth() * rv->getRect().min.x),
                .viewport_y = (int)(target->getHeight() * rv->getRect().min.y),
                .viewport_width = (int)(target->getWidth() * (rv->getRect().max.x - rv->getRect().min.x)),
                .viewport_height = (int)(target->getHeight() * (rv->getRect().max.y - rv->getRect().min.y)),
                .time = total_time
            };
            
            gpuDraw(bucket, target, params);
            bucket->clear();
        }

        // Blit to screen
        {
            auto rt = gpuGetDefaultRenderTarget();
            if (rt) {
                gpuDrawToDefaultFrameBuffer(rt, gfxm::rect(0, 0, 1, 1));
            }
        }

        // TODO: Remove
        LocalPlayer* local = dynamic_cast<LocalPlayer*>(playerGetPrimary());
        if (!local) {
            return;
        }
        assert(local->getViewport());
        dbgDrawDraw(
            local->getViewport()->getProjection(),
            local->getViewport()->getViewTransform(),
            0, 0, local->getViewport()->getRenderTarget()->getWidth(), local->getViewport()->getRenderTarget()->getHeight()
        );
        
        dbgDrawClearBuffers();
        // ====

        guiRender(false);
        stats.cpu_draw_time = timer_render.stop();

        ResourceManager::get()->collectGarbage();

        stats.gpu_wait_time = FLT_MAX;
        /*
        timer_gpu_wait.start();
        glFinish();
        stats.gpu_wait_time = timer_gpu_wait.stop();
        */
        stats.frame_time_no_vsync = timer_.stop();
        platformSwapBuffers();

        stats.frame_time = timer_.stop();
        total_time += stats.frame_time;
        // Don't let the frame time be too large
        dt = gfxm::_min(1.f / 15.f, stats.frame_time);

        stats.fps = 1.0f / stats.frame_time;
    }
}

