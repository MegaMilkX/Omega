#pragma once

#include <vector>
#include <optional>
#include "gui/layout/layout_base.hpp"
#include "gui/types.hpp"

class Font;


class GuiFlowLayout : public GuiLayoutBase {
    static constexpr uint32_t DIRTY_LINES           = 0x0001;
    static constexpr uint32_t DIRTY_WRAPPING        = 0x0002;
    static constexpr uint32_t DIRTY_PLACEMENT       = 0x0004;

    uint32_t dirty_flags = DIRTY_LINES;

    enum class BuildMode {
        TellWidth,
        TellHeight,
        Full
    };
    
    struct BOX {
        GuiElement* elem = nullptr;

        // resolved
        bool is_measured = false;
        int px_width = 0;
        int px_height = 0;
        int width_constrained = true;
        int height_constrained = true;
    };
    struct LINE {
        int begin;
        int end;

        // resolved
        int px_width = 0;
        int px_height = 0;
    };

    //
    std::vector<BOX> boxes;
    std::vector<LINE> wrapped_lines;
    int bounding_width = 0;
    int bounding_height = 0;
    int secondary_extent_no_padding = 0;
    //

    void buildLayout(
        GuiElement* elem,
        GuiElement** children,
        size_t child_count,
        std::optional<int> width_constraint,
        std::optional<int> height_constraint,
        BuildMode build_mode
    );
public:
    void fontChanged(GuiElement* elem, Font* font) override;
    int measureWidth(GuiElement* elem, const std::optional<int>& height_constraint) override;
    int measureHeight(GuiElement* elem, const std::optional<int>& width_constraint) override;
    void layout(GuiElement* elem, const gui_layout_context& ctx) override;
};

