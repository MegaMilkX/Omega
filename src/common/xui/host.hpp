#pragma once

#include <memory>
#include "xui/renderer/renderer.hpp"
#include "xui/elements/root.hpp"
#include "gui/style/style_component.hpp"


namespace xui {
    
    enum class InputState {
        Idle,
        Resize,
        Drag
    };

    class Host {
        std::shared_ptr<Font> default_font;
        std::unique_ptr<Root> root;
        gui::style_sheet style_sheet;

        InputState inp_state = InputState::Idle;

        gfxm::ivec2 mouse_pos;
        gfxm::ivec2 mouse_delta;
        Element* elem_mouse_captor = nullptr;
        Element* elem_hovered = nullptr; // TODO: deleted elements leave this dangling
        Element* elem_pressed = nullptr;
        Element* elem_resized = nullptr;
        gfxm::ivec2 resized_initial_pos;
        gfxm::ivec2 resized_initial_sz;
        gfxm::ivec2 resized_initial_mouse_pos;
        HIT hit_hovered = HIT::NOWHERE;
        HIT hit_pressed = HIT::NOWHERE;

        std::vector<Element*> managed_elements;

        void updateHovered(Element*, HIT);
        void updatePressed(Element*, HIT);

        void bringToTop(Element*);

        void enterIdleState();
        void enterDragState(Element*);
        void enterResizeState(Element*);

        void idle_mouseMove();
        void drag_mouseMove();
        void resize_mouseMove();
    public:
        Host();
        ~Host();

        template<typename ELEM_T, typename... ARGS>
        ELEM_T* create(ARGS&&... args);

        void captureMouse(Element* elem);
        void releaseMouseCapture(Element* elem);

        void mouseMove(int x, int y);
        void mouseEvent(MouseButton btn, KeyEvent evt, int value = 0);

        void layout(int width, int height);
        void hitTest(int x, int y);
        void draw(IRenderer*);
        void render(IRenderer*, int width, int height, bool clear);

        bool isHovered(Element*) const;

        gui::style_sheet& getStyleSheet();
        Font* getDefaultFont();
        Font* resolveFont(Element*);
    };

    template<typename ELEM_T, typename... ARGS>
    ELEM_T* Host::create(ARGS&&... args) {
        auto e = new ELEM_T(std::forward<ARGS>(args)...);
        managed_elements.push_back(e);
        return e;
    }

}

