#pragma once

#include "gui/elements/gui_element.hpp"
#include "gui/elements/gui_container.hpp"
#include "gui/elements/gui_scroll_bar.hpp"
#include "gui/elements/gui_menu_bar.hpp"
#include "gui/gui_system.hpp"


enum GUI_COMPONENT_LAYOUT {
    GUI_COMP_LAYOUT_FILL,
    GUI_COMP_LAYOUT_LEFT,
    GUI_COMP_LAYOUT_RIGHT,
    GUI_COMP_LAYOUT_TOP,
    GUI_COMP_LAYOUT_BOTTOM
};
class GuiModular;
class GuiComponent : public GuiElement {
    friend GuiModular;
    GUI_COMPONENT_LAYOUT comp_layout = GUI_COMP_LAYOUT_TOP;
    gfxm::rect comp_padding = gfxm::rect(.0f, .0f, .0f, .0f);
public:
    void setComponentLayout(GUI_COMPONENT_LAYOUT l) { comp_layout = l; }
    GUI_COMPONENT_LAYOUT getComponentLayout() const { return comp_layout; }
    void setComponentPadding(float left, float top, float right, float bottom) { comp_padding = gfxm::rect(left, top, right, bottom); }
    const gfxm::rect& getComponentPadding() const { return comp_padding; }
};
class GuiModular : public GuiElement {
    std::vector<GuiComponent*> components;

    void layoutComponents() {
        // TODO: Sort components
        for (int i = 0; i < components.size(); ++i) {
            auto c = components[i];
            GUI_COMPONENT_LAYOUT clayout = c->getComponentLayout();
            switch (clayout) {
            case GUI_COMP_LAYOUT_FILL:
                c->layout(client_area, 0);
                break;
            case GUI_COMP_LAYOUT_LEFT: {
                gfxm::rect rc_ = client_area;
                rc_.max.x = rc_.min.x + c->size.x;
                c->layout(rc_, 0);
                break;
            }
            case GUI_COMP_LAYOUT_RIGHT: {
                gfxm::rect rc_ = client_area;
                rc_.min.x = rc_.max.x - c->size.x;
                c->layout(rc_, 0);
                break;
            }
            case GUI_COMP_LAYOUT_TOP: {
                gfxm::rect rc_ = client_area;
                rc_.max.y = rc_.min.y + c->size.y;
                c->layout(rc_, 0);
                break;
            }
            case GUI_COMP_LAYOUT_BOTTOM: {
                gfxm::rect rc_ = client_area;
                rc_.min.y = rc_.max.y - c->size.y;
                c->layout(rc_, 0);
                break;
            }
            }
            const gfxm::rect& pad = c->getComponentPadding();
            client_area.min += pad.min;
            client_area.max -= pad.max;
        }
    }
    void drawComponents() {
        for (int i = 0; i < components.size(); ++i) {
            auto c = components[i];
            c->draw();
        }
    }
public:
    ~GuiModular() {
        for (auto& c : components) {
            delete c;
        }
    }
    template<typename COMP_T>
    COMP_T* addComponent() {
        COMP_T* ptr = new COMP_T;
        components.push_back(ptr);
        ptr->setOwner(this);
        ptr->setParent(this);
        return ptr;
    }
    void removeComponent(GuiComponent* c) {
        for (int i = 0; i < components.size(); ++i) {
            if (components[i] == c) {
                components.erase(components.begin() + i);
                delete c;
                break;
            }
        }
    }

    GuiHitResult onHitTest(int x, int y) override {
        const gfxm::vec2 p(x, y);
        if (!gfxm::point_in_rect(rc_bounds, p)) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }

