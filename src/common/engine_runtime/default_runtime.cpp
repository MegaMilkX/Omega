#include "default_runtime.hpp"

#include <format>

#include "platform/platform.hpp"
#include "transform_node/transform_system.hpp"
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
#include "con_registry/con_registry.hpp"


DefaultRuntime::DefaultRuntime(IGameInstance* game)
: game_instance(game) {

}

void DefaultRuntime::onDisplayChanged(int w, int h) {
    // TODO:
    //gpuGetDefaultRenderTarget()->setSize(w, h);
    //game_instance->onViewportResize(w, h);
}

static void drawView(EngineRenderView* rv, float time) {
    gpuRenderer* renderer = rv->getRenderer();
    gpuRenderTarget* target = rv->getRenderTarget();
    gpuRenderBucket* bucket = rv->getRenderBucket();

    for (int j = 0; j < rv->queryInterfaceCount(); ++j) {
        auto qi = rv->getQueryInterface(j);
        if (!qi) {
            assert(false);
            continue;
        }
        VisibilityQuery vq(rv->getProjection(), rv->getViewTransform(), 0);
        GeometryQuery query_geo(vq, bucket);
        qi->queryGeometry(query_geo);
    }            

    DRAW_PARAMS params = {
        .view = rv->getViewTransform(),
        .view_prev = rv->getViewTransform(), // TODO: motion blur
        .projection = rv->getProjection(),
        .vp_rect_ratio = rv->getRect(),
        .viewport_x = (int)(target->getWidth() * rv->getRect().min.x),
        .viewport_y = (int)(target->getHeight() * rv->getRect().min.y),
        .viewport_width = (int)(target->getWidth() * (rv->getRect().max.x - rv->getRect().min.x)),
        .viewport_height = (int)(target->getHeight() * (rv->getRect().max.y - rv->getRect().min.y)),
        .time = time
    };

    renderer->draw(bucket, rv, params);
}

