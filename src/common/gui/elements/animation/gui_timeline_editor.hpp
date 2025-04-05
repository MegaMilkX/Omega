#pragma once

#include "gui/elements/element.hpp"

#include "gui/gui_icon.hpp"
#include "gui/gui.hpp"
#include "gui/elements/animation/gui_timeline_track_list.hpp"
#include "gui/elements/animation/gui_timeline_track_view.hpp"
#include "gui/elements/animation/gui_timeline_bar.hpp"
#include "gui/elements/splitters/gui_splitter_grid4.hpp"

#include "math/bezier.hpp"

// The derivative of x from t
inline float computeXDerivativeFromT(float t, const gfxm::vec2& p0, const gfxm::vec2& p1, const gfxm::vec2& p2, const gfxm::vec2& p3) {
    float u = 1 - t;
    return 3 * u * u * (p1.x - p0.x) + 6 * u * t * (p2.x - p1.x) + 3 * t * t * (p3.x - p2.x);
}

// Compute x from t
inline float computeTfromX(float t, const gfxm::vec2& p0, const gfxm::vec2& p1, const gfxm::vec2& p2, const gfxm::vec2& p3) {
    float u = 1 - t;
    return u * u * u * p0.x + 3 * u * u * t * p1.x + 3 * u * t * t * p2.x + t * t * t * p3.x;
}

// Find t for a particular x through Newton-Raphson method
inline float solveForT(float x, const gfxm::vec2& p0, const gfxm::vec2& p1, const gfxm::vec2& p2, const gfxm::vec2& p3, float guessT = 0.5f, float tolerance = 1e-6f, int maxIter = 100) {
    for (int i = 0; i < maxIter; ++i) {
        float currentX = computeTfromX(guessT, p0, p1, p2, p3) - x; // compute delta x
        float derivative = computeXDerivativeFromT(guessT, p0, p1, p2, p3); // compute derivative

        if (std::fabs(currentX) < tolerance) { 
            return gfxm::clamp(guessT, .0f, 1.f);
        }

        if (fabsf(derivative) < .0001f) {
            guessT += currentX > .0f ? -.0001f : .0001f;
        } else {
            guessT -= currentX / derivative; // update guess
        }
    }

    return gfxm::clamp(guessT, .0f, 1.f);
}

class GuiAnimCurveView : public GuiElement {
    gfxm::vec2 pan;
    gfxm::vec2 scale = gfxm::vec2(1.f, 1.f);
    float left_mouse_down = false;
    float mid_mouse_down = false;
    gfxm::vec2 last_mouse_pos;
    

