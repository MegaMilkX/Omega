#include "box_layout_handler.hpp"

#include "xui/host.hpp"
#include "xui/element.hpp"
#include "gui/style/styles.hpp"


namespace xui {


    void BoxLayoutHandler::onInit(Element* elem) {
        boxes.clear();
        for (int i = 0; i < elem->layout_children.size(); ++i) {
            auto ch = elem->layout_children[i];
            if (ch->isHidden()) {
                continue;
            }
            Font* font = ch->resolved_font;
            boxes.push_back(
                BoxLayout::Box{
                    .user_ptr = ch,
                    .px_line_height = font->getLineHeight(),
                    .size = ch->size,
                    .min_size = ch->min_size,
                    .max_size = ch->max_size,
                    .same_line = ch->same_line
                }
            );
        }

        // TODO: TEMPORARY
        layout.m_px_line_height = elem->resolved_font->getLineHeight();
        auto style = elem->getStyle();
        auto box_style = style->get_component<gui::style_box>();
        gui_vec2 gui_content_margin;
        gui_rect gui_padding;
        if (box_style) {
            gui_content_margin = box_style->content_margin.value(gui_vec2());
            gui_padding = box_style->padding.value(gui_rect());
        }
        layout.padding = gui_padding;
        layout.content_margin = gui_content_margin;
    }
    int BoxLayoutHandler::resolveWidth(Host* host, int width_available) {
        if(width_available >= 0) {
            layout.m_px_container_width = width_available;
            layout.m_fit_content_width = false;
        } else {
            // TODO: BoxLayout needs to resolve content size and be able to accept max sizes(?)
            layout.m_px_container_width = 0;
            layout.m_fit_content_width = true;
        }

        layout.buildWidth(boxes.data(), boxes.size(), [host](const BoxLayout::BOX* box)->int {
            Element* e = (Element*)(box->elem->user_ptr);
            LayoutContext ctx{
                .phase = LAYOUT_PHASE::WIDTH,
            };
            e->layout(host, ctx);
            return ctx.px_measured_width;
        });
        return layout.m_px_container_width;
    }
    int BoxLayoutHandler::resolveHeight(Host* host, int height_available) {
        if(height_available >= 0) {
            layout.m_px_container_height = height_available;
            layout.m_fit_content_height = false;
        } else {
            // TODO: BoxLayout needs to resolve content size and be able to accept max sizes
            layout.m_px_container_height = 0;
            layout.m_fit_content_height = true;
        }

        layout.buildHeight([host](const BoxLayout::BOX* box)->int {
            Element* e = (Element*)(box->elem->user_ptr);
            LayoutContext ctx{
                .phase = LAYOUT_PHASE::HEIGHT,
                .px_resolved_width = int(box->width.value),
            };
            e->layout(host, ctx);
            return ctx.px_measured_height;
        });
        return layout.m_px_container_height;
    }
    void BoxLayoutHandler::resolvePlacement(Host* host) {
        layout.buildPosition();
        for (int i = 0; i < layout.lines.size(); ++i) {
            auto& line = layout.lines[i];
            for(int j = 0; j < line.boxes.size(); ++j) {
                auto& box = line.boxes[j];
                int px_resolved_width = box.width.value;
                int px_resolved_height = box.height.value;
                Element* e = (Element*)(box.elem->user_ptr);
                LayoutContext ctx{
                    .phase = LAYOUT_PHASE::COMMIT,
                    .px_resolved_width = px_resolved_width,
                    .px_resolved_height = px_resolved_height
                };
                e->layout(host, ctx);
                assert(box.min_width.unit == gui_pixel);
                e->min_size.x = box.min_width.value;
                e->min_size.y = box.min_height.value;
                e->px_pos = gfxm::ivec2(box.px_pos_x, box.px_pos_y);
            }
        }
    }


}