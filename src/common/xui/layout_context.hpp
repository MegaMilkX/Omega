#pragma once

#include <optional>


namespace xui {

    enum class LAYOUT_PHASE {
        NONE,
        WIDTH,
        HEIGHT,
        COMMIT
    };
    inline LAYOUT_PHASE& operator++(LAYOUT_PHASE& e) {
        e = static_cast<LAYOUT_PHASE>(static_cast<int>(e) + 1);
        return e;
    }

    struct LayoutContext {
        LAYOUT_PHASE phase = LAYOUT_PHASE::NONE;
        int px_measured_width = 0;
        int px_measured_height = 0;
        std::optional<int> px_resolved_width;
        std::optional<int> px_resolved_height;
    };


}