        if (gfxm::point_in_rect(client_area, p)) {
            for (int i = 0; i < children.size(); ++i) {
                GuiHitResult hit = children[i]->onHitTest(x, y);
                if (hit.hasHit()) {
                    return hit;
                }
            }
        } else {
            for (int i = 0; i < components.size(); ++i) {
                GuiHitResult hit = components[i]->onHitTest(x, y);
                if (hit.hasHit()) {
                    return hit;
                }
            }
        }

        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::MOUSE_SCROLL:
            pos_content.y = gfxm::_min(gfxm::_max(.0f, rc_content.max.y - (client_area.max.y - client_area.min.y)), gfxm::_max(rc_content.min.y, pos_content.y - params.getA<int32_t>()));
            break;
        case GUI_MSG::NOTIFY:
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::SCROLL_V:
                pos_content.y = params.getB<float>();
                return true;
            case GUI_NOTIFY::SCROLL_H:
                pos_content.x = params.getB<float>();
                return true;
            }
            break;
        }
        return false;
    }

    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        rc_bounds = rc;
        client_area = rc_bounds;

        layoutComponents();
        layoutContentTopDown(client_area);
    }
    void onDraw() override {
        drawComponents();
        drawContent();
    }
};

class GuiWindowTitleBarComponent : public GuiComponent {
    GuiTextBuffer title;
public:
    uint32_t color = GUI_COL_HEADER;
    GuiWindowTitleBarComponent()
    : title(guiGetDefaultFont()) {
        setSize(0, 25);
        title.replaceAll("Unnamed", strlen("Unnamed"));
        setComponentLayout(GUI_COMP_LAYOUT_TOP);
        setComponentPadding(0, 25, 0, 0);
    }
    void setCaption(const char* cap) {
        title.replaceAll(cap, strlen(cap));
    }
    GuiHitResult onHitTest(int x, int y) override {
        if (!gfxm::point_in_rect(rc_bounds, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        return GuiHitResult{ GUI_HIT::CAPTION, this };
    }
    void onDraw() override {
        guiDrawRect(rc_bounds, color);
        title.draw(rc_bounds, GUI_LEFT | GUI_VCENTER, GUI_COL_TEXT, 0);
    }
};
class GuiScrollComponent : public GuiComponent {
    GuiScrollBarV scroll_v;
    GuiScrollBarH scroll_h;
    bool v_enabled = false;
    bool h_enabled = false;
public:
    GuiScrollComponent() {
        setSize(10.f, 10.f);
        setComponentLayout(GUI_COMP_LAYOUT_FILL);
        setComponentPadding(0, 0, 10, 10);
        scroll_v.setOwner(this);
        scroll_h.setOwner(this);
    }
    GuiHitResult onHitTest(int x, int y) override {
        GuiHitResult hit = scroll_v.onHitTest(x, y);
        if (hit.hasHit()) {
            return hit;
        }
        return scroll_h.onHitTest(x, y);
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        auto& rc_content = getOwner()->getLocalContentRect();
        auto& rc_offset = getOwner()->getLocalContentOffset();
        auto& rc_client = getOwner()->getClientArea();

        scroll_v.setScrollBounds(rc_content.min.y, rc_content.max.y);
        scroll_v.setScrollPageLength(rc_client.max.y - rc_client.min.y);
        scroll_v.setScrollPosition(rc_offset.y);

        scroll_h.setScrollBounds(rc_content.min.x, rc_content.max.x);
        scroll_h.setScrollPageLength(rc_client.max.x - rc_client.min.x);
        scroll_h.setScrollPosition(rc_offset.x);

        v_enabled = rc_content.max.y - rc_content.min.y > rc_client.max.y - rc_client.min.y;
        h_enabled = rc_content.max.x - rc_content.min.x > rc_client.max.x - rc_client.min.x;

        if (v_enabled) {
            scroll_v.layout(rc, flags);
        }
        if (h_enabled) {
            scroll_h.layout(rc, flags);
        }
        setComponentPadding(.0f, .0f, (v_enabled ? 10.f : .0f), (h_enabled ? 10.f : .0f));
    }
    void onDraw() override {
        if (v_enabled) {
            scroll_v.draw();
        }
        if (h_enabled) {
            scroll_h.draw();
        }
    }
    virtual void carveParentArea(gfxm::rect* rc) {
        if (v_enabled) {
            rc->max.x -= size.x;
        }
        if (h_enabled) {
            rc->max.y -= size.y;
        }
    }
};


#include "gui_window_title_bar_button.hpp"

enum GUI_WINDOW_FRAME_STYLE {
    GUI_WINDOW_FRAME_NONE,
    GUI_WINDOW_FRAME_UNSPECIFIED,
    GUI_WINDOW_FRAME_FULL
};

class GuiWindow : public GuiElement {
    void* dock_group = 0;
    uint64_t flags_cached = 0;
    const int titlebar_width = 25.f;

    std::string title;
    GuiTextBuffer title_buf;
    GuiWindowTitleBarButton close_btn = GuiWindowTitleBarButton(guiLoadIcon("svg/entypo/cross.svg"), GUI_MSG::CLOSE);;
    gfxm::rect rc_titlebar;
    gfxm::rect icon_rc;

    std::unique_ptr<GuiMenuBar> menu_bar;
    std::unique_ptr<GuiScrollBarV> scroll_v;
    std::unique_ptr<GuiScrollBarH> scroll_h;

    GUI_DOCK dock_position = GUI_DOCK::NONE;
    bool is_dockable = true;

    char getResizeBorderMask() const {
        char sz_mask = 0b1111;
        if (getDockPosition() == GUI_DOCK::FILL) {
            sz_mask = 0b0000;
        } else if(getDockPosition() == GUI_DOCK::LEFT) {
            sz_mask = 0b0010;
        } else if(getDockPosition() == GUI_DOCK::RIGHT) {
            sz_mask = 0b0001;
        } else if(getDockPosition() == GUI_DOCK::TOP) {
            sz_mask = 0b1000;
        } else if(getDockPosition() == GUI_DOCK::BOTTOM) {
            sz_mask = 0b0100;
        }
        return sz_mask;
    }
    void calcResizeBorders(const gfxm::rect& rect, float thickness_outer, float thickness_inner, gfxm::rect* left, gfxm::rect* right, gfxm::rect* top, gfxm::rect* bottom) {
        assert(left && right && top && bottom);

        left->min = gfxm::vec2(
            rect.min.x - thickness_outer,
            rect.min.y - thickness_outer
        );
        left->max = gfxm::vec2(
            rect.min.x + thickness_inner,
            rect.max.y + thickness_outer
        );

        right->min = gfxm::vec2(
            rect.max.x - thickness_inner,
            rect.min.y - thickness_outer
        );
        right->max = gfxm::vec2(
            rect.max.x + thickness_outer,
            rect.max.y + thickness_outer
        );

        top->min = gfxm::vec2(
            rect.min.x - thickness_outer,
            rect.min.y - thickness_outer
        );
        top->max = gfxm::vec2(
            rect.max.x + thickness_outer,
            rect.min.y + thickness_inner
        );

        bottom->min = gfxm::vec2(
            rect.min.x - thickness_outer,
            rect.max.y - thickness_inner
        );
        bottom->max = gfxm::vec2(
            rect.max.x + thickness_outer,
            rect.max.y + thickness_outer
        );
    }
    GuiHitResult hitTestResizeBorders(const gfxm::rect& rc, float border_thickness, int x, int y, char mask) {
        gfxm::vec2 pt(x, y);
        gfxm::rect rc_szleft, rc_szright, rc_sztop, rc_szbottom;
        calcResizeBorders(rc, border_thickness * .5f, border_thickness * .5f, &rc_szleft, &rc_szright, &rc_sztop, &rc_szbottom);
        char sz_flags = 0b0000;
        if (gfxm::point_in_rect(rc_szleft, pt)) {
            sz_flags |= 0b0001;
        } else if (gfxm::point_in_rect(rc_szright, pt)) {
            sz_flags |= 0b0010;
        }
        if (gfxm::point_in_rect(rc_sztop, pt)) {
            sz_flags |= 0b0100;
        } else if (gfxm::point_in_rect(rc_szbottom, pt)) {
            sz_flags |= 0b1000;
        }
        sz_flags &= mask;
        GUI_HIT ht = GUI_HIT::ERR;
        switch (sz_flags) {
        case 0b0001: ht = GUI_HIT::LEFT; break;
        case 0b0010: ht = GUI_HIT::RIGHT; break;
        case 0b0100: ht = GUI_HIT::TOP; break;
        case 0b1000: ht = GUI_HIT::BOTTOM; break;
        case 0b0101: ht = GUI_HIT::TOPLEFT; break;
        case 0b1001: ht = GUI_HIT::BOTTOMLEFT; break;
        case 0b0110: ht = GUI_HIT::TOPRIGHT; break;
        case 0b1010: ht = GUI_HIT::BOTTOMRIGHT; break;
        };
        if (ht != GUI_HIT::ERR) {
            return GuiHitResult{ ht, this };
        }
        return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
    }
    GuiHitResult hitTestTitleBar(const gfxm::rect& rc, int x, int y) {
        gfxm::rect rc_ = rc;
        rc_.max.y = rc_.min.y + titlebar_width;
        if (gfxm::point_in_rect(rc_, gfxm::vec2(x, y))) {
            auto hit = close_btn.onHitTest(x, y);
            if (hit.hasHit()) {
                return hit;
            }
            return GuiHitResult{ GUI_HIT::CAPTION, this };
        }
        return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
    }
    GuiHitResult hitTestFrame(const gfxm::rect& rc, int x, int y) {
        if ((flags_cached & GUI_LAYOUT_NO_BORDER) == 0) {
            GuiHitResult hit = hitTestResizeBorders(rc, 10.f, x, y, 0b1111);
            if (hit.hasHit()) {
                return hit;
            }
        }
        if ((flags_cached & GUI_LAYOUT_NO_BORDER) == 0) {
            GuiHitResult hit = hitTestTitleBar(rc_bounds, x, y);
            if (hit.hasHit()) {
                return hit;
            }
        }
        if (menu_bar) {
            GuiHitResult hit = menu_bar->onHitTest(x, y);
            if (hit.hasHit()) {
                return hit;
            }
        }
        if (scroll_v && getContentHeight() > getClientHeight()) {
            GuiHitResult hit = scroll_v->onHitTest(x, y);
            if (hit.hasHit()) {
                return hit;
            }
        }
        if (scroll_h && getContentWidth() > getClientWidth()) {
            GuiHitResult hit = scroll_h->onHitTest(x, y);
            if (hit.hasHit()) {
                return hit;
            }
        }
        return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
    }
public:
    GuiWindow(const char* title = "MyWindow");
    ~GuiWindow();

    void setDockGroup(void* dock_group) { this->dock_group = dock_group; }
    void* getDockGroup() const { return dock_group; }

    void setTitle(const std::string& title) {
        this->title = title;
        title_buf.replaceAll(title.data(), title.size());
    }
    const std::string& getTitle() {
        return title;
    }

    void sendCloseMessage() {
        sendMessage(GUI_MSG::CLOSE, 0, 0, 0);
    }

    GuiMenuBar* createMenuBar() {
        if (menu_bar) {
            return menu_bar.get();
        } else {
            menu_bar.reset(new GuiMenuBar);
            menu_bar->setOwner(this);
            return menu_bar.get();
        }
    }

    GuiHitResult onHitTest(int x, int y) override {
        gfxm::rect rc_padded(rc_bounds.min - gfxm::vec2(5.f, 5.f), rc_bounds.max + gfxm::vec2(5.f, 5.f));
        if (!gfxm::point_in_rect(rc_padded, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        GuiHitResult hit = hitTestFrame(rc_bounds, x, y);
        if (hit.hasHit()) {
            return hit;
        }
        
        if (gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            for (auto& ch : children) {
                GuiHitResult hit = ch->onHitTest(x, y);
                if (hit.hasHit()) {
                    return hit;
                }
            }
        }

        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::CLOSE: {
            guiDestroyWindow(this);
            return true;
        }
        case GUI_MSG::MOUSE_SCROLL:
            //pos_content.y = gfxm::_min(gfxm::_max(.0f, rc_content.max.y - (client_area.max.y - client_area.min.y)), gfxm::_max(rc_content.min.y, pos_content.y - params.getA<int32_t>()));
            pos_content.y 
                = gfxm::_max(
                    .0f, 
                    gfxm::_min(pos_content.y - params.getA<int32_t>(), (float)(getContentHeight() - getClientHeight()))
                );
            break;
        case GUI_MSG::NOTIFY:
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::SCROLL_V:
                pos_content.y = params.getB<float>();
                return true;
            case GUI_NOTIFY::SCROLL_H:
                pos_content.x = params.getB<float>();
                return true;
            }
            break;
        case GUI_MSG::ACTIVATE:
            //title_bar->color = GUI_COL_ACCENT;
            return true;
        case GUI_MSG::DEACTIVATE:
            //title_bar->color = GUI_COL_HEADER;
            return true;
        case GUI_MSG::MOVING: {
            gfxm::rect* prc = params.getB<gfxm::rect*>();
            pos += prc->max - prc->min;
            } return true;
        case GUI_MSG::RESIZING: {
            gfxm::rect* prc = params.getB<gfxm::rect*>();
            switch (params.getA<GUI_HIT>()) {
            case GUI_HIT::LEFT:
                pos.x = prc->max.x;
                size.x -= prc->max.x - prc->min.x;
                break;
            case GUI_HIT::RIGHT:
                size.x += prc->max.x - prc->min.x;
                break;
            case GUI_HIT::TOP:
                pos.y = prc->max.y;
                size.y -= prc->max.y - prc->min.y;
                break;
            case GUI_HIT::BOTTOM:
                size.y += prc->max.y - prc->min.y;
                break;
            case GUI_HIT::TOPLEFT:
                pos.y = prc->max.y;
                size.y -= prc->max.y - prc->min.y;
                pos.x = prc->max.x;
                size.x -= prc->max.x - prc->min.x;
                break;
            case GUI_HIT::TOPRIGHT:
                pos.y = prc->max.y;
                size.y -= prc->max.y - prc->min.y;
                size.x += prc->max.x - prc->min.x;
                break;
            case GUI_HIT::BOTTOMLEFT:
                size.y += prc->max.y - prc->min.y;
                pos.x = prc->max.x;
                size.x -= prc->max.x - prc->min.x;
                break;
            case GUI_HIT::BOTTOMRIGHT:
                size.y += prc->max.y - prc->min.y;
                size.x += prc->max.x - prc->min.x;
                break;
            }
            size = gfxm::vec2(
                std::max(min_size.x, std::min(max_size.x, size.x)),
                std::max(min_size.y, std::min(max_size.y, size.y))
            );
        } return true;
        }

        return GuiElement::onMessage(msg, params);
    }

    void layout(const gfxm::rect& rc, uint64_t flags) {
        if (is_hidden) {
            return;
        }
        flags_cached = flags;

        if (getFont()) { guiPushFont(getFont()); }
        onLayoutFrame(rc, flags);
        onLayout(rc, flags);
        if (getFont()) { guiPopFont(); }
    }
    void draw() {
        if (is_hidden) {
            return;
        }
        if (getFont()) { guiPushFont(getFont()); }
        onDrawFrame();
        onDraw();
        if (getFont()) { guiPopFont(); }
    }

    void onLayoutFrame(const gfxm::rect& rc, uint64_t flags) {
        rc_bounds = rc;
        client_area = rc_bounds;

        if ((flags & GUI_LAYOUT_NO_TITLE) == 0) {
            client_area.min.y += titlebar_width;
        }
        if (menu_bar) {
            menu_bar->layout(client_area, 0);
            auto menu_rc = menu_bar->getBoundingRect();
            client_area.min.y += menu_rc.max.y - menu_rc.min.y;
        }

        client_area.min += content_padding.min;
        client_area.max -= content_padding.max;
        
        if (scroll_v && (getFlags() & GUI_FLAG_SCROLLV)) {
            scroll_v->setScrollBounds(0, getContentHeight());
            scroll_v->setScrollPageLength(getClientHeight());
            scroll_v->setScrollPosition(pos_content.y);
            scroll_v->layout(client_area, 0);
            auto scroll_rc = scroll_v->getBoundingRect();
            client_area.max.x -= scroll_rc.max.x - scroll_rc.min.x;
        }
        if (scroll_h && (getFlags() & GUI_FLAG_SCROLLH)) {
            scroll_h->setScrollBounds(0, getContentWidth());
            scroll_h->setScrollPageLength(getClientWidth());
            scroll_h->setScrollPosition(pos_content.x);
            scroll_h->layout(client_area, 0);
            auto scroll_rc = scroll_h->getBoundingRect();
            client_area.max.y -= scroll_rc.max.y - scroll_rc.min.y;
        }


        if ((flags_cached & GUI_LAYOUT_NO_TITLE) == 0) {
            rc_titlebar = rc_bounds;
            rc_titlebar.max.y = rc_titlebar.min.y + titlebar_width;

            float icon_sz = rc_titlebar.max.y - rc_titlebar.min.y;
            icon_rc = gfxm::rect(
                rc_titlebar.max - gfxm::vec2(icon_sz, icon_sz),
                rc_titlebar.max
            );
            close_btn.layout(icon_rc, 0);
        }
    }
    void onDrawFrame() {
        guiDrawRect(rc_bounds, GUI_COL_BG);
        if (guiGetActiveWindow() == this) {
            guiDrawRectLine(rc_bounds, GUI_COL_BUTTON_HOVER);
        } else {
            guiDrawRectLine(rc_bounds, GUI_COL_HEADER);
        }

        if ((flags_cached & GUI_LAYOUT_NO_TITLE) == 0) {
            if (guiGetActiveWindow() == this) {
                guiDrawRectGradient(rc_titlebar, GUI_COL_ACCENT, GUI_COL_ACCENT_DIM, GUI_COL_ACCENT, GUI_COL_ACCENT_DIM);
            } else {
                guiDrawRect(rc_titlebar, GUI_COL_HEADER);
            }
            gfxm::rect rc = rc_titlebar;
            rc.min.x += GUI_MARGIN;
            title_buf.draw(rc, GUI_LEFT | GUI_VCENTER, GUI_COL_TEXT, GUI_COL_HEADER);
            
            // Draw close button
            close_btn.draw();
            
        }
        if (menu_bar) {
            menu_bar->draw();
        }
        if (scroll_v && (getFlags() & GUI_FLAG_SCROLLV)) {
            scroll_v->draw();
        }
        if (scroll_h && (getFlags() & GUI_FLAG_SCROLLH)) {
            scroll_h->draw();
        }
        //guiDrawRectLine(client_area, GUI_COL_GREEN);
        //guiDrawRectLine(rc_content, GUI_COL_RED);
    }

    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        gfxm::rect rc_ = client_area;
        layoutContentTopDown(rc_);
    }
    void onDraw() override {
        if (client_area.min.x >= client_area.max.x || client_area.min.y >= client_area.max.y) {
            return;
        }
        guiDrawPushScissorRect(client_area);
        drawContent();
        guiDrawPopScissorRect();
    }

    GUI_DOCK getDockPosition() const override {
        return dock_position;
    }
    void setDockPosition(GUI_DOCK dock) override {
        dock_position = dock;
    }

    bool isDockable() const {
        return is_dockable;
    }
    virtual void setDockable(bool is_dockable) {
        this->is_dockable = is_dockable;
    }
};