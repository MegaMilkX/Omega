#include "box_layout.hpp"

#include <algorithm>


namespace xui {


    void BoxLayout::buildWidth(Box* in_boxes, int box_count, std::function<int(const BOX*)> on_measure_width) {
        int px_content_margin_x = to_px(content_margin.x, m_px_line_height, m_px_container_width);
        //gfxm::vec2 px_content_margin = to_px(content_margin, m_px_line_height, gfxm::vec2(m_px_container_width, m_px_container_height));
        //gfxm::rect px_padding = gfxm::rect(10, 10, 10, 10);
        //gfxm::rect px_border = gfxm::rect(0, 0, 0, 0);

        // ================================
        // Width stage, lines are generated
        // ================================
        lines.clear();

        int i = 0;
        while (i < box_count) {
            int line_begin = i;
            int j = i + 1;
            for (; j < box_count; ++j) {
                const Box& ch = in_boxes[j];
                if (!ch.same_line) {
                    break;
                }
            }
            i = j;
            int line_end = i;

            LINE line = { 0 };
            std::vector<BOX>& boxes = line.boxes;
            boxes.reserve(line_end - line_begin);
            for (int j = line_begin; j < line_end; ++j) {
                int li = j - line_begin;
                Box& ch = in_boxes[j];

                /*if (ch->isHidden()) {
                    continue;
                }*/

                xfloat width = float_convert(ch.size.x, ch.px_line_height, m_px_container_width);
                xfloat min_width = float_convert(ch.min_size.x, ch.px_line_height, m_px_container_width);
                xfloat max_width = float_convert(ch.max_size.x, ch.px_line_height, m_px_container_width);

                BOX box = { 0 };
                box.overflow_width = px(0);
                box.overflow_height = px(0);
                box.elem = &ch;
                box.index = j;
                box.width = width;
                box.min_width = min_width;
                box.max_width = max_width;
                box.early_layout = false;

                box.color = ch.color;

                boxes.push_back(box);
            }
                
            // Early layout for horizontal content sizing
            for (int j = 0; j < boxes.size(); ++j) {
                BOX& box = boxes[j];
                if (box.width.unit != u_content
                    && box.min_width.unit != u_content
                    && box.max_width.unit != u_content) {
                    continue;
                }

                // TODO: Do the early horizontal layout
                /*GuiElement* ch = boxes[j].elem;
                ch->layout(
                    gfxm::vec2(0, 0),
                    GUI_LAYOUT_WIDTH_PASS
                    | GUI_LAYOUT_FIT_CONTENT
                );*/
                if (!on_measure_width) {
                    assert(false);
                    return;
                }
                int px_box_content_width = on_measure_width(&box);
                
                box.early_layout = true;
                if(box.width.unit == u_content) {
                    box.width.unit = u_pixel;
                    box.width.value = px_box_content_width;//ch->rc_bounds.max.x - ch->rc_bounds.min.x;
                }
                if(box.min_width.unit == u_content) {
                    box.min_width.unit = u_pixel;
                    box.min_width.value = px_box_content_width;//ch->rc_bounds.max.x - ch->rc_bounds.min.x;
                }
                if(box.max_width.unit == u_content) {
                    box.max_width.unit = u_pixel;
                    box.max_width.value = px_box_content_width;//ch->rc_bounds.max.x - ch->rc_bounds.min.x;
                }
            }

            // Adjust width between min and max for pixel values
            for (int j = 0; j < boxes.size(); ++j) {
                BOX& box = boxes[j];
                if (box.width.unit == u_fill) {
                    continue;
                }

                if (box.max_width.unit == u_pixel) {
                    box.width.value = gfxm::_min(box.max_width.value, box.width.value);
                }

                if (box.min_width.unit == u_pixel) {
                    box.width.value = gfxm::_max(box.min_width.value, box.width.value);
                }
            }

            // Inflate client_width to content
            if (m_fit_content_width) {
                int total_width_no_margins = 0;
                for (int j = 0; j < boxes.size(); ++j) {
                    BOX& box = boxes[j];
                    if (box.width.unit == u_fill) {
                        continue;
                    }
                    total_width_no_margins += box.width.value;
                }
                m_px_container_width = gfxm::_max(
                    m_px_container_width,
                    int(total_width_no_margins + px_content_margin_x * (boxes.size() - 1))
                );
            } else {
                if(boxes.size() > 0) {
                    int hori_advance = 0;
                    for (int j = 0; j < boxes.size(); ++j) {
                        if (boxes[j].width.unit == u_pixel) {
                            hori_advance += boxes[j].width.value;
                        }

                        if (hori_advance > m_px_container_width && j != 0) {
                            for (int k = j; k < boxes.size(); ++k) {
                                if (boxes[k].width.unit != u_pixel) {
                                    continue;
                                }
                            }

                            i = boxes[j].index;
                            line_end = i;
                            boxes.resize(j);
                            break;
                        }

                        hori_advance += px_content_margin_x;
                    }
                }
            }

            line.boxes = boxes;
            line.begin = line_begin;
            line.end = line_end;
            lines.push_back(line);
        }
            
        // Handle u_fill widths
        for (int j = 0; j < lines.size(); ++j) {
            LINE& line = lines[j];

            int n_w_fill = 0;
            int width_eaten = 0;
            int free_width = 0;
            for (int k = 0; k < line.boxes.size(); ++k) {
                BOX& box = line.boxes[k];
                if (box.width.unit == u_fill) {
                    ++n_w_fill;
                } else {
                    assert(box.width.unit == gui_pixel);
                    width_eaten += box.width.value;
                }
            }
            free_width = gfxm::_max(0, m_px_container_width - width_eaten);

            // Subtract margin space from free_width
            {
                int margin_space = int(px_content_margin_x * (line.boxes.size() - 1));
                width_eaten += margin_space;
                free_width = gfxm::_max(0, free_width - margin_space);
            }

            // Handle min width for fills
            {
                std::vector<BOX*> boxes_sorted(line.boxes.size());
                for (int k = 0; k < line.boxes.size(); ++k) {
                    boxes_sorted[k] = &line.boxes[k];
                }
                std::sort(boxes_sorted.begin(), boxes_sorted.end(), [](const BOX* a, const BOX* b) {
                    return a->min_width.value > b->min_width.value;
                });
                for (int k = 0; k < boxes_sorted.size(); ++k) {
                    BOX& box = *boxes_sorted[k];
                    if (box.width.unit != u_fill) {
                        continue;
                    }
                    if (box.min_width.unit == u_fill) {
                        continue;
                    }

                    if (box.min_width.value > free_width / n_w_fill) {
                        --n_w_fill;
                        free_width = gfxm::_max(.0f, free_width - box.min_width.value);
                        box.width = box.min_width;
                    }
                }
            }

            // Handle max width for fills
            {
                std::vector<BOX*> boxes_sorted(line.boxes.size());
                for (int k = 0; k < line.boxes.size(); ++k) {
                    boxes_sorted[k] = &line.boxes[k];
                }
                std::sort(boxes_sorted.begin(), boxes_sorted.end(), [](const BOX* a, const BOX* b) {
                    return a->max_width.value < b->max_width.value;
                });
                for (int k = 0; k < boxes_sorted.size(); ++k) {
                    BOX& box = *boxes_sorted[k];
                    if (box.width.unit != u_fill) {
                        continue;
                    }
                    if (box.max_width.unit == u_fill) {
                        continue;
                    }

                    if (box.max_width.value < free_width / n_w_fill) {
                        --n_w_fill;
                        free_width = gfxm::_max(.0f, free_width - box.max_width.value);
                        box.width = box.max_width;
                    }
                }
            }

            // Convert fills to pixels (width)
            for (int k = 0; k < line.boxes.size(); ++k) {
                if (line.boxes[k].width.unit != u_fill) {
                    continue;
                }
                line.boxes[k].width.unit = u_pixel;
                line.boxes[k].width.value = free_width / n_w_fill;
            }
        }

        // Call child width layouts so the heights could be figured out in phase 2
        /*for (int j = 0; j < lines.size(); ++j) {
            LINE& line = lines[j];
            for (int k = 0; k < line.boxes.size(); ++k) {
                BOX& box = line.boxes[k];
                if (box.early_layout) {
                    continue;
                }
                assert(box.width.unit == u_pixel);
                _measureWidth(box);
                
                float w = box.width.value;
                box.elem->layout(
                    gfxm::vec2(w, 0),
                    GUI_LAYOUT_WIDTH_PASS
                );
            }
        }*/

        // Find horizontal content extents
        int max_line_width = 0;
        for (int j = 0; j < lines.size(); ++j) {
            LINE& line = lines[j];
            int col_cur = 0;
            for (int k = 0; k < line.boxes.size(); ++k) {
                const BOX& box = line.boxes[k];

                int w = box.width.value;
                // Add width first
                col_cur += w;
                // Then calculate new line width
                max_line_width = gfxm::_max(max_line_width, col_cur);
                // Then apply margin before the next sibling
                col_cur += px_content_margin_x;
            }
        }
        /*
        rc_content.min.x = client_area.min.x - pos_content.x;
        rc_content.max.x = rc_content.min.x;
        if(lines.size() > 0) {
            rc_content.max.x = client_area.min.x + max_line_width - pos_content.x;
        }

        if (flags & GUI_LAYOUT_FIT_CONTENT) {
            const float sz_content = rc_content.max.x - rc_content.min.x;
            client_area.max.x = gfxm::_max(client_area.max.x, client_area.min.x + sz_content);
            rc_bounds.max.x = client_area.max.x + px_padding.max.x;
        }*/
    }

