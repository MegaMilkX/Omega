#pragma once

#include <functional>
#include <vector>
#include "xui/types.hpp"


namespace xui {


    struct BoxLayout {
        struct Box {
            void* user_ptr = nullptr;
            int px_line_height = 0; // from font
            xvec2 size = px(100, 100);
            xvec2 min_size = px(0, 0);
            xvec2 max_size = px(INT_MAX, INT_MAX);
            uint32_t color = 0xFFFFFFFF;
            bool same_line = false;
        };

        struct BOX {
            Box* elem;
            //Font* font;
            int index;
            xfloat width;
            xfloat height;
            xfloat overflow_width;
            xfloat overflow_height;
            xfloat min_width;
            xfloat min_height;
            xfloat max_width;
            xfloat max_height;
            bool early_layout;

            int px_pos_x = 0;
            int px_pos_y = 0;

            uint32_t color;
        };
        struct LINE {
            xfloat height;
            int begin;
            int end;
            std::vector<BOX> boxes;
        };
        std::vector<LINE> lines;

        xvec2 content_margin;
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