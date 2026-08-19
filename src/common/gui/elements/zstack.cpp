#include "zstack.hpp"


GuiZStack::GuiZStack() {

}
int GuiZStack::measureWidth(const std::optional<int>& height) {
    return 0;
}
int GuiZStack::measureHeight(const std::optional<int>& width) {
    return 0;
}
void GuiZStack::layout_2(const gui_layout_context& ctx) {
    rc_bounds = gfxm::rect(gfxm::vec2(0, 0), gfxm::vec2(ctx.width.value_or(0), ctx.height.value_or(0)));
    client_area = rc_bounds;
    rc_content = rc_bounds;

    for (int i = 0; i < children.size(); ++i) {
        auto ch = children[i];

        gui_vec2 sz = ch->size;
        gui_layout_context ctx;
        ctx.width = rc_bounds.max.x - rc_bounds.min.x;
        ctx.height = rc_bounds.max.y - rc_bounds.min.y;
        ch->layout_2(ctx);

        // TODO: metrics?
        ch->layout_position.x = 0;
        ch->layout_position.y = 0;
    }
}
