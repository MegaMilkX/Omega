#pragma once

#include <functional>
#include <vector>
#include "xui/types.hpp"
#include "gui/types.hpp"


namespace xui {


    struct BoxLayout {
        struct Box {
            void* user_ptr = nullptr;
            int px_line_height = 0; // from font
            gui_vec2 size = gui::px(100, 100);
            gui_vec2 min_size = gui::px(0, 0);
            gui_vec2 max_size = gui::px(INT_MAX, INT_MAX);
            uint32_t color = 0xFFFFFFFF;
            bool same_line = false;
        };

        struct BOX {
            Box* elem;
            //Font* font;
            int index;
            gui_float width;
            gui_float height;
            gui_float overflow_width;
            gui_float overflow_height;
            gui_float min_width;
            gui_float min_height;
            gui_float max_width;
            gui_float max_height;
            bool early_layout;

            int px_pos_x = 0;
            int px_pos_y = 0;

            uint32_t color;
        };
        struct LINE {
            gui_float height;
            int begin;
            int end;
            std::vector<BOX> boxes;
        };
        std::vector<LINE> lines;

        gui_rect padding;
        gui_vec2 content_margin;
        int m_px_line_height = 0;
        int m_px_container_width = 0;
        int m_px_container_height = 0;
        bool m_fit_content_width = false;
        bool m_fit_content_height = false;

        void buildWidth(Box* boxes, int count, std::function<int(const BOX*)> on_measure_width);
        void buildHeight(std::function<int(const BOX*)> on_measure_height);
        void buildPosition();
    };


}