    void BoxLayout::buildHeight(std::function<int(const BOX*)> on_measure_height) {
        int px_content_margin_y = to_px(content_margin.y, m_px_line_height, m_px_container_height);

        // Find box heights
        for (int j = 0; j < lines.size(); ++j) {
            LINE& line = lines[j];
            for (int k = 0; k < line.boxes.size(); ++k) {
                BOX& box = line.boxes[k];

                Box* ch = box.elem;
                xfloat height = float_convert(ch->size.y, ch->px_line_height, m_px_container_height);
                xfloat min_height = float_convert(ch->min_size.y, ch->px_line_height, m_px_container_height);
                xfloat max_height = float_convert(ch->max_size.y, ch->px_line_height, m_px_container_height);

                box.height = height;
                box.min_height = min_height;
                box.max_height = max_height;
                box.early_layout = false;
            }
        }

        // Early layout for content based height
        for (int j = 0; j < lines.size(); ++j) {
            LINE& line = lines[j];
            for (int k = 0; k < line.boxes.size(); ++k) {
                BOX& box = line.boxes[k];
                Box* ch = box.elem;
                if (box.height.unit != u_content
                    && box.min_height.unit != u_content
                    && box.max_height.unit != u_content) {
                    continue;
                }

                // TODO: Do the early horizontal layout
                /*ch->layout(
                    gfxm::vec2(0, 0),
                    GUI_LAYOUT_HEIGHT_PASS
                    | GUI_LAYOUT_FIT_CONTENT
                );*/
                if (!on_measure_height) {
                    assert(false);
                    return;
                }
                int px_box_content_height = on_measure_height(&box);
                
                box.early_layout = true;
                if(box.height.unit == u_content) {
                    box.height.unit = u_pixel;
                    box.height.value = px_box_content_height;//ch->rc_bounds.max.y - ch->rc_bounds.min.y;
                }
                if(box.min_height.unit == u_content) {
                    box.min_height.unit = u_pixel;
                    box.min_height.value = px_box_content_height;//ch->rc_bounds.max.y - ch->rc_bounds.min.y;
                }
                if(box.max_height.unit == u_content) {
                    box.max_height.unit = u_pixel;
                    box.max_height.value = px_box_content_height;//ch->rc_bounds.max.y - ch->rc_bounds.min.y;
                }
            }
        }

        // Handle min-max height
        for (int j = 0; j < lines.size(); ++j) {
            LINE& line = lines[j];
            for (int k = 0; k < line.boxes.size(); ++k) {
                BOX& box = line.boxes[k];
                if (box.height.unit == u_fill) {
                    continue;
                }

                if (box.min_height.unit == u_pixel) {
                    box.height.value = gfxm::_max(box.min_height.value, box.height.value);
                }
                if (box.max_height.unit == u_pixel) {
                    box.height.value = gfxm::_min(box.max_height.value, box.height.value);
                }
            }
        }

        // Find line heights
        for (int j = 0; j < lines.size(); ++j) {
            LINE& line = lines[j];
            xfloat max_height = px(0);
            int n_fixed_height = false;
            int n_fill_height = false;
            for (int k = 0; k < line.boxes.size(); ++k) {
                BOX& box = line.boxes[k];
                xfloat height = box.height;
                if (box.overflow_height.value > height.value) {
                    height = box.overflow_height;
                }

                if (height.unit != u_fill) {
                    ++n_fixed_height;
                } else {
                    ++n_fill_height;
                }
                if (height.unit != u_fill && height.value > max_height.value) {
                    max_height = height;
                }
            }
            if (n_fill_height > 0 && n_fixed_height == 0) {
                max_height = fill();
            }
            line.height = max_height;
        }

        // Find free height
        int n_h_fill = 0;
        int height_eaten = 0;
        int free_height = 0;
        for (int j = 0; j < lines.size(); ++j) {
            LINE& line = lines[j];            
            if (line.height.unit == u_fill) {
                ++n_h_fill;
            } else {
                assert(line.height.unit == u_pixel);
                height_eaten += line.height.value;
            }
        }
        free_height = gfxm::_max(0, m_px_container_height - height_eaten);

        // Subtract margin space from free_height
        {
            int margin_space = int(px_content_margin_y * (lines.size() - 1));
            height_eaten += margin_space;
            free_height = gfxm::_max(0, free_height - margin_space);
        }

        // Convert height fills to pixels
        for (int j = 0; j < lines.size(); ++j) {
            LINE& line = lines[j];
            if (line.height.unit == u_fill) {
                int fill_height = n_h_fill ? free_height / n_h_fill : 0;
                line.height = px(fill_height);
            }

            for (int k = 0; k < line.boxes.size(); ++k) {
                BOX& box = line.boxes[k];
                if (box.height.unit != u_fill) {
                    continue;
                }
                box.height = line.height;
            }
        }

        /*
        // Call child layouts
        for (int j = 0; j < lines.size(); ++j) {
            LINE& line = lines[j];
            for (int k = 0; k < line.boxes.size(); ++k) {
                BOX& box = line.boxes[k];
                float h = box.height.value;
                if (box.early_layout) {
                    continue;
                }
                //box.elem->layout(gfxm::vec2(0,h), GUI_LAYOUT_HEIGHT_PASS);
            }
        }*/

        // Find vertical content extents
        int total_content_height = 0;
        int row_cur = 0;
        for (int j = 0; j < lines.size(); ++j) {
            LINE& line = lines[j];
            row_cur += line.height.value;
            total_content_height = row_cur;
            row_cur += px_content_margin_y;
        }
        /*
        rc_content.min.y = client_area.min.y - pos_content.y;
        rc_content.max.y = rc_content.min.y;
        if(lines.size() > 0) {
            rc_content.max.y = client_area.min.y + total_content_height - pos_content.y;
        }

        if (flags & GUI_LAYOUT_FIT_CONTENT) {
            const float sz_content = rc_content.max.y - rc_content.min.y;
            client_area.max.y = gfxm::_max(client_area.max.y, client_area.min.y + sz_content);
            rc_bounds.max.y = client_area.max.y + px_padding.max.y;
        }*/
    }