    std::vector<gfxm::vec2> control_points;
    int hovered_node = -1;
    bool dragging_node = false;
    gfxm::vec2 internal_mouse;
public:
    GuiAnimCurveView() {
        setSize(gui::fill(), gui::fill());

        control_points = {
            gfxm::vec2(-10.f, 0.f), gfxm::vec2(.0f, .0f), gfxm::vec2(10.f, 0.f),
            gfxm::vec2(10.f, -10.f), gfxm::vec2(20.0f, -10.f), gfxm::vec2(30.f, -10.f),
            gfxm::vec2(30.f, 10.f), gfxm::vec2(40.0f, 10.f), gfxm::vec2(50.f, 10.f),
            gfxm::vec2(50.f, 0.f), gfxm::vec2(60.0f, 0.f), gfxm::vec2(70.f, 0.f)
        };
        pan = gfxm::vec2(-30.f, .0f);
        scale = gfxm::vec2(.1f, .1f);
    }
    gfxm::rect margin = gfxm::rect(50.f, 50.f, 50.f, 50.f);
    
    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!gfxm::point_in_rect(rc_bounds, gfxm::vec2(x, y))) {
            return;
        }

        if (!dragging_node) {
            gfxm::vec2 ndc_mouse(x, y);
            float x_side = (client_area.max.x - client_area.min.x);
            float y_side = (client_area.max.y - client_area.min.y);
            float x_halfside = x_side * .5f;
            float y_halfside = y_side * .5f;
            ndc_mouse.x = ((ndc_mouse.x - client_area.min.x) / x_side - .5f) * 2.f;
            ndc_mouse.y = ((ndc_mouse.y - client_area.min.y) / y_side - .5f) * 2.f;
            gfxm::mat4 projection = gfxm::ortho(-x_halfside, x_halfside, -y_halfside, y_halfside, -1.f, 1.f);
            gfxm::mat4 view = gfxm::inverse(gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(pan.x, pan.y, .0f)));
            gfxm::vec3 mouse = view * gfxm::inverse(projection) * gfxm::vec4(ndc_mouse.x * scale.x, ndc_mouse.y * scale.y, .0f, 1.f);
            gfxm::vec2 m2(mouse.x, mouse.y);
            internal_mouse = m2;
            hovered_node = -1;
            const float HIT_RADIUS = 7.f;
            for (int i = 0; i < control_points.size(); ++i) {
                if (((m2 - control_points[i]) * gfxm::vec2(1.f / scale.x, 1.f / scale.y)).length() < HIT_RADIUS) {
                    hovered_node = i;
                }
            }
        }
        
        hit.add(GUI_HIT::CLIENT, this);
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
        switch (msg) {
        case GUI_MSG::MOUSE_SCROLL: {
            gfxm::vec2 ndc_mouse = guiGetMousePos();
            float x_side = client_area.max.x - client_area.min.x;
            float y_side = client_area.max.y - client_area.min.y;
            ndc_mouse.x = ((ndc_mouse.x - client_area.min.x) / x_side - .5f) * 2.f;
            ndc_mouse.y = ((ndc_mouse.y - client_area.min.y) / y_side - .5f) * 2.f;
            gfxm::vec2 prev_scale = scale;
            gfxm::vec2 delta = (scale) * .2f;
            scale -= delta * (float)params.getA<int32_t>() * 0.01f;
            scale = gfxm::vec2(gfxm::_max(.001f, scale.x), gfxm::_max(.001f, scale.y));
            gfxm::vec2 pan_offs_mul_prev = gfxm::vec2(x_side, y_side) * .5f * (prev_scale);
            gfxm::vec2 pan_offs_mul = gfxm::vec2(x_side, y_side) * .5f * (scale);
            if (scale.x < prev_scale.x) {
                pan.x -= (ndc_mouse.x * (pan_offs_mul_prev.x - pan_offs_mul.x));
            } else if(scale.x > prev_scale.x) {
                pan.x += (ndc_mouse.x * (pan_offs_mul.x - pan_offs_mul_prev.x));
            }
            if (scale.y < prev_scale.y) {
                pan.y -= (ndc_mouse.y * (pan_offs_mul_prev.y - pan_offs_mul.y));
            } else if(scale.x > prev_scale.x) {
                pan.y += (ndc_mouse.y * (pan_offs_mul.y - pan_offs_mul_prev.y));
            }
            return true;
        }
        case GUI_MSG::LBUTTON_DOWN: {
            left_mouse_down = true;
            guiCaptureMouse(this);

            if (hovered_node >= 0) {
                dragging_node = true;
            }

            return true;
        }
        case GUI_MSG::LBUTTON_UP: {
            left_mouse_down = false;
            guiCaptureMouse(0);

            dragging_node = false;
            return true;
        }
        case GUI_MSG::MBUTTON_DOWN: {
            mid_mouse_down = true;
            guiCaptureMouse(this);
            return true;
        }
        case GUI_MSG::MBUTTON_UP: {
            mid_mouse_down = false;
            guiCaptureMouse(0);
            return true;
        }
        case GUI_MSG::MOUSE_MOVE: {/*
            if (left_mouse_down) {
                t_guess = (guiGetMousePos().x - (client_area.min.x + margin.min.x)) / ((client_area.max.x - margin.max.x) - (client_area.min.x + margin.min.x));
                return true;
            }*/
            if(mid_mouse_down) {
                if (guiIsModifierKeyPressed(GUI_KEY_CONTROL)) {
                    gfxm::vec2 mul = (scale) * .005f * gfxm::vec2(-1.f, 1.f);
                    scale += (guiGetMousePos() - last_mouse_pos) * mul;
                } else {
                    pan += (guiGetMousePos() - last_mouse_pos) * scale;
                }
            }
            gfxm::vec2 mouse_delta = guiGetMousePos() - last_mouse_pos;
            last_mouse_pos = guiGetMousePos();

            if (dragging_node) {
                int idx = hovered_node;
                control_points[idx] += mouse_delta * scale;
                if ((idx - 1) % 3 == 0) {
                    control_points[idx - 1] += mouse_delta * scale;
                    control_points[idx + 1] += mouse_delta * scale;
                }/* else if((idx - 2) % 3 == 0) {
                    control_points[idx - 2] -= mouse_delta * zoom;
                } else if((idx) % 3 == 0) {
                    control_points[idx + 2] -= mouse_delta * zoom;
                }*/
            }
            return true;
        }
        }
        return GuiElement::onMessage(msg, params);
    }
    void onDraw() {
        guiDrawRect(client_area, 0xFF111111);

        guiPushViewportRect(getGlobalClientArea());
        float x_side = (client_area.max.x - client_area.min.x) * scale.x;
        float y_side = (client_area.max.y - client_area.min.y) * scale.y;
        float x_halfside = x_side * .5f;
        float y_halfside = y_side * .5f;
        guiPushProjectionOrthographic(-x_halfside, x_halfside, y_halfside, -y_halfside);
        guiPushViewTransform(gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(pan.x, pan.y, .0f)));

        const gfxm::vec2 units_per_pixel = scale;
        const gfxm::vec2 rawStep = 50.f * units_per_pixel;
        const gfxm::vec2 base = gfxm::vec2(powf(10.f, floorf(log10f(rawStep.x))), powf(10.f, floorf(log10f(rawStep.y))));
        const gfxm::vec2 normalized_step = gfxm::vec2(rawStep.x / base.x, rawStep.y / base.y);
        gfxm::vec2 step;
        if (normalized_step.x < 2) step.x = 2;
        else if (normalized_step.x < 4) step.x = 4;
        else if (normalized_step.x < 5) step.x = 5;
        else if (normalized_step.x < 10) step.x = 10;
        else step.x = 20;
        if (normalized_step.y < 2) step.y = 2;
        else if (normalized_step.y < 4) step.y = 4;
        else if (normalized_step.y < 5) step.y = 5;
        else if (normalized_step.y < 10) step.y = 10;
        else step.y = 20;
        step *= base;
        gfxm::vec2 grid_spacing = step;
        gfxm::vec2 offs = gfxm::vec2(
            pan.x - (grid_spacing.x * roundf(pan.x / grid_spacing.x)),
            pan.y - (grid_spacing.y * roundf(pan.y / grid_spacing.y))
        );
        gfxm::vec2 mid = -gfxm::vec2(pan.x - fmodf(pan.x, grid_spacing.x), pan.y - fmodf(pan.y, grid_spacing.y));// +gfxm::vec2(fmodf(pan.x, grid_spacing.x), fmodf(pan.y, grid_spacing.y));
        gfxm::rect grid_sides = gfxm::rect(
            (-gfxm::vec2(x_halfside, y_halfside) - pan),
            (gfxm::vec2(x_halfside, y_halfside) - pan)
        );

        // Clip range
        {
            float from = .0f;
            float to = 60.f;
            guiDrawRect(
                gfxm::rect(
                    gfxm::vec2(from, grid_sides.min.y),
                    gfxm::vec2(to, grid_sides.max.y)
                ),
                GUI_COL_BG
            );
        }

        int x_line_count = int(x_side / grid_spacing.x) + 1;
        int x_half_line_count = x_line_count / 2;
        for (int i = -x_half_line_count; i <= x_half_line_count; ++i) {
            float x = mid.x + roundf(i * grid_spacing.x);
            guiDrawLine(
                gfxm::rect(
                    gfxm::vec2(x, grid_sides.min.y),
                    gfxm::vec2(x, grid_sides.max.y)
                ),
                2.f, 0xCC000000, scale
            );
            guiDrawText(gfxm::vec2(x, grid_sides.min.y), MKSTR(x).c_str(), getFont(), 0, GUI_COL_WHITE, scale);
        }
        int y_line_count = int(y_side / grid_spacing.y) + 1;
        int y_half_line_count = y_line_count / 2;
        for (int i = -y_half_line_count; i <= y_half_line_count; ++i) {
            float y = mid.y + roundf(i * grid_spacing.y);
            guiDrawLine(
                gfxm::rect(
                    gfxm::vec2(grid_sides.min.x, y),
                    gfxm::vec2(grid_sides.max.x, y)
                ),
                2.f, 0xCC000000, scale
            );
            guiDrawText(gfxm::vec2(grid_sides.min.x, y), MKSTR(y).c_str(), getFont(), 0, GUI_COL_WHITE, scale);
        }

        if (control_points.size() > 3) {
            for (int i = 4; i < control_points.size() - 1; i += 3) {
                auto a = control_points[i - 3];
                auto b = control_points[i - 2];
                auto c = control_points[i - 1];
                auto d = control_points[i];
                b.x = gfxm::clamp(b.x, a.x, d.x);
                c.x = gfxm::clamp(c.x, a.x, d.x);
                guiDrawBezierCurve(
                    a, b, c, d,
                    2.f, GUI_COL_RED, scale
                );
            }
        }
        for (int i = 1; i < control_points.size(); i += 3) {
            guiDrawLine(control_points[i], control_points[i - 1], 4.f, 0x33FFFFFF, scale);
            guiDrawLine(control_points[i], control_points[i + 1], 4.f, 0x33FFFFFF, scale);
            guiDrawDiamond(control_points[i - 1], 2.5f, 0xFFAAAAAA, scale);
            guiDrawDiamond(control_points[i + 1], 2.5f, 0xFFAAAAAA, scale);
            
            guiDrawDiamond(control_points[i], 5.f, hovered_node == i ? GUI_COL_WHITE : GUI_COL_YELLOW, scale);
        }

        // Cursor
        {
            int frame_cursor = 0;
            float x = frame_cursor;
            guiDrawLine(
                gfxm::rect(
                    gfxm::vec2(x, grid_sides.min.y),
                    gfxm::vec2(x, grid_sides.max.y)
                ),
                2.f, GUI_COL_YELLOW, scale
            );
        }
        
        //guiPopTransform();
        guiPopViewTransform();
        guiPopProjection();
        guiPopViewportRect();
    }
};


