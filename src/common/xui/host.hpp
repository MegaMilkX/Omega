#pragma once

#include <memory>
#include "xui/renderer/renderer.hpp"
#include "xui/elements/root.hpp"
#include "gui/style/style_component.hpp"


namespace xui {
    

    class Host {
        std::shared_ptr<Font> default_font;
        std::unique_ptr<Root> root;
        gui::style_sheet style_sheet;
        Element* elem_hovered = nullptr;
    public:
        Host();
        ~Host();

        void layout(int width, int height);
        void hitTest(int x, int y);
        void draw(IRenderer*);
        void render(IRenderer*, int width, int height, bool clear);

        bool isHovered(Element*) const;

        gui::style_sheet& getStyleSheet();
        Font* getDefaultFont();
        Font* resolveFont(Element*);
    };


}