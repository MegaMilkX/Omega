#pragma once


namespace xui {


    enum LayoutMode {
        LayoutNone,
        LayoutFloat,
        LayoutBox,
        LayoutStack
    };

    class Host;
    class Element;

    class LayoutHandler {
        LayoutMode type;
    protected:
        Element* element = nullptr;
    public:
        LayoutHandler(LayoutMode mode) : type(mode) {}
        LayoutMode getType() const { return type; }

        // TODO: Parent element as parameter, not array of children
        void init(Element* elem);
        virtual void onInit(Element* elem) {}
        virtual int resolveWidth(Host*, int width_available) = 0;
        virtual int resolveHeight(Host*, int height_available) = 0;
        virtual void resolvePlacement(Host*) = 0;
    };


}

