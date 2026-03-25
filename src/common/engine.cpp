#include "engine.hpp"

#include "platform/platform.hpp"
#include "init_handler/init_handler.hpp"
#include "reflect.hpp"

#include "animation/animation.hpp"
#include "audio/audio.hpp"

#include "input/input.hpp"

#include "gui/gui.hpp"

#include "util/timer.hpp"

static IGameInstance* s_game_instance = 0;
static InitHandlerRAII* engine_init_handler = 0;
static std::vector<EngineRenderView*> render_views;
static ENGINE_STATS engine_stats;


static void onWindowResize(int width, int height) {
    gpuGetDefaultRenderTarget()->setSize(width, height);
    s_game_instance->onViewportResize(width, height);
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

    // onWindowResize moved to IEngineRuntime
    //platformSetWindowResizeCallback(&onWindowResize);

    return true;
}

void engineGameCleanup() {
    delete engine_init_handler;
    engine_init_handler = 0;
}

void    engineAddViewport(EngineRenderView* vp) {
    if (vp->getRenderTarget() == 0) {
        vp->setRenderTarget(gpuGetDefaultRenderTarget());
    }
    render_views.push_back(vp);
}
void    engineRemoveViewport(EngineRenderView* vp) {
    auto it = std::find(render_views.begin(), render_views.end(), vp);
    if (it == render_views.end()) {
        return;
    }
    render_views.erase(it);
}

ENGINE_STATS& engineGetStats() {
    return engine_stats;
}

