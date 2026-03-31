#include "window_decorator.hpp"


namespace xui {


    WindowDecorator::WindowDecorator()
    : title("Window") {
        addToLayout(&title);
        title.size = xvec2(fill(), em(1));
        title.hit_type = HIT::CAPTION;
        addToLayout(&content);
        content.size = xvec2(fill(), fill());
    }

    void WindowDecorator::onDraw(IRenderer* r) {
        r->drawRectRound(gfxm::rect(0, 0, px_size.x, px_size.y), 0xFF333333, 10, 10, 10, 10);
        r->drawRectRoundBorder(gfxm::rect(0, 0, px_size.x, px_size.y), 0xFF557711, 10, 10, 10, 10, 3, 3, 3, 3);
        StackElement::onDraw(r);
    }

    void WindowDecorator::setContent(Element* e) {
        content.addToLayout(e);
    }

}