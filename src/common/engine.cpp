#include "engine.hpp"

#include "platform/platform.hpp"
#include "init_handler/init_handler.hpp"
#include "reflect.hpp"

#include "animation/animation.hpp"
#include "audio/res_cache_audio_clip.hpp"
#include "audio/audio.hpp"

#include "input/input.hpp"

#include "gui/gui.hpp"

#include "util/timer.hpp"

static GameBase* engine_game_instance = 0;
static InitHandlerRAII* engine_init_handler = 0;
static std::vector<Viewport*> viewports;
static ENGINE_STATS engine_stats;


static void onWindowResize(int width, int height) {
    gpuGetDefaultRenderTarget()->setSize(width, height);
    engine_game_instance->onViewportResize(width, height);
}

int engineGameInit() {

    engine_init_handler = new InitHandlerRAII;

    // NOTE: May seem stupid, but:
    // - Allows to automatically cleanup in correct order, just by destroying the object
    // - init and cleanup are in the same place
    // - you can clearly see when no cleanup is intentional
    engine_init_handler
        ->add("Reflection", 
            []()->bool { reflectInit(); return true; },
            0
        )
        .add("Platform",
            []()->bool { platformInit(); return true; },
            &platformCleanup
        )
        .add("ResourceCache", &resInit, &resCleanup)
        .add("Typeface", &typefaceInit, &typefaceCleanup)
        .add("Animation", &animInit, &animCleanup)
        .add("Rendering",
            []()->bool {
                gpuInit();
                return true;
            },
            []() {
                gpuCleanup();
            }
        )
        .add("Audio",
            []()->bool {
                resAddCache<AudioClip>(new resCacheAudioClip);
                audioInit();
                return true;
            },
            []() {
                audioCleanup();
            }
        )
        .add("GUI",
            []()->bool {
                std::shared_ptr<Font> fnt = fontGet("fonts/ProggyClean.ttf", 16, 72);
                guiInit(fnt);
                return true;
            },
            []() {
                guiCleanup();
            }
        );
    if (!engine_init_handler->init()) {
        return false;
    }

    platformSetWindowResizeCallback(&onWindowResize);

    // TODO: Create common render target

    return true;
}

void engineGameCleanup() {
    delete engine_init_handler;
    engine_init_handler = 0;
}

void engineGameRun(ENGINE_INIT_DATA& data) {
    // Init
    {
        if (data.primary_viewport) {
            if (data.primary_viewport->getRenderTarget() == 0) {
                data.primary_viewport->setRenderTarget(gpuGetDefaultRenderTarget());
            }
            viewports.push_back(data.primary_viewport);
        }
        if (data.primary_player) {
            playerAdd(data.primary_player);
            playerSetPrimary(data.primary_player);
        }
        if (data.game) {
            engine_game_instance = data.game;
            engine_game_instance->init();
            for (int i = 0; i < playerCount(); ++i) {
                auto p = playerGet(i);
                engine_game_instance->onPlayerJoined(p);
            }
        }
    }

    // Run
    timer timer_;
    timer timer_render;
    float dt = 1.0f / 60.0f;
    float total_time = .0f;
    while (platformIsRunning()) {
        timer_.start();
        
        // Update viewport sizes
        int screen_w, screen_h;
        platformGetWindowSize(screen_w, screen_h);
        {
            auto rt = gpuGetDefaultRenderTarget();
            if (rt) {
                rt->setSize(screen_w, screen_h);
            }
        }/*
        for (int i = 0; i < viewports.size(); ++i) {
            if (viewports[i]->isOffscreen()) {
                continue;
            }
            viewports[i]->updateAvailableSize(screen_w, screen_h);
        }*/

        platformPollMessages();

        inputUpdate(dt);
        
        if (engine_game_instance) {
            engine_game_instance->update(dt);
        }
        /*
        int world_count = gameWorldGetUpdateListCount();
        RuntimeWorld** world_list = gameWorldGetUpdateList();
        for (int i = 0; i < world_count; ++i) {
            world_list[i]->update(dt);
        }*/

        guiPollMessages();
        guiLayout();
        guiDraw();

        if (engine_game_instance) {
            engine_game_instance->draw(dt);
        }

        timer_render.start();
        // Render viewports
        for (int i = 0; i < viewports.size(); ++i) {
            Viewport* vp = viewports[i];
            RuntimeWorld* world = vp->getWorld();
            gpuRenderTarget* target = vp->getRenderTarget();
            gpuRenderBucket* bucket = vp->getRenderBucket();
            if (!world) {
                continue;
            }

            world->getRenderScene()->draw(bucket);
            DRAW_PARAMS params = {
                .view = vp->getViewTransform(),
                .projection = vp->getProjection(),
                .vp_rect_ratio = vp->getRect(),
                .viewport_x = (int)(target->getWidth() * vp->getRect().min.x),
                .viewport_y = (int)(target->getHeight() * vp->getRect().min.y),
                .viewport_width = (int)(target->getWidth() * (vp->getRect().max.x - vp->getRect().min.x)),
                .viewport_height = (int)(target->getHeight() * (vp->getRect().max.y - vp->getRect().min.y)),
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
        }/*
        for (int i = 0; i < viewports.size(); ++i) {
            Viewport* vp = viewports[i];
            const gfxm::rect& rc_normalized = vp->getRect();
            gpuRenderTarget* target = vp->getRenderTarget();
            if (!viewports[i]->isOffscreen()) {
                gpuDrawToDefaultFrameBuffer(target, rc_normalized);
            }
        }*/

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

        engine_stats.frame_time_no_vsync = timer_.stop();
        engine_stats.render_time = timer_render.stop();
        platformSwapBuffers();

        engine_stats.frame_time = timer_.stop();
        total_time += engine_stats.frame_time;
        // Don't let the frame time be too large
        dt = gfxm::_min(1.f / 15.f, engine_stats.frame_time);

        engine_stats.fps = 1.0f / dt;
    }
}

void    engineAddViewport(Viewport* vp) {
    if (vp->getRenderTarget() == 0) {
        vp->setRenderTarget(gpuGetDefaultRenderTarget());
    }
    viewports.push_back(vp);
}
void    engineRemoveViewport(Viewport* vp) {
    auto it = std::find(viewports.begin(), viewports.end(), vp);
    if (it == viewports.end()) {
        return;
    }
    viewports.erase(it);
}

ENGINE_STATS& engineGetStats() {
    return engine_stats;
}