// Should persist when events are being dragged across tracks or inside the same track
struct GuiTimelineEventData {
    GuiTimelineEventTrack* track;
    int frame;
    void* user_ptr;
};

class GuiTimelineEditor : public GuiElement {
    bool is_playing = false;

    gfxm::rect rc_left;
    gfxm::rect rc_right;
    gfxm::rect rc_splitter;

    std::unique_ptr<GuiSplitterGrid4> splitter;
    std::unique_ptr<GuiTimelineBar> track_bar;
    std::unique_ptr<GuiTimelineTrackList> track_list;
    std::unique_ptr<GuiTimelineTrackView> track_view;
    // TODO:
    std::unique_ptr<GuiIconButton> button_play;

    std::unique_ptr<GuiScrollBarV> scroll_v;
    std::unique_ptr<GuiScrollBarH> scroll_h;

    int next_track_id = 0;
public:
    std::function<void(void)> on_play;
    std::function<void(void)> on_pause;
    std::function<void(int)> on_cursor;

    GuiTimelineKeyframeTrack* findKeyframeTrack(const std::string& name) {
        return track_view->findKeyframeTrack(name);
    }

    GuiTimelineKeyframeTrack* addKeyframeTrack(GUI_KEYFRAME_TYPE type, const char* name, void* user_ptr) {
        track_list->addItem(name);
        auto trk = track_view->addKeyframeTrack(type, name, next_track_id);
        int track_id = next_track_id;
        ++next_track_id;
        //trk->user_ptr = user_ptr;
        return trk;
    }
    GuiTimelineEventTrack* addEventTrack(void* user_ptr) {
        track_list->addItem("EventTrack");
        auto trk = track_view->addEventTrack(next_track_id);
        int track_id = next_track_id;
        ++next_track_id;
        trk->user_ptr = user_ptr;
        return trk;
    }
    GuiTimelineBlockTrack* addBlockTrack(const char* name = "BlockTrack") {
        track_list->addItem(name);
        auto trk = track_view->addBlockTrack();
        return trk;
    }

