#include "root.hpp"
#include "gui/elements/menu_bar.hpp"
#include "gui/elements/dock_space.hpp"


GuiRoot::GuiRoot() {
    background_texture = resGet<gpuTexture2d>("images/wallhaven-xee3wz_1920x1080.png");

    setStyleClasses({ "root" });
    addFlags(GUI_FLAG_NO_HIT);

    menu_box = new GuiElement();
    menu_box->setSize(gui::fill(), gui::content());
    _addChild(menu_box);
    /*
    content_box = new GuiElement();
    content_box->setSize(gui::fill(), gui::fill());
    content_box->addFlags(GUI_FLAG_NO_HIT);
    pushBack(content_box);

    content = content_box;
    */

    window_layer.reset(new GuiWindowLayer());
    window_layer->setSize(gui::fill(), gui::fill());
    _addChild(window_layer.get());
    //content = window_layer.get();

    popup_layer.reset(new GuiPopupLayer());
    popup_layer->setSize(gui::fill(), gui::fill());
    _addChild(popup_layer.get());

    /*
    auto title_bar = new GuiTitleBar();
    title_bar->addFlags(GUI_FLAG_PERSISTENT | GUI_FLAG_FRAME);
    pushBack(title_bar);*/
}

GuiMenuBar* GuiRoot::getMenuBar() {
    if (menu_bar) {
        return menu_bar.get();
    } else {
        menu_bar.reset(new GuiMenuBar);
        menu_bar->addFlags(GUI_FLAG_PERSISTENT);
        menu_box->pushBack(menu_bar.get());
        return menu_bar.get();
    }
}

GuiDockSpace* GuiRoot::getDockSpace() {
    if (!dock_space) {
        dock_space.reset(new GuiDockSpace());
        dock_space->setSize(gui::fill(), gui::fill());
        _addChild(dock_space.get());
        dock_space->setOwner(this);
    }
    return dock_space.get();
}
GuiPopupLayer* GuiRoot::getPopupLayer() {
    return popup_layer.get();
}

void GuiRoot::onHitTest(GuiHitResult& hit, int x, int y) {
    if (!gfxm::point_in_rect(rc_bounds, gfxm::vec2(x, y))) {
        return;
    }

    if (popup_layer) {
        popup_layer->onHitTest(hit, x - popup_layer->layout_position.x, y - popup_layer->layout_position.y);
        if (hit.hasHit()) {
            return;
        }
    }

    if(menu_bar){
        menu_bar->onHitTest(hit, x, y);
        if (hit.hasHit()) {
            return;
        }
    }

    if (window_layer) {
        window_layer->onHitTest(hit, x - window_layer->layout_position.x, y - window_layer->layout_position.y);
        if (hit.hasHit()) {
            return;
        }
    }

    if (dock_space) {
        dock_space->onHitTest(hit, x - dock_space->layout_position.x, y - dock_space->layout_position.y);
        if (hit.hasHit()) {
            return;
        }
    }

    return;
}

bool GuiRoot::onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
    switch (msg) {
    case GUI_MSG::NOTIFY:
        switch (params.getA<GUI_NOTIFY>()) {
        case GUI_NOTIFY::UNDOCKED:
            //window_layer->pushBack(params.getB<GuiElement*>());
            window_layer->pushBackInDragState(params.getB<GuiElement*>());
            return true;
        }
        break;
    }
    return GuiElement::onMessage(msg, params);
}

void GuiRoot::onLayout(const gfxm::vec2& extents, uint64_t flags) {
    rc_bounds = gfxm::rect(gfxm::vec2(0, 0), extents);
    client_area = rc_bounds;

    gfxm::rect rc = client_area;

    if (menu_bar) {
        menu_bar->layout(rc.size(), flags);
        rc.min.y += menu_bar->getBoundingRect().size().y;
    }

    if (dock_space) {
        dock_space->layout_position.y = rc.min.y;
        dock_space->layout(rc.size(), flags);
    }
    if (window_layer) {
        window_layer->layout_position.y = rc.min.y;
        window_layer->layout(rc.size(), flags);
    }
    if (popup_layer) {
        popup_layer->layout_position = layout_position;
        popup_layer->layout(client_area.size(), flags);
    }
}


void GuiRoot::onDraw() {
    //guiDrawRectTextured(rc_bounds, background_texture.get(), 0xFFFFFFFF);

    if (dock_space) {
        dock_space->draw();
    }

    if (window_layer) {
        window_layer->draw();
    }

    if(menu_bar) {
        menu_bar->draw();
    }

    if (popup_layer) {
        popup_layer->draw();
    }
}


void GuiRoot::addChild(GuiElement* elem) {
    window_layer->addChild(elem);
}

void GuiRoot::removeChild(GuiElement* elem) {
    if(window_layer) {
        window_layer->removeChild(elem);
    }
}