    void BoxLayout::buildPosition() {
        gfxm::vec2 px_content_margin = to_px(content_margin, m_px_line_height, gfxm::vec2(m_px_container_width, m_px_container_height));

        // Perform child layouts
        int total_content_height = 0;
        int row_cur = 0;
        for (int j = 0; j < lines.size(); ++j) {
            LINE& line = lines[j];
            int col_cur = 0;
            for (int k = 0; k < line.boxes.size(); ++k) {
                BOX& box = line.boxes[k];
                Box* ch = box.elem;

                box.px_pos_x = col_cur;
                box.px_pos_y = row_cur;
                /*
                ch->layout_position
                    = gfxm::vec2(client_area.min.x, client_area.min.y)
                    + gfxm::vec2(col_cur, row_cur)
                    - pos_content;
                assert(
                    box.width.unit == gui_pixel
                    && box.height.unit == gui_pixel
                );*/
                float w = box.width.value;
                float h = box.height.value;
                /*ch->layout(
                    gfxm::vec2(w, h),
                    GUI_LAYOUT_POSITION_PASS
                );*/

                col_cur += w;
                col_cur += px_content_margin.x;
            }
            row_cur += line.height.value;
            total_content_height = row_cur;
            row_cur += px_content_margin.y;
        }
    }


}