    void togglePlay() {
        if (!is_playing) {
            is_playing = true;
            button_play->setIcon(guiLoadIcon("svg/entypo/controller-paus.svg"));
            if (on_play) { on_play(); }
        } else {
            is_playing = false;
            button_play->setIcon(guiLoadIcon("svg/entypo/controller-play.svg"));
            if (on_pause) { on_pause(); }
        }
    }

    void setCursor(int frame, bool send_notification = true) {
        frame = std::max(frame, 0);
        track_bar->setCursor(frame, send_notification);
        track_view->setCursor(frame, send_notification);
        if (on_cursor) { on_cursor(frame); }
    }
    void setCursorSilent(int frame) {
        frame = std::max(frame, 0);
        track_bar->setCursor(frame, false);
        track_view->setCursor(frame, false);
    }
    int getCursor() const {
        return track_view->getCursor();
    }
    GuiTimelineEditor() {
        setMinSize(0, 0);
        setMaxSize(0, 0);
        setSize(0, 0);

        splitter.reset(new GuiSplitterGrid4);
        addChild(splitter.get());

        scroll_v.reset(new GuiScrollBarV);
        scroll_h.reset(new GuiScrollBarH);

        track_bar.reset(new GuiTimelineBar);

        track_list.reset(new GuiTimelineTrackList);

        track_view.reset(new GuiTimelineTrackView);

        button_play.reset(new GuiIconButton(guiLoadIcon("svg/entypo/controller-play.svg")));
        button_play->setOwner(this);

        scroll_v->setOwner(this);
        scroll_h->setOwner(this);
        track_bar->setOwner(this);
        track_list->setOwner(this);
        track_view->setOwner(this);

        splitter->setElemTopLeft(button_play.get());
        splitter->setElemTopRight(track_bar.get());
        splitter->setElemBottomLeft(track_list.get());
        splitter->setElemBottomRight(track_view.get());
    }
    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return;
        }
        splitter->hitTest(hit, x, y);
        if (hit.hasHit()) {
            return;
        }
        hit.add(GUI_HIT::CLIENT, this);
        return;
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::NOTIFY:
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::TIMELINE_JUMP:
                setCursor(params.getB<int>(), false);
                return true;
            case GUI_NOTIFY::TIMELINE_ZOOM:
                track_bar->setFrameWidth(params.getB<float>());
                return true;
            case GUI_NOTIFY::TIMELINE_PAN_X:
                track_bar->content_offset.x = params.getB<float>();
                track_view->content_offset.x = params.getB<float>();
                return true;
            case GUI_NOTIFY::TIMELINE_PAN_Y:
                track_bar->content_offset.y = params.getB<float>();
                track_view->content_offset.y = params.getB<float>();
                return true;
            case GUI_NOTIFY::BUTTON_CLICKED:
                togglePlay();
                return true;
            case GUI_NOTIFY::TIMELINE_EVENT_ADDED: {
                forwardMessageToOwner(msg, params);
                }return true;
            case GUI_NOTIFY::TIMELINE_EVENT_REMOVED: {
                forwardMessageToOwner(msg, params);
                }return true;
            case GUI_NOTIFY::TIMELINE_EVENT_MOVED: {
                forwardMessageToOwner(msg, params);
                }return true;
            case GUI_NOTIFY::TIMELINE_BLOCK_ADDED: {
                forwardMessageToOwner(msg, params);
                }return true;
            case GUI_NOTIFY::TIMELINE_BLOCK_REMOVED: {
                forwardMessageToOwner(msg, params);
                }return true;
            case GUI_NOTIFY::TIMELINE_BLOCK_MOVED_RESIZED: {
                forwardMessageToOwner(msg, params);
                }return true;
            case GUI_NOTIFY::TIMELINE_EVENT_SELECTED:
                forwardMessageToOwner(msg, params);
                return true;
            case GUI_NOTIFY::TIMELINE_BLOCK_SELECTED:
                forwardMessageToOwner(msg, params);
                return true;
            }
            break;
        }
        return false;
    }
    void onLayout(const gfxm::vec2& extents, uint64_t flags) override {
        rc_bounds = gfxm::rect(gfxm::vec2(0, 0), extents);
        client_area = rc_bounds;

        splitter->layout_position = client_area.min;
        splitter->layout(gfxm::rect_size(client_area), flags);
    }
    void onDraw() override {
        splitter->draw();
    }
};


