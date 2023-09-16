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
    float zoom = 1.f;
    float left_mouse_down = false;
    float mid_mouse_down = false;
    gfxm::vec2 last_mouse_pos;
    

    std::vector<gfxm::vec2> control_points;
    int hovered_node = -1;
    bool dragging_node = false;
    gfxm::vec2 internal_mouse;
public:
    GuiAnimCurveView() {
        setSize(gui::perc(100), gui::perc(100));

        control_points = {
            gfxm::vec2(-10.f, 0.f), gfxm::vec2(.0f, .0f), gfxm::vec2(10.f, 0.f),
            gfxm::vec2(10.f, -10.f), gfxm::vec2(20.0f, -10.f), gfxm::vec2(30.f, -10.f),
            gfxm::vec2(30.f, 10.f), gfxm::vec2(40.0f, 10.f), gfxm::vec2(50.f, 10.f),
            gfxm::vec2(50.f, 0.f), gfxm::vec2(60.0f, 0.f), gfxm::vec2(70.f, 0.f)
        };
        pan = gfxm::vec2(-30.f, .0f);
        zoom = 0.1f;
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
            gfxm::vec3 mouse = view * gfxm::inverse(projection) * gfxm::vec4(ndc_mouse.x * zoom, ndc_mouse.y * zoom, .0f, 1.f);
            gfxm::vec2 m2(mouse.x, mouse.y);
            internal_mouse = m2;
            hovered_node = -1;
            const float HIT_RADIUS = 7.f;
            for (int i = 0; i < control_points.size(); ++i) {
                if ((m2 - control_points[i]).length() * (1.f / zoom) < HIT_RADIUS) {
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
            float prev_zoom = zoom;
            float dz = (zoom) * .2f;
            zoom -= params.getA<int32_t>() * dz * 0.01f;
            zoom = gfxm::_max(.001f, zoom);
            gfxm::vec2 pan_offs_mul_prev = gfxm::vec2(x_side, y_side) * .5f * (prev_zoom);
            gfxm::vec2 pan_offs_mul = gfxm::vec2(x_side, y_side) * .5f * (zoom);
            if (zoom < prev_zoom) {
                pan -= (ndc_mouse * (pan_offs_mul_prev - pan_offs_mul));
            } else if(zoom > prev_zoom) {
                pan += (ndc_mouse * (pan_offs_mul - pan_offs_mul_prev));
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
                pan += (guiGetMousePos() - last_mouse_pos) * zoom;
            }
            gfxm::vec2 mouse_delta = guiGetMousePos() - last_mouse_pos;
            last_mouse_pos = guiGetMousePos();

            if (dragging_node) {
                int idx = hovered_node;
                control_points[idx] += mouse_delta * zoom;
                if ((idx - 1) % 3 == 0) {
                    control_points[idx - 1] += mouse_delta * zoom;
                    control_points[idx + 1] += mouse_delta * zoom;
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

        guiPushViewportRect(client_area);
        float x_side = (client_area.max.x - client_area.min.x) * zoom;
        float y_side = (client_area.max.y - client_area.min.y) * zoom;
        float x_halfside = x_side * .5f;
        float y_halfside = y_side * .5f;
        guiPushProjectionOrthographic(-x_halfside, x_halfside, y_halfside, -y_halfside);
        guiPushViewTransform(gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(pan.x, pan.y, .0f)));
        //gfxm::rect rc_viewport = platformGetViewportRect();
        //float x_ratio = (client_area.min.x - rc_viewport.min.x) / (rc_viewport.max.x - rc_viewport.min.x);
        //float y_ratio = (client_area.min.y - rc_viewport.min.y) / (rc_viewport.max.y - rc_viewport.min.y);
        //guiPushTransform(gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(x_ratio, y_ratio, .0f)));


        // zoom == .01f; spacing = 1
        // zoom == .1f; spacing = 10
        // zoom == 1.f; spacing = 100
        float units_per_pixel = zoom;
        float rawStep = 50.f * units_per_pixel;
        float base = powf(10.f, floorf(log10f(rawStep)));
        float normalized_step = rawStep / base;
        float step;
        if (normalized_step < 2) step = 2;
        else if (normalized_step < 4) step = 4;
        else if (normalized_step < 5) step = 5;
        else if (normalized_step < 10) step = 10;
        else step = 20;
        step *= base;
        gfxm::vec2 grid_spacing = gfxm::vec2(step, step);
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
                2.f, 0xCC000000, zoom
            );
            guiDrawText(gfxm::vec2(x, grid_sides.min.y), MKSTR(x).c_str(), getFont(), 0, GUI_COL_WHITE, zoom);
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
                2.f, 0xCC000000, zoom
            );
            guiDrawText(gfxm::vec2(grid_sides.min.x, y), MKSTR(y).c_str(), getFont(), 0, GUI_COL_WHITE, zoom);
        }

        if (control_points.size() > 3) {
            for (int i = 4; i < control_points.size() - 1; i += 3) {
                guiDrawBezierCurve(
                    control_points[i - 3], control_points[i - 2],
                    control_points[i - 1], control_points[i],
                    2.f, GUI_COL_RED, zoom
                );
            }
        }
        for (int i = 1; i < control_points.size(); i += 3) {
            guiDrawLine(control_points[i], control_points[i - 1], 4.f, 0x33FFFFFF, zoom);
            guiDrawLine(control_points[i], control_points[i + 1], 4.f, 0x33FFFFFF, zoom);
            guiDrawDiamond(control_points[i - 1], 2.5f, 0xFFAAAAAA, zoom);
            guiDrawDiamond(control_points[i + 1], 2.5f, 0xFFAAAAAA, zoom);
            
            guiDrawDiamond(control_points[i], 5.f, hovered_node == i ? GUI_COL_WHITE : GUI_COL_YELLOW, zoom);
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
                2.f, GUI_COL_YELLOW, zoom
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
        splitter->onHitTest(hit, x, y);
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
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        rc_bounds = rc;
        client_area = rc_bounds;

        splitter->layout(client_area, flags);
    }
    void onDraw() override {
        splitter->draw();
    }
};