void DefaultRuntime::run() {
    // Init
    {
        if (game_instance) {
            game_instance->init(this);
        }

        {
            auto conreg = ConRegistry::get();
            conreg->registerBool("r.vsync", "vertical sync", true);
            conreg->getBoolVar("r.vsync")->on_change.subscribe([](bool v) {
                wglSwapIntervalEXT(v ? 1 : 0);
            });

            ConRegistry::get()->registerFloat("r.gamma", "gamma correction", 2.2f, .0f, 100.f);
            ConRegistry::get()->registerFloat("r.exposure", "exposure", 1.0f, .0f, 100.f);
            ConRegistry::get()->registerBool("r.wire", "wireframe mode", false);
            conreg->getFloatVar("r.gamma")->on_change.subscribe([](float v) {
                gpuGetPipeline()->setGamma(v);
            });
            conreg->getFloatVar("r.exposure")->on_change.subscribe([](float v) {
                gpuGetPipeline()->setExposure(v);
            });
            conreg->getBoolVar("r.wire")->on_change.subscribe([this](bool v) {
                auto pipe = gpuGetPipeline();
                if (pipe->viewCount() == 0) {
                    return;
                }
                if (!pipe->getView(0)->getRenderTarget()) {
                    return;
                }
                pipe->getView(0)->getRenderTarget()->dbg_drawWireframe = v;
            });

            conreg->registerCmd("snd", "play a sound clip", [](const ConsoleCommand& cmd) {
                static ResourceRef<AudioClip> clip;
                static Handle<AudioChannel> chan;
                auto clip_name = cmd.arg<std::string>(0);
                clip = loadResource<AudioClip>(clip_name);
                if (!clip) {
                    LOG_WARN("No such audio clip: '" << clip_name << "'");
                    return true;
                }
                if (chan) {
                    audioFreeChannel(chan);
                }
                chan = audioCreateChannel();
                audioSetBuffer(chan, clip->getBuffer());
                audioSetGain(chan, .3f);
                audioSetLooping(chan, false);
                audioPlay(chan);
            });
        }

        // Developer console
        {
            playerGetPrimary()->getInputState()->pushContext(&input_ctx);
            inputCreateActionDesc("dev_console")
                .linkKey(Key.Keyboard.Tilde, 1.f);
            inputDevConsole = input_ctx.createAction("dev_console");

            dev_console = guiCreate<GuiDevConsole>(this);
            dev_console->setHidden(true);
            guiGetRoot()->getOverlay()->pushBack(dev_console);

            guiGetStyleSheet()
                .add("dev-console", {
                    gui::background_color(0x99000000),
                    gui::font_size(22),
                    gui::font_file("fonts/nimbusmono-bold.otf"),
                    //gui::font_size(16),
                    //gui::font_file("fonts/ProggyClean.ttf"),
                    gui::padding(gui::em(.5f), gui::em(.25f), gui::em(.5f), gui::em(.25f)),
                    gui::content_margin(gui::em(.5), gui::em(.5))
                })
                .add("dev-console-log", {
                    gui::valign(GUI_VERTICAL_ALIGNMENT::BOTTOM)
                })
                .add("dev-console-log-entry", {
                    gui::content_margin(gui::em(.5), gui::em(.5))
                })
                .add("dev-console-log-type", {
                })
                .add("dev-console-prompt", {
                    gui::color(GUI_COL_BUTTON_HIGHLIGHT)
                })
                .add("log-message", {
                })
                .add("log-warning", {
                    gui::background_color(GUI_COL_YELLOW)
                })
                .add("log-error", {
                    gui::background_color(GUI_COL_RED)
                })
                .add("log-debug", {
                    gui::background_color(GUI_COL_BLUE)
                });
        }

        // Stats label
        {
            stats_label = new GuiTextElement("FPS: -");
            stats_label->setStyleClasses({"perf-stats"});

            auto gui_overlay = guiGetRoot()->getOverlay();
            gui_overlay->pushBack(stats_label);
            /*
            gui_overlay->pushBack("SAME LINE ELEMENT", { "paragraph", "notification" })
                ->addFlags(GUI_FLAG_SAME_LINE);
            */
            guiGetStyleSheet()
                .add("perf-stats", {
                    gui::background_color(0x99000000),
                    gui::border_radius(0, 0, gui::em(1), 0),
                    gui::font_size(22),
                    gui::font_file("fonts/nimbusmono-bold.otf"),
                    gui::padding(gui::em(.5f), gui::em(.25f), gui::em(.5f), gui::em(.25f))
                });
            
            ConRegistry::get()->registerInt("perflabel", "performance display label variant\n\t0 - none, 1 - minimal, 2 - full", 2, 0, 2);
            ConRegistry::get()->watchInt("perflabel", [this](int i) {
                stats_label->setHidden(i == 0);
            });
        }
        /*
        {
            auto box = guiGetRoot()->getOverlay()->pushBack(guiCreate<GuiElement>());
            box->setSize(gui::content(), gui::content());
            box->setStyleClasses({ "dbg-box" });

            box->pushBack("Hello", { "paragraph", "dbg-10" })
                ->setSize(gui::fill(), gui::content());
            box->pushBack("Text element", { "paragraph", "dbg-10" })
                ->setSize(gui::content(), gui::content());
            box->pushBack("Another\ntext\nelement", { "paragraph", "dbg-10" })
                ->setSize(gui::content(), gui::content());
            box->pushBack("qweqwe qwe", { "paragraph", "dbg-10" })
                ->setSize(gui::fill(), gui::content());
        }
        {
            guiGetRoot()->getOverlay()->pushBack("Hello, World!\nForced line break here\nAnother line");
            auto box = guiGetRoot()->getOverlay()->pushBack(new GuiElement);
            box->setSize(gui::px(300), gui::content());
            {
                auto header = box->pushBack(new GuiCollapsingHeader());
                header->pushBack("Hello, World!");
                header->pushBack("AAAAAAAAAAAA\nweqeqwe");
            }
            guiGetRoot()->getOverlay()->pushBack("Hello, World!\nForced line break here\nAnother line", { "paragraph" });
            
            guiGetRoot()->getOverlay()->pushBack(R"(Then Fingolfin beheld (as it seemed to him) the utter ruin of the Noldor,
and the defeat beyond redress of all their houses;
and filled with wrath and despair he mounted upon Rochallor his great horse and rode forth alone,
and none might restrain him.)", { "paragraph", "window-frame" });
            guiGetRoot()->getOverlay()->pushBack(R"(He passed over Dor-nu-Fauglith like a wind amid the dust,
and all that beheld his onset fled in amaze, thinking that Orome himself was come:
for a great madness of rage was upon him, so that his eyes shone like the eyes of the Valar.
Thus he came alone to Angband's gates, and he sounded his horn,
and smote once more upon the brazen doors,
and challenged Morgoth to come forth to single combat. And Morgoth came.)", { "paragraph", "window-frame" });
        }
        */
    }

    ConRegistry::get()->runFile("autoexec.cfg");

    timer timer_;
    timer timer_render;
    timer timer_gpu_wait;
    timer timer_ui_layout;
    timer timer_ui_draw;
    timer timer_ui_render;
    float dt = 1.f / 60.f;
    float total_time = .0f;
    while (platformIsRunning()) {
        timer_.start();

        platformPollMessages();
        TransformSystem::nextFrame();
        inputUpdate(dt);

        if (inputDevConsole->isJustPressed()) {
            dev_console->setHidden(!dev_console->isHidden());
            if (!dev_console->isHidden()) {
                dev_console->focusInput();
            } else {
                dev_console->unfocusInput();
            }
        }

        if (game_instance) {
            game_instance->update(dt);
        }

        {
            static ConInt* con_perf_kind = ConRegistry::get()->getIntVar("perflabel");
            if (con_perf_kind->get() == 1) {
                stats_label->setContent(
                    std::format(
                        "FPS: {:.1f}",
                        stats.fps
                    ).c_str()
                );
            } else if(con_perf_kind->get() == 2) {
                float leftover_perc = (stats.frame_time - stats.frame_time_no_vsync) / stats.frame_time * 100.f;
                stats_label->setContent(
                    std::format(
                        "Frame time (no vsync): \t{:.3f}ms\n"
                        "Leftover: \t\t\t\t{:.3f}ms\t{:.2f}%\n"
                        "CPU draw: \t\t\t\t{:.3f}ms\n"
                        "SwapBuffers: \t\t\t{:.3f}ms\n"
                        "Frame time: \t\t\t{:.3f}ms\n"
                        "Collision: \t\t\t\t{:.3f}ms\n"
                        "Audio: \t\t\t\t\t{:.3f}ms\n"
                        "UI Layout: \t\t\t\t{:.3f}ms\n"
                        "UI Draw: \t\t\t\t{:.3f}ms\n"
                        "UI Render: \t\t\t\t{:.3f}ms\n"
                        "FPS: \t\t\t\t\t{:.1f}\n"
                        "Param block uploads: \t{}\n"
                        "Skin task execs: \t\t{}",
                        stats.frame_time_no_vsync * 1000.f,
                        (stats.frame_time - stats.frame_time_no_vsync) * 1000.f, leftover_perc,
                        stats.cpu_draw_time * 1000.f,
                        stats.gpu_wait_time * 1000.f,
                        stats.frame_time * 1000.f,
                        stats.collision_time * 1000.f,
                        audioGetStats().buffer_update_time.load() * 1000.f,
                        stats.ui_layout_time * 1000.f,
                        stats.ui_draw_time * 1000.f,
                        stats.ui_render_time * 1000.f,
                        stats.fps,
                        gpuGetPipeline()->dbg_getParamBlockUploadCount(),
                        gpuGetSkinTaskExecCount()
                    ).c_str()
                );
            }
        }

        guiPollMessages();
        timer_ui_layout.start();
        guiLayout();
        stats.ui_layout_time = timer_ui_layout.stop();
        guiUpdate(dt);
        timer_ui_draw.start();
        guiDraw();
        stats.ui_draw_time = timer_ui_draw.stop();

        if (game_instance) {
            game_instance->draw(dt);
        }

        // Render viewports
        timer_render.start();
        
        // TODO: This or the callback? Choose one
        int screen_w, screen_h;
        platformGetWindowSize(screen_w, screen_h);
        for (int i = 0; i < gpuGetPipeline()->viewCount(); ++i) {
            auto view = gpuGetPipeline()->getView(i);
            if (gpuRenderTarget* rt = view->getRenderTarget()) {
                rt->setSize(screen_w, screen_h);
            }
        }

        for (int i = 0; i < gpuGetPipeline()->viewCount(); ++i) {
            EngineRenderView* rv = gpuGetPipeline()->getView(i);
            drawView(rv, total_time);
        }

        // Blit to screen
        for (int i = 0; i < gpuGetPipeline()->viewCount(); ++i) {
            auto view = gpuGetPipeline()->getView(i);
            if (view->isOffscreen()) {
                continue;
            }

            if (gpuRenderTarget* rt = view->getRenderTarget()) {
                auto rc = view->getRect();

                gpuDrawToDefaultFrameBuffer(rt, rc);
                /*
                gpuDrawTextureToDefaultFrameBuffer(
                    rt->getTexture("Depth"), nullptr,
                    RT_OUTPUT_DEPTH, gfxm::rect(rc.min + (rc.max - rc.min) * gfxm::vec2(.0f, .0f), rc.min + (rc.max - rc.min) * gfxm::vec2(.2f, .2f))
                );
                gpuDrawTextureToDefaultFrameBuffer(
                    rt->getTexture("DepthLayer"), nullptr,
                    RT_OUTPUT_DEPTH, gfxm::rect(rc.min + (rc.max - rc.min) * gfxm::vec2(.0f, .2f), rc.min + (rc.max - rc.min) * gfxm::vec2(.2f, .4f))
                );
                gpuDrawTextureToDefaultFrameBuffer(
                    rt->getTexture("Normal"), nullptr,
                    RT_OUTPUT_AUTO, gfxm::rect(rc.min + (rc.max - rc.min) * gfxm::vec2(.8f, 0), rc.min + (rc.max - rc.min) * gfxm::vec2(1.0f, .2f))
                );*/
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

        timer_ui_render.start();
        guiRender(false);
        stats.ui_render_time = timer_ui_render.stop();
        stats.cpu_draw_time = timer_render.stop();

        ResourceManager::get()->collectGarbage();

        stats.frame_time_no_vsync = timer_.stop();
        timer_gpu_wait.start();
        platformSwapBuffers();
        stats.gpu_wait_time = timer_gpu_wait.stop();

        ResourceManager::get()->getBackend<gpuTexture2d>()->update();

        stats.frame_time = timer_.stop();
        total_time += stats.frame_time;
        // Don't let the frame time be too large
        dt = gfxm::_min(1.f / 15.f, stats.frame_time);

        stats.fps = 1.0f / stats.frame_time;
    }
}

