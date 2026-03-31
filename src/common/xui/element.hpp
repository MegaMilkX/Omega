#pragma once

#include <list>
#include <memory>

#include "math/gfxm.hpp"
#include "typeface/font.hpp"
#include "xui/renderer/renderer.hpp"
#include "xui/types.hpp"
#include "xui/hit.hpp"
#include "xui/layout_context.hpp"

#include "gui/style/style_component.hpp"


namespace xui {


    class Host;
    class Element {
    protected:
        void registerChild(Element*);
        void unregisterChild(Element*);
    public:
        // Base element has children but does not expose any way to modify them from outside
        // This is only for propagation
        Element* layout_parent = nullptr;
        std::vector<Element*> layout_children;
        //

        //
        HIT hit_type = HIT::CLIENT;
        //

        //
        std::unique_ptr<gui::style> style;
        std::list<std::string> style_selectors;
        bool is_style_dirty = true;
        //

        //
        xvec2 size = px(100, 100);
        xvec2 min_size = px(0, 0);
        xvec2 max_size = px(INT_MAX, INT_MAX);
        uint32_t color = 0xFFFFFFFF;
        bool same_line = false;
        //

        gfxm::ivec2 px_pos; // in parent space
        gfxm::ivec2 px_size;


        virtual void onHitTest(HitResult& hit, int x, int y) = 0;
        virtual void layout(Host*, LayoutContext&) = 0;
        virtual void onDraw(IRenderer*) = 0;


        virtual ~Element();

        void applyStyle(Host*);
        void hitTest(HitResult& hit, int x, int y);
        void draw(IRenderer*);

        gfxm::rect getGlobalRect() const;
        gui::style* getStyle();
    };


}