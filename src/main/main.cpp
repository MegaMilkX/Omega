#include "reflect.hpp"
#include "platform/platform.hpp"
#include "input/input.hpp"
#include "typeface/typeface.hpp"
#include "util/timer.hpp"
#include "resource/resource.hpp"
#include "animation/animation.hpp"
#include "audio/res_cache_audio_clip.hpp"
#include "audio/audio_mixer.hpp"
#include "init_handler/init_handler.hpp"

#include "game/game_test.hpp"


static GameBase* engine_game_instance = 0;
static InitHandlerRAII* engine_init_handler = 0;


static void onWindowResize(int width, int height) {
    gpuGetDefaultRenderTarget()->setSize(width, height);
    engine_game_instance->onViewportResize(width, height);
}

int engineGameInit() {
    engine_init_handler = new InitHandlerRAII;
    
    build_config::gpuPipelineCommon* gpu_pipeline = 0;

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
            [&gpu_pipeline]()->bool {
                gpu_pipeline = new build_config::gpuPipelineCommon;
                gpuInit(gpu_pipeline);
                return true;
            },
            [gpu_pipeline]() {
                gpuCleanup();
                delete gpu_pipeline;
            }
        )
        .add("Audio",
            []()->bool {
                resAddCache<AudioClip>(new resCacheAudioClip);
                audio().init(44100, 16);
                return true;
            },
            []() {
                audio().cleanup();
            }
        );
    if (!engine_init_handler->init()) {
        return false;
    }

    platformSetWindowResizeCallback(&onWindowResize);

    return true;
}
void engineGameSet(GameBase* game) {
    engine_game_instance = game;
    engine_game_instance->init();
}
void engineGameLoop() {
    timer timer_;
    float dt = 1.0f / 60.0f;
    while (platformIsRunning()) {
        timer_.start();
        platformPollMessages();

        inputUpdate(dt);
        
        if (engine_game_instance) {
            engine_game_instance->update(dt);
        }
        /*
        int world_count = gameWorldGetUpdateListCount();
        gameWorld** world_list = gameWorldGetUpdateList();
        for (int i = 0; i < world_count; ++i) {
            world_list[i]->update(dt);
        }*/

        if (engine_game_instance) {
            engine_game_instance->draw(dt);
        }

        gpuDrawToDefaultFrameBuffer(gpuGetDefaultRenderTarget());
        platformSwapBuffers();

        // Don't let the frame time be too large
        dt = gfxm::_min(1.f / 15.f, timer_.stop());
    }
}
void engineGameCleanup() {
    delete engine_init_handler;
    engine_init_handler = 0;
}

int main(int argc, char* argv) {
    engineGameInit();

    GameTest game;
    engineGameSet(&game);

    engineGameLoop();

    engineGameCleanup();
    return 0;
}
