#pragma once

#include <assert.h>
#include <stack>
#include "platform/platform.hpp"
#include "math/gfxm.hpp"
#include "gpu/gpu_pipeline.hpp"
#include "gpu/gpu_text.hpp"

#include "gui/gui_hit.hpp"
#include "gui/gui_draw.hpp"

#include "gui/gui_values.hpp"
#include "gui/gui_color.hpp"

#include "gui/gui_system.hpp"

#include "gui/elements/dock_space.hpp"
#include "gui/elements/text.hpp"


#include "gui/gui_layout_helpers.hpp"

#include "gui/elements/text_element.hpp"
#include "gui/elements/icon.hpp"
#include "gui/elements/label.hpp"
#include "gui/elements/input.hpp"
#include "gui/elements/input_text.hpp"
#include "gui/elements/image.hpp"
#include "gui/elements/button.hpp"
#include "gui/elements/input_file_path.hpp"


enum class GUI_INPUT_NUMBER_TYPE {
    INT8,
    UINT8,
    INT16,
    UINT16,
    INT32,
    UINT32,
    INT64,
    UINT64,
    FLOAT,
    DOUBLE
};
inline int guiInputNumberTypeSize(GUI_INPUT_NUMBER_TYPE type) {
    switch (type) {
    case GUI_INPUT_NUMBER_TYPE::INT8:
    case GUI_INPUT_NUMBER_TYPE::UINT8:
        return 1;
    case GUI_INPUT_NUMBER_TYPE::INT16:
    case GUI_INPUT_NUMBER_TYPE::UINT16:
        return 2;
    case GUI_INPUT_NUMBER_TYPE::INT32:
    case GUI_INPUT_NUMBER_TYPE::UINT32:
    case GUI_INPUT_NUMBER_TYPE::FLOAT:
        return 4;
    case GUI_INPUT_NUMBER_TYPE::INT64:
    case GUI_INPUT_NUMBER_TYPE::UINT64:
    case GUI_INPUT_NUMBER_TYPE::DOUBLE:
        return 8;
    default:
        return 0;
    }
}

template<typename TYPE>
class GuiInputNumberBoxT : public GuiElement {
    GuiTextBuffer text_content;

    gfxm::vec2 pos_content;

    gfxm::vec2 mouse_pos;
    bool pressing = false;
    bool dragging = false;
    bool editing = false;

    TYPE value = 0;

    bool hexadecimal = false;
    unsigned decimal_places = 2;

    void updateValueFromText() {
        std::string str;
        text_content.getWholeText(str);
        value = valueFromTextImpl(str);
        updateTextFromValue();
    }
    TYPE valueFromTextImpl(const std::string& str) {
        static_assert(false, "GuiInputNumberBoxT: TYPE unsupported");
        return TYPE();
    }
    void updateTextFromValue(bool silent = false) {
        std::string str = textFromValueImpl(value);
        text_content.replaceAll(getFont(), str.c_str(), str.length());
        if (!silent) {
            getParent()->sendMessage(GUI_MSG::NUMERIC_UPDATE, GUI_MSG_PARAMS{});
        }
    }
    std::string textFromValueImpl(const TYPE& value) {
        static_assert(false, "GuiInputNumberBoxT: TYPE unsupported");
        return std::string();
    }
    void incrementImpl(int offset) {
        setValue(getValue() + offset);
    }
public:
    GuiInputNumberBoxT(
        bool hexadecimal = false,
        unsigned decimal_places = 2
    )
    :   hexadecimal(hexadecimal),
        decimal_places(decimal_places)
    {
        setSize(0, 0);
        setMaxSize(0, 0);
        setMinSize(0, 0);

        updateTextFromValue();
    }

    void        setValue        (TYPE val) { value = val; updateTextFromValue(); }
    void        setValueSilent  (TYPE val) { value = val; updateTextFromValue(true); }
    TYPE        getValue        () const { return value; }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::FOCUS:
            return true;
        case GUI_MSG::UNFOCUS:
            if (editing) {
                updateValueFromText();
                editing = false;
            }
            return true;
        case GUI_MSG::KEYDOWN:
            // TODO: Remove winapi usage
            switch (params.getA<uint16_t>()) {
            case VK_RIGHT: text_content.advanceCursor(1, !(guiGetModifierKeysState() & GUI_KEY_SHIFT)); break;
            case VK_LEFT: text_content.advanceCursor(-1, !(guiGetModifierKeysState() & GUI_KEY_SHIFT)); break;
            case VK_UP: text_content.advanceCursorLine(-1, !(guiGetModifierKeysState() & GUI_KEY_SHIFT)); break;
            case VK_DOWN: text_content.advanceCursorLine(1, !(guiGetModifierKeysState() & GUI_KEY_SHIFT)); break;
            case VK_DELETE:
                text_content.del();
                break;
            // CTRL+V key
            case 0x0056: if (guiGetModifierKeysState() & GUI_KEY_CONTROL) {
                std::string str;
                if (guiClipboardGetString(str)) {
                    text_content.putString(getFont(), str.c_str(), str.size());
                }
            } break;
            // CTRL+C, CTRL+X
            case 0x0043: // C
            case 0x0058: /* X */ if (guiGetModifierKeysState() & GUI_KEY_CONTROL) {
                std::string copied;
                if (text_content.copySelected(copied, params.getA<uint16_t>() == 0x0058)) {
                    guiClipboardSetString(copied);
                }
            } break;
            // CTRL+A
            case 0x41: if (guiGetModifierKeysState() & GUI_KEY_CONTROL) { // A
                text_content.selectAll();
            } break;
            }
            return true;
        case GUI_MSG::UNICHAR:
            switch (params.getA<GUI_CHAR>()) {
            case GUI_CHAR::BACKSPACE:
                text_content.backspace();
                break;
            case GUI_CHAR::ESCAPE:
                break;
            case GUI_CHAR::RETURN:
                if (editing) {
                    updateValueFromText();
                    editing = false;
                }
                break;
            default: {
                if (editing) {
                    char ch = (char)params.getA<GUI_CHAR>();
                    if (ch > 0x1F || ch == 0x0A) {
                        text_content.putString(getFont(), &ch, 1);
                    }
                }
            }
            }
            return true;
        case GUI_MSG::MOUSE_MOVE: {
            gfxm::vec2 new_mouse_pos = gfxm::vec2(params.getA<int32_t>(), params.getB<int32_t>());
            if (pressing && guiGetFocusedWindow() == this) {
                if (!editing) {
                    dragging = true;
                    incrementImpl((new_mouse_pos.x - mouse_pos.x));
                } else {
                    text_content.pickCursor(getFont(), new_mouse_pos - pos_content, false);
                }
            }
            mouse_pos = new_mouse_pos;
            } return true;
        case GUI_MSG::LBUTTON_DOWN:
            guiCaptureMouse(this);
            pressing = true;
            if (editing) {
                text_content.pickCursor(getFont(), mouse_pos - pos_content, true);
            }
            return true;
        case GUI_MSG::LBUTTON_UP:
            pressing = false;
            dragging = false;
            return true;
        case GUI_MSG::DBL_LCLICK:
            editing = true;
            return true;
        }

        return GuiElement::onMessage(msg, params);
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        Font* font = getFont();

        const float text_box_height = font->getLineHeight() * 2.0f;
        setHeight(text_box_height);
        rc_bounds = gfxm::rect(
            rc.min,
            gfxm::vec2(rc.max.x, rc.min.y + text_box_height)
        );
        client_area = rc_bounds;

        text_content.prepareDraw(font, true);
    }
    void onDraw() override {
        uint32_t col_box = GUI_COL_HEADER;
        if (isHovered() && !editing) {
            col_box = GUI_COL_BUTTON;
        }
        guiDrawRectRound(client_area, GUI_PADDING * 2.f, col_box);
        if (editing) {
            guiDrawRectRoundBorder(client_area, GUI_PADDING * 2.f, 2.f, GUI_COL_ACCENT, GUI_COL_ACCENT);
        }

        guiDrawPushScissorRect(client_area);
        text_content.draw(getFont(), client_area, GUI_VCENTER | GUI_HCENTER, GUI_COL_TEXT, GUI_COL_ACCENT);
        guiDrawPopScissorRect();

        //guiDrawRectLine(rc_bounds, 0xFF00FF00);
        //guiDrawRectLine(client_area, 0xFFFFFF00);
    }
};
template<>
int8_t GuiInputNumberBoxT<int8_t>::valueFromTextImpl(const std::string& str) {
    char* e = 0;
    return std::max(
        (long)std::numeric_limits<int8_t>::min(),
        std::min((long)std::numeric_limits<int8_t>::max(), strtol(str.c_str(), &e, 10))
    );
}
template<>
int16_t GuiInputNumberBoxT<int16_t>::valueFromTextImpl(const std::string& str) {
    char* e = 0;
    return std::max(
        (long)std::numeric_limits<int16_t>::min(),
        std::min((long)std::numeric_limits<int16_t>::max(), strtol(str.c_str(), &e, 10))
    );
}
template<>
int32_t GuiInputNumberBoxT<int32_t>::valueFromTextImpl(const std::string& str) {
    char* e = 0;
    return std::max(
        (long)std::numeric_limits<int32_t>::min(),
        std::min((long)std::numeric_limits<int32_t>::max(), strtol(str.c_str(), &e, 10))
    );
}
template<>
int64_t GuiInputNumberBoxT<int64_t>::valueFromTextImpl(const std::string& str) {
    char* e = 0;
    return std::max(
        (int64_t)std::numeric_limits<int64_t>::min(),
        std::min((int64_t)std::numeric_limits<int64_t>::max(), strtoll(str.c_str(), &e, 10))
    );
}
template<>
uint8_t GuiInputNumberBoxT<uint8_t>::valueFromTextImpl(const std::string& str) {
    char* e = 0;
    return std::max(
        (unsigned long)std::numeric_limits<uint8_t>::min(),
        std::min((unsigned long)std::numeric_limits<uint8_t>::max(), strtoul(str.c_str(), &e, 10))
    );
}
template<>
uint16_t GuiInputNumberBoxT<uint16_t>::valueFromTextImpl(const std::string& str) {
    char* e = 0;
    return std::max(
        (unsigned long)std::numeric_limits<uint16_t>::min(),
        std::min((unsigned long)std::numeric_limits<uint16_t>::max(), strtoul(str.c_str(), &e, 10))
    );
}
template<>
uint32_t GuiInputNumberBoxT<uint32_t>::valueFromTextImpl(const std::string& str) {
    char* e = 0;
    return std::max(
        (unsigned long)std::numeric_limits<uint32_t>::min(),
        std::min((unsigned long)std::numeric_limits<uint32_t>::max(), strtoul(str.c_str(), &e, 10))
    );
}
template<>
uint64_t GuiInputNumberBoxT<uint64_t>::valueFromTextImpl(const std::string& str) {
    char* e = 0;
    return std::max(
        (uint64_t)std::numeric_limits<uint64_t>::min(),
        std::min((uint64_t)std::numeric_limits<uint64_t>::max(), strtoull(str.c_str(), &e, 10))
    );
}
template<>
float GuiInputNumberBoxT<float>::valueFromTextImpl(const std::string& str) {
    char* e = 0;
    return strtof(str.c_str(), &e);
}
template<>
double GuiInputNumberBoxT<double>::valueFromTextImpl(const std::string& str) {
    char* e = 0;
    return strtod(str.c_str(), &e);
}

template<>
std::string GuiInputNumberBoxT<int8_t>::textFromValueImpl(const int8_t& value) {
    const int BUF_SIZE = 32;
    char buf[BUF_SIZE];
    int str_len = 0;
    str_len = snprintf(buf, BUF_SIZE, hexadecimal ? "%X" : "%i", value);
    return std::string(buf, buf + str_len);
}
template<>
std::string GuiInputNumberBoxT<int16_t>::textFromValueImpl(const int16_t& value) {
    const int BUF_SIZE = 32;
    char buf[BUF_SIZE];
    int str_len = 0;
    str_len = snprintf(buf, BUF_SIZE, hexadecimal ? "%X" : "%i", value);
    return std::string(buf, buf + str_len);
}
template<>
std::string GuiInputNumberBoxT<int32_t>::textFromValueImpl(const int32_t& value) {
    const int BUF_SIZE = 32;
    char buf[BUF_SIZE];
    int str_len = 0;
    str_len = snprintf(buf, BUF_SIZE, hexadecimal ? "%X" : "%i", value);
    return std::string(buf, buf + str_len);
}
template<>
std::string GuiInputNumberBoxT<int64_t>::textFromValueImpl(const int64_t& value) {
    const int BUF_SIZE = 32;
    char buf[BUF_SIZE];
    int str_len = 0;
    str_len = snprintf(buf, BUF_SIZE, hexadecimal ? "%X" : "%i", value);
    return std::string(buf, buf + str_len);
}
template<>
std::string GuiInputNumberBoxT<uint8_t>::textFromValueImpl(const uint8_t& value) {
    const int BUF_SIZE = 32;
    char buf[BUF_SIZE];
    int str_len = 0;
    str_len = snprintf(buf, BUF_SIZE, hexadecimal ? "%X" : "%i", value);
    return std::string(buf, buf + str_len);
}
template<>
std::string GuiInputNumberBoxT<uint16_t>::textFromValueImpl(const uint16_t& value) {
    const int BUF_SIZE = 32;
    char buf[BUF_SIZE];
    int str_len = 0;
    str_len = snprintf(buf, BUF_SIZE, hexadecimal ? "%X" : "%i", value);
    return std::string(buf, buf + str_len);
}
template<>
std::string GuiInputNumberBoxT<uint32_t>::textFromValueImpl(const uint32_t& value) {
    const int BUF_SIZE = 32;
    char buf[BUF_SIZE];
    int str_len = 0;
    str_len = snprintf(buf, BUF_SIZE, hexadecimal ? "%X" : "%i", value);
    return std::string(buf, buf + str_len);
}
template<>
std::string GuiInputNumberBoxT<uint64_t>::textFromValueImpl(const uint64_t& value) {
    const int BUF_SIZE = 32;
    char buf[BUF_SIZE];
    int str_len = 0;
    str_len = snprintf(buf, BUF_SIZE, hexadecimal ? "%X" : "%i", value);
    return std::string(buf, buf + str_len);
}
template<>
std::string GuiInputNumberBoxT<float>::textFromValueImpl(const float& value) {
    const int BUF_SIZE = 32;
    char buf[BUF_SIZE];
    int str_len = 0;
    std::string fmt = MKSTR("%." << decimal_places << "f");
    str_len = snprintf(buf, BUF_SIZE, fmt.c_str(), value);
    return std::string(buf, buf + str_len);
}
template<>
std::string GuiInputNumberBoxT<double>::textFromValueImpl(const double& value) {
    const int BUF_SIZE = 32;
    char buf[BUF_SIZE];
    int str_len = 0;
    std::string fmt = MKSTR("%." << decimal_places << "f");
    str_len = snprintf(buf, BUF_SIZE, fmt.c_str(), value);
    return std::string(buf, buf + str_len);
}

template<>
void GuiInputNumberBoxT<float>::incrementImpl(int offset) {
    setValue(getValue() + offset * .01f);
}
template<>
void GuiInputNumberBoxT<double>::incrementImpl(int offset) {
    setValue(getValue() + offset * .01);
}


inline void guiLayoutSplitRectX(const gfxm::rect& rc, gfxm::rect& a, gfxm::rect& b, float width_a) {
    const float w = rc.max.x - rc.min.x;
    const float w0 = gfxm::_min(w, width_a);
    const float w1 = w - w0;

    gfxm::rect rc_original = rc;

    a = gfxm::rect(
        gfxm::vec2(rc_original.min.x, rc_original.min.y),
        gfxm::vec2(rc_original.min.x + w0, rc_original.max.y)
    );
    b = gfxm::rect(
        gfxm::vec2(a.max.x, a.min.y),
        gfxm::vec2(rc_original.max.x, rc_original.max.y)
    );
}
inline void guiLayoutSplitRectY(const gfxm::rect& rc, gfxm::rect& a, gfxm::rect& b, float height_a) {
    const float h = rc.max.y - rc.min.y;
    const float h0 = gfxm::_min(h, height_a);
    const float h1 = h - h0;

    gfxm::rect rc_original = rc;

    a = gfxm::rect(
        gfxm::vec2(rc_original.min.x, rc_original.min.y),
        gfxm::vec2(rc_original.max.x, rc_original.min.y + h0)
    );
    b = a;
    b.min.y = a.max.y;
    b.max.y = rc_original.max.y;
}

template<int WIDTH, int HEIGHT>
class GuiGridHelper {
    gfxm::rect rects[WIDTH * HEIGHT];
public:
    GuiGridHelper(const gfxm::rect& rc) {
        int count = WIDTH * HEIGHT;
        float cell_w = (rc.max.x - rc.min.x) / WIDTH;
        float cell_h = (rc.max.y - rc.min.y) / HEIGHT;
        for (int i = 0; i < count; ++i) {
            gfxm::vec2 min;
            min.x = (i % WIDTH) * cell_w;
            min.y = (i / HEIGHT) * cell_h;
            gfxm::vec2 max;
            max.x = min.x + cell_w;
            max.y = min.y + cell_h;
            rects[i] = gfxm::rect(rc.min + min, rc.min + max);
        }
    }
    const gfxm::rect& getRect(int x, int y) {
        return rects[y * WIDTH + x];
    }
};

class GuiInputBase : public GuiElement {
public:
    virtual void refreshData() {}
};

template<typename TYPE, int COUNT>
class GuiInputNumericT : public GuiInputBase {
    std::array<TYPE, COUNT> fallback_data;
    TYPE* pvalue = 0;
    std::function<void(TYPE*)> getter;
    std::function<void(TYPE*)> setter;
    GuiLabel           label;
    std::vector<std::unique_ptr<GuiInputNumberBoxT<TYPE>>> boxes;
public:
    GuiInputNumericT(
        const char* caption = "InputNumber",
        TYPE* value_ptr = nullptr,
        unsigned decimal_places = 2,
        bool hexadecimal = false
    ) : label(caption), pvalue(value_ptr) {
        setSize(0, 0);
        setMaxSize(0, 0);
        setMinSize(0, 0);
        setStyleClasses({ "control" });

        if (!pvalue) {
            pvalue = &fallback_data[0];
        }

        static_assert(COUNT > 0 && COUNT <= 4, "");
        boxes.resize(COUNT);
        for (int i = 0; i < COUNT; ++i) {
            boxes[i].reset(new GuiInputNumberBoxT<TYPE>(hexadecimal, decimal_places));
            boxes[i]->setParent(this);
        }
        for (int i = 0; i < COUNT; ++i) {
            boxes[i]->setValueSilent(pvalue[i]);
        }
    }
    GuiInputNumericT(
        const char* caption,
        std::function<void(TYPE*)> getter,
        std::function<void(TYPE*)> setter,
        unsigned decimal_places = 2,
        bool hexadecimal = false
    ) : label(caption), pvalue(&fallback_data[0]), getter(getter), setter(setter) {
        setSize(0, 0);
        setMaxSize(0, 0);
        setMinSize(0, 0);
        setStyleClasses({ "control" });

        static_assert(COUNT > 0 && COUNT <= 4, "");

        boxes.resize(COUNT);
        for (int i = 0; i < COUNT; ++i) {
            boxes[i].reset(new GuiInputNumberBoxT<TYPE>(hexadecimal, decimal_places));
            boxes[i]->setParent(this);
        }

        if (getter) {
            getter(&fallback_data[0]);            
        }
        for (int i = 0; i < COUNT; ++i) {
            boxes[i]->setValueSilent(pvalue[i]);
        }
    }

    void refreshData() override {
        if (getter) {
            getter(&fallback_data[0]);
        }
        for (int i = 0; i < COUNT; ++i) {
            boxes[i]->setValueSilent(pvalue[i]);
        }
    }

    void onHitTest(GuiHitResult& hit, int x, int y) override {
        for (auto& b : boxes) {
            b->onHitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
        }
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
        switch (msg) {
        case GUI_MSG::NUMERIC_UPDATE:
            for (int i = 0; i < COUNT; ++i) {
                fallback_data[i] = boxes[i]->getValue();
            }
            if (setter) {
                setter(pvalue);
            }
            return true;
        }
        return GuiElement::onMessage(msg, params);
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        setHeight(getFont()->getLineHeight() * 2.f);
        gfxm::rect rc_label = rc;
        gfxm::rect rc_inp;
        guiLayoutSplitRect2XRatio(rc_label, rc_inp, .25f);

        label.layout(rc_label, flags);

        std::vector<gfxm::rect> rcs(boxes.size());
        guiLayoutSplitRectH(rc_inp, &rcs[0], rcs.size(), .0f);
        for (int i = 0; i < boxes.size(); ++i) {
            boxes[i]->layout(rcs[i], flags);
        }

        rc_bounds = label.getBoundingRect();
        for (int i = 0; i < boxes.size(); ++i) {
            gfxm::expand(rc_bounds, boxes[i]->getBoundingRect());
        }
    }
    void onDraw() override {
        label.draw();
        for (int i = 0; i < boxes.size(); ++i) {
            boxes[i]->draw();
        }
    }
};
typedef GuiInputNumericT<uint64_t, 1> GuiInputUInt64;
typedef GuiInputNumericT<uint64_t, 2> GuiInputUInt64_2;
typedef GuiInputNumericT<uint64_t, 3> GuiInputUInt64_3;
typedef GuiInputNumericT<uint64_t, 4> GuiInputUInt64_4;
typedef GuiInputNumericT<int64_t, 1> GuiInputInt64;
typedef GuiInputNumericT<int64_t, 2> GuiInputInt64_2;
typedef GuiInputNumericT<int64_t, 3> GuiInputInt64_3;
typedef GuiInputNumericT<int64_t, 4> GuiInputInt64_4;
typedef GuiInputNumericT<uint32_t, 1> GuiInputUInt32;
typedef GuiInputNumericT<uint32_t, 2> GuiInputUInt32_2;
typedef GuiInputNumericT<uint32_t, 3> GuiInputUInt32_3;
typedef GuiInputNumericT<uint32_t, 4> GuiInputUInt32_4;
typedef GuiInputNumericT<int32_t, 1> GuiInputInt32;
typedef GuiInputNumericT<int32_t, 2> GuiInputInt32_2;
typedef GuiInputNumericT<int32_t, 3> GuiInputInt32_3;
typedef GuiInputNumericT<int32_t, 4> GuiInputInt32_4;
typedef GuiInputNumericT<uint16_t, 1> GuiInputUInt16;
typedef GuiInputNumericT<uint16_t, 2> GuiInputUInt16_2;
typedef GuiInputNumericT<uint16_t, 3> GuiInputUInt16_3;
typedef GuiInputNumericT<uint16_t, 4> GuiInputUInt16_4;
typedef GuiInputNumericT<int16_t, 1> GuiInputInt16;
typedef GuiInputNumericT<int16_t, 2> GuiInputInt16_2;
typedef GuiInputNumericT<int16_t, 3> GuiInputInt16_3;
typedef GuiInputNumericT<int16_t, 4> GuiInputInt16_4;
typedef GuiInputNumericT<uint8_t, 1> GuiInputUInt8;
typedef GuiInputNumericT<uint8_t, 2> GuiInputUInt8_2;
typedef GuiInputNumericT<uint8_t, 3> GuiInputUInt8_3;
typedef GuiInputNumericT<uint8_t, 4> GuiInputUInt8_4;
typedef GuiInputNumericT<int8_t, 1> GuiInputInt8;
typedef GuiInputNumericT<int8_t, 2> GuiInputInt8_2;
typedef GuiInputNumericT<int8_t, 3> GuiInputInt8_3;
typedef GuiInputNumericT<int8_t, 4> GuiInputInt8_4;
typedef GuiInputNumericT<float, 1> GuiInputFloat;
typedef GuiInputNumericT<float, 2> GuiInputFloat2;
typedef GuiInputNumericT<float, 3> GuiInputFloat3;
typedef GuiInputNumericT<float, 4> GuiInputFloat4;
typedef GuiInputNumericT<double, 1> GuiInputDouble;
typedef GuiInputNumericT<double, 2> GuiInputDouble2;
typedef GuiInputNumericT<double, 3> GuiInputDouble3;
typedef GuiInputNumericT<double, 4> GuiInputDouble4;


class GuiCheckBox : public GuiElement {
    GuiTextBuffer caption;
    bool* value = 0;
public:
    GuiCheckBox(const char* cap = "CheckBox", bool* data = 0)
        : value(data) {
        setSize(0, 0);
        setMaxSize(0, 0);
        setMinSize(0, 0);
        setStyleClasses({ "control" });
        caption.replaceAll(getFont(), cap, strlen(cap));
    }

    void toggle() {
        if (value) {
            *value = !(*value);
        }
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::LCLICK:
        case GUI_MSG::DBL_LCLICK: {
            toggle();
            return true;
        }
        }
        return false;
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        Font* font = getFont();

        rc_bounds = rc;
        rc_bounds.max.y = rc_bounds.min.y + font->getLineHeight();
        client_area = rc_bounds;

        caption.prepareDraw(font, false);

        float box_sz = client_area.max.y - client_area.min.y;
        client_area.max.x = client_area.min.x + box_sz + GUI_MARGIN + caption.getBoundingSize().x;
    }
    void onDraw() override {
        gfxm::rect rc_box = client_area;
        rc_box.max.x = rc_box.min.x + (rc_box.max.y - rc_box.min.y);
        
        guiDrawCheckBox(rc_box, value ? *value : false, isHovered());

        caption.draw(getFont(), gfxm::vec2(rc_box.max.x + GUI_MARGIN, client_area.min.y), GUI_COL_TEXT, GUI_COL_ACCENT);
    }
};

class GuiRadioButton : public GuiElement {
    GuiTextBuffer caption;
public:
    GuiRadioButton(const char* cap = "RadioButton") {
        setSize(0, 0);
        setMaxSize(0, 0);
        setMinSize(0, 0);
        setStyleClasses({ "control" });
        caption.replaceAll(getFont(), cap, strlen(cap));
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        return false;
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        Font* font = getFont();

        rc_bounds = rc;
        rc_bounds.max.y = rc_bounds.min.y + font->getLineHeight();
        client_area = rc_bounds;

        caption.prepareDraw(font, false);
    }
    void onDraw() override {
        gfxm::rect rc_box = client_area;
        rc_box.max.x = rc_box.min.x + (rc_box.max.y - rc_box.min.y);
        guiDrawCircle(rc_box.center(), rc_box.size().x * .5f, true, GUI_COL_HEADER);
        guiDrawCircle(rc_box.center(), rc_box.size().x * .24f, true, GUI_COL_TEXT);

        caption.draw(getFont(), gfxm::vec2(rc_box.max.x + GUI_MARGIN, client_area.min.y), GUI_COL_TEXT, GUI_COL_ACCENT);
    }
};

class GuiListItem : public GuiElement {
    GuiTextBuffer caption;
public:
    GuiListItem(const char* cap = "ListItem") {
        caption.replaceAll(getFont(), cap, strlen(cap));
    }

    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        Font* font = getFont();
        const float h = font->getLineHeight();
        rc_bounds = gfxm::rect(rc.min, gfxm::vec2(rc.max.x, .0f));
        rc_bounds.max.y = rc_bounds.min.y + h;
        client_area = rc_bounds;

        caption.prepareDraw(font, false);
    }
    void onDraw() override {
        if (isHovered()) {
            guiDrawRect(client_area, GUI_COL_BUTTON);
        }
        caption.draw(getFont(), client_area.min, GUI_COL_TEXT, GUI_COL_ACCENT);
    }
};

class GuiMenuList;
class GuiMenuListItem : public GuiElement {
    GuiTextBuffer caption;
    bool is_open = false;
    std::unique_ptr<GuiMenuList> menu_list;
    GuiIcon* icon_arrow = 0;
    std::function<void(void)> on_click;
public:
    int id = 0;
    int command_identifier = 0;

    void open();
    void close();

    bool hasList() { return menu_list.get() != nullptr; }

    GuiMenuListItem(const char* cap, std::function<void(void)> on_click)
        : on_click(on_click) {
        setSize(0, 0);
        caption.replaceAll(getFont(), cap, strlen(cap));
        icon_arrow = guiLoadIcon("svg/entypo/triangle-right.svg");
    }
    GuiMenuListItem(const char* cap = "MenuListItem", int cmd = 0)
        : command_identifier(cmd) {
        setSize(0, 0);
        caption.replaceAll(getFont(), cap, strlen(cap));
        icon_arrow = guiLoadIcon("svg/entypo/triangle-right.svg");
    }
    GuiMenuListItem(const char* cap, const std::initializer_list<GuiMenuListItem*>& child_items);
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
        switch (msg) {
        case GUI_MSG::CLOSE_MENU:
            return true;
        case GUI_MSG::MOUSE_ENTER:
            notifyOwner(GUI_NOTIFY::MENU_ITEM_HOVER, id);
            break;
        case GUI_MSG::LCLICK:
        case GUI_MSG::DBL_LCLICK:
            if (hasList()) {
                open();
                //notifyOwner(GUI_NOTIFY::MENU_ITEM_CLICKED, id);
            } else {
                notifyOwner(GUI_NOTIFY::MENU_COMMAND, command_identifier);
                if (on_click) {
                    on_click();
                }
            }
            return true;
        case GUI_MSG::NOTIFY:
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::MENU_COMMAND:
                close();
                forwardMessageToOwner(msg, params);
                return true;
            }
            break;
        }

        return GuiElement::onMessage(msg, params);
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        Font* font = getFont();
        caption.prepareDraw(font, false);
        const float h = font->getLineHeight() * 2.0f;
        const float w = gfxm::_max(rc.max.x - rc.min.x, caption.getBoundingSize().x + GUI_MARGIN * 2.f);
        rc_bounds = gfxm::rect(rc.min, gfxm::vec2(rc.min.x + w, rc.min.y + h));
        client_area = rc_bounds;
    }
    void onDraw() override {
        Font* font = getFont();
        if (isHovered()) {
            guiDrawRect(client_area, GUI_COL_BUTTON);
        }
        {
            gfxm::rect rc = client_area;
            rc.min.x += GUI_MARGIN;
            caption.draw(font, rc, GUI_LEFT | GUI_VCENTER, GUI_COL_TEXT, GUI_COL_ACCENT);
            //caption.draw(client_area.min + gfxm::vec2(GUI_MARGIN, guiGetCurrentFont()->font->getLineHeight() * .25f), GUI_COL_TEXT, GUI_COL_ACCENT);
        }
        float fontH = font->getLineHeight();
        if (hasList() && icon_arrow) {
            icon_arrow->draw(guiLayoutPlaceRectInsideRect(client_area, gfxm::vec2(fontH, fontH), GUI_RIGHT | GUI_VCENTER, gfxm::rect(GUI_MARGIN, GUI_MARGIN, GUI_MARGIN, GUI_MARGIN)), GUI_COL_TEXT);
        }
    }
};
class GuiMenuList : public GuiElement {
    std::vector<std::unique_ptr<GuiMenuListItem>> items;
    GuiMenuListItem* open_elem = 0;
public:
    void open() {
        setHidden(false);
    }
    void close() {
        setHidden(true);
        if (open_elem) {
            open_elem->close();
        }
    }

    GuiMenuList() {
        addFlags(
            GUI_FLAG_TOPMOST
            | GUI_FLAG_FLOATING
            | GUI_FLAG_MENU_POPUP
        );
        overflow = GUI_OVERFLOW_FIT;
    }
    GuiMenuList* addItem(GuiMenuListItem* item) {
        item->id = items.size();
        items.push_back(std::unique_ptr<GuiMenuListItem>(item));
        addChild(item);
        item->setOwner(this);
        return this;
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::CLOSE_MENU:
            if (getOwner()) {
                guiPostMessage(getOwner(), GUI_MSG::CLOSE_MENU, GUI_MSG_PARAMS());
            }
            setHidden(true);
            return true;
        case GUI_MSG::NOTIFY:
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::MENU_ITEM_HOVER: {
                int id = params.getB<int>();
                if (items[id]->hasList()) {
                    if (open_elem && open_elem->id != params.getA<int>()) {
                        open_elem->close();
                        open_elem = nullptr;
                    }
                    if (!open_elem && items[id]->hasList()) {
                        open_elem = items[id].get();
                        open_elem->open();
                    }
                }
                }return true;
            }
            break;
        }
        return false;
    }/*
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        rc_bounds = gfxm::rect(
            pos, pos + size
        );
        client_area = rc_bounds;
        
        gfxm::vec2 cur = client_area.min;
        gfxm::rect rc_content = rc;
        cur.y += GUI_PADDING;
        rc_content.max.x = rc_content.min.x + 200.0f;
        float min_width = .0f;
        for (int i = 0; i < childCount(); ++i) {
            auto ch = getChild(i);
            gfxm::rect rc = rc_content;
            rc.min = cur;
            ch->layout(rc, flags);
            cur.y += ch->getBoundingRect().max.y - ch->getBoundingRect().min.y;
            if (ch->getBoundingRect().max.x - ch->getBoundingRect().min.x > min_width) {
                min_width = ch->getBoundingRect().max.x - ch->getBoundingRect().min.x;
            }
            rc_bounds.max.y = ch->getBoundingRect().max.y;
        }
        rc_bounds.max.x = rc_bounds.min.x + min_width;
        rc_bounds.max.y += GUI_PADDING;
        client_area = rc_bounds;
    }*/
    void onDraw() override {
        guiDrawRectShadow(rc_bounds);
        guiDrawRect(rc_bounds, GUI_COL_HEADER);
        guiDrawRectLine(rc_bounds, GUI_COL_BUTTON);
        for (int i = 0; i < childCount(); ++i) {
            auto ch = getChild(i);
            ch->draw();
        }
    }
};
inline GuiMenuListItem::GuiMenuListItem(const char* cap, const std::initializer_list<GuiMenuListItem*>& child_items) {
    setSize(0, 0);
    caption.replaceAll(getFont(), cap, strlen(cap));
    menu_list.reset(new GuiMenuList);
    menu_list->setOwner(this);
    menu_list->setHidden(true);
    menu_list->addFlags(GUI_FLAG_MENU_SKIP_OWNER_CLICK);
    guiGetRoot()->addChild(menu_list.get());
    for (auto ch : child_items) {
        menu_list->addItem(ch);
    }
    icon_arrow = guiLoadIcon("svg/entypo/triangle-right.svg");
}
inline void GuiMenuListItem::open() {
    menu_list->open();
    menu_list->pos = gui_vec2(client_area.max.x, client_area.min.y);
    menu_list->size = gui_vec2(200, 200);
    is_open = true;
    guiBringWindowToTop(menu_list.get());
}
inline void GuiMenuListItem::close() {
    menu_list->close();
    is_open = false;
}

class GuiComboBoxCtrl : public GuiElement {
    GuiTextBuffer text_content;

    gfxm::vec2 pos_content;

    gfxm::vec2 mouse_pos;
    bool pressing = false;

    bool is_open = false;
    GuiIcon* current_icon = 0;

    std::unique_ptr<GuiMenuList> menu_list;
public:
    GuiComboBoxCtrl(const char* text = "ComboBox") {
        setSize(0, 0);
        setMaxSize(0, 0);
        setMinSize(0, 0);

        text_content.putString(getFont(), text, strlen(text));

        current_icon = guiLoadIcon("svg/entypo/triangle-down.svg");
        
        menu_list.reset(new GuiMenuList());
        guiGetRoot()->addChild(menu_list.get());
        menu_list->setOwner(this);
        menu_list->addItem(new GuiMenuListItem("Hello"));
        menu_list->addItem(new GuiMenuListItem("An item"));
        menu_list->setHidden(true);
        menu_list->addFlags(GUI_FLAG_MENU_SKIP_OWNER_CLICK);
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::CLOSE_MENU: {
            is_open = false;
            return GuiElement::onMessage(msg, params);
        }
        case GUI_MSG::LCLICK:
        case GUI_MSG::DBL_LCLICK:
            if (!is_open) {
                is_open = true;
                menu_list->open();
                menu_list->pos = gui_vec2(client_area.min.x, client_area.max.y);
                menu_list->min_size = gui_vec2(client_area.max.x - client_area.min.x, 0);
                menu_list->max_size = gui_vec2(client_area.max.x - client_area.min.x, 0);
            } else {
                is_open = false;
                menu_list->close();
            }
            return true;
        }

        return GuiElement::onMessage(msg, params);
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        Font* font = getFont();

        const float text_box_height = font->getLineHeight() * 2.0f;
        setHeight(text_box_height);
        rc_bounds = gfxm::rect(
            rc.min,
            gfxm::vec2(rc.max.x, rc.min.y + text_box_height)
        );
        client_area = rc_bounds;

        text_content.prepareDraw(guiGetDefaultFont(), true);

        gfxm::rect text_rc = guiLayoutPlaceRectInsideRect(
            client_area, text_content.getBoundingSize(), GUI_LEFT | GUI_VCENTER,
            gfxm::rect(GUI_MARGIN, 0, 0, 0)
        );
        pos_content = text_rc.min;

    }
    void onDraw() override {
        Font* font = getFont();
        uint32_t col_box = GUI_COL_HEADER;
        if (isHovered()) {
            col_box = GUI_COL_BUTTON;
        }
        if (is_open) {
            guiDrawRectRound(client_area, GUI_PADDING * 2.f, col_box, GUI_DRAW_CORNER_TOP);
        } else {
            guiDrawRectRound(client_area, GUI_PADDING * 2.f, col_box);
        }
        float fontH = font->getLineHeight();
        float fontLG = font->getLineGap();
        if (current_icon) {
            current_icon->draw(gfxm::rect(gfxm::vec2(client_area.max.x - fontH * 2.f, pos_content.y), gfxm::vec2(client_area.max.x - fontH, pos_content.y + fontH)), GUI_COL_TEXT);
        }
        text_content.draw(font, pos_content, GUI_COL_TEXT, GUI_COL_ACCENT);
    }
};

class GuiComboBox : public GuiElement {
    GuiLabel label;
    GuiComboBoxCtrl ctrl;
public:
    GuiComboBox(const char* caption = "ComboBox", const char* text = "Select an item...")
        : label(caption), ctrl(text) {
        setSize(0, 0);
        setMaxSize(0, 0);
        setMinSize(0, 0);

        label.setOwner(this);
        label.setParent(this);
        ctrl.setOwner(this);
        ctrl.setParent(this);
    }
    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return;
        }

        ctrl.onHitTest(hit, x, y);
        if (hit.hasHit()) {
            return;
        }

        hit.add(GUI_HIT::CLIENT, this);
        return;
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        Font* font = getFont();

        const float text_box_height = font->getLineHeight() * 2.0f;
        setHeight(text_box_height);
        rc_bounds = gfxm::rect(
            rc.min,
            gfxm::vec2(rc.max.x, rc.min.y + text_box_height)
        );
        client_area = rc_bounds;

        gfxm::rect rc_left, rc_right;
        rc_left = client_area;
        guiLayoutSplitRect2XRatio(rc_left, rc_right, .25f);
        label.layout(rc_left, flags);
        ctrl.layout(rc_right, flags);
    }
    void onDraw() override {
        label.draw();
        ctrl.draw();
    }
};

#include "gui/elements/collapsing_header.hpp"

#include "gui/filesystem/gui_file_thumbnail.hpp"
class GuiFileListItem : public GuiElement {
    std::string item_name;
    GuiTextBuffer caption;
    bool is_selected = false;
    const guiFileThumbnail* thumb = 0;
public:
    bool is_directory = false;
    std::string path_canonical;

    GuiFileListItem(const char* cap = "FileListItem", const guiFileThumbnail* thumb = 0)
    : item_name(cap), thumb(thumb) {
        setSize(74 * 1.25, 96 * 1.25);
        caption.replaceAll(getFont(), cap, strlen(cap));
    }
    ~GuiFileListItem() {
        guiFileThumbnailRelease(thumb);
    }

    const std::string& getName() const {
        return item_name;
    }

    void setSelected(bool is_selected) {
        this->is_selected = is_selected;
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::LCLICK:
            notifyOwner<GuiFileListItem*>(GUI_NOTIFY::FILE_ITEM_CLICK, this);
            return true;
        case GUI_MSG::DBL_LCLICK:
            notifyOwner<GuiFileListItem*>(GUI_NOTIFY::FILE_ITEM_DOUBLE_CLICK, this);
            return true;
        case GUI_MSG::PULL_START: {
            auto rel_path = fsMakeRelativePath(fsGetCurrentDirectory(), path_canonical);
            guiDragStartFile(rel_path.c_str());
            return true;
        }
        case GUI_MSG::PULL_STOP:
            guiDragStop();
            return true;
        }
        return false;
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        Font* font = getFont();
        const float h = font->getLineHeight();
        gfxm::vec2 px_size = gui_to_px(size, font, gfxm::rect_size(rc));
        rc_bounds = gfxm::rect(rc.min, rc.min + px_size);
        client_area = rc_bounds;

        caption.setMaxLineWidth(font, 74 - 10);
        caption.prepareDraw(font, false);
    }
    void onDraw() override {
        if (isHovered()) {
            if (is_selected) {
                guiDrawRect(client_area, GUI_COL_BUTTON_HOVER);
            } else {
                guiDrawRect(client_area, GUI_COL_BUTTON);
            }
        } else if(is_selected) {
            guiDrawRect(client_area, GUI_COL_BUTTON_HOVER);
        }
        gfxm::rect rc_img = client_area;
        rc_img.min += gfxm::vec2(10.f, 5.f);
        rc_img.max -= gfxm::vec2(10.f, 5.f);
        rc_img.max.y = rc_img.min.y + (rc_img.max.x - rc_img.min.x);
        if (thumb) {
            guiFileThumbnailDraw(rc_img, thumb);
        } else {
            guiDrawRect(rc_img, GUI_COL_BUTTON_HOVER);
        }
        caption.draw(getFont(), gfxm::vec2(rc_img.min.x - 5.f, rc_img.max.y + GUI_MARGIN), GUI_COL_TEXT, GUI_COL_ACCENT);
    }
};

class GuiTreeItem : public GuiElement {
    GuiElement* head = 0;
    GuiIconElement* icon = 0;
    GuiTextElement* head_text = 0;
    GuiElement* content_box = 0;

    gfxm::rect rc_header;
    gfxm::rect rc_children;

    bool collapsed = true;
public:
    std::function<void(GuiTreeItem*)> on_click;

    GuiTreeItem(const char* cap = "TreeItem") {
        setSize(gui::perc(100), 0);
        padding = gfxm::rect(0, 0, 0, 0);
        overflow = GUI_OVERFLOW_FIT;

        setStyleClasses({ "tree-item" });
        
        {
            icon = new GuiIconElement;
            icon->setIcon(guiLoadIcon("svg/entypo/plus.svg"));
            icon->setSize(gui::em(1), gui::em(1));
            icon->padding = gfxm::rect(0, 0, 0, 0);
            icon->setHidden(true);

            head_text = new GuiTextElement;
            head_text->setContent(cap);
            head_text->addFlags(GUI_FLAG_SAME_LINE);
            head_text->padding = gfxm::rect(0, 0, 0, 0);
            //head->addStyleComponent(gui::style_background_color{ GUI_COL_ACCENT });
            //head->addStyleComponent(gui::style_border_radius{ 10.0f, .0f, 10.f, .0f });
            //head->addStyleComponent(gui::style_border{ .0f, 6.f, .0f, .0f, GUI_COL_RED, GUI_COL_RED, GUI_COL_RED, GUI_COL_RED });

            head = new GuiElement;
            head->setSize(gui::perc(100), gui::em(1.5));
            head->padding = gfxm::rect(0, 0, 0, 0);
            head->addStyleComponent(gui::style_color{ GUI_COL_TEXT });
            head->overflow = GUI_OVERFLOW_FIT;
            head->_addChild(icon);
            head->_addChild(head_text);
            //head->addFlags(GUI_FLAG_SELECTABLE);
            head->setStyleClasses({ "tree-item-head" });
        }

        content_box = new GuiElement;
        content_box->setSize(gui::perc(100), 0);
        content_box->padding = gfxm::rect(20,0,0,0);
        content_box->overflow = GUI_OVERFLOW_FIT;
        this->content = content_box;
        _addChild(head);
        _addChild(content_box);
        content_box->setHidden(collapsed);
    }

    void setCaption(const char* caption) {
        head_text->setContent(caption);
    }
    void setSelected(bool value) {
        head->setSelected(value);
    }
    GuiTreeItem* addItem(const char* name) {
        auto item = new GuiTreeItem(name);
        addChild(item);
        return item;
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::CHILD_ADDED:
        case GUI_MSG::CHILD_REMOVED: {
            if (content->childCount() == 0) {
                icon->setHidden(true);
            } else {
                icon->setHidden(false);
            }
            return true;
        }
        case GUI_MSG::LCLICK:
            notifyOwner<GuiTreeItem*>(GUI_NOTIFY::TREE_ITEM_CLICK, this);
            if (on_click) {
                on_click(this);
            }
            return true;
        case GUI_MSG::DBL_LCLICK:
            collapsed = !collapsed;
            content_box->setHidden(collapsed);
            if (collapsed) {
                icon->setIcon(guiLoadIcon("svg/entypo/plus.svg"));
            } else {
                icon->setIcon(guiLoadIcon("svg/entypo/minus.svg"));
            }
            return true;
        }
        return false;
    }
    void onDraw() override {/*
        if (isHovered()) {
            if (selected) {
                guiDrawRect(client_area, GUI_COL_BUTTON_HOVER);
            } else {
                guiDrawRect(client_area, GUI_COL_BUTTON);
            }
        } else if(selected) {
            guiDrawRect(client_area, GUI_COL_BUTTON_HOVER);
        }

        if (childCount() > 0) {            
            tree_indicator.draw(client_area.min, GUI_COL_TEXT, GUI_COL_ACCENT);
            caption.draw(rc_header.min, GUI_COL_TEXT, GUI_COL_ACCENT);
        } else {
            caption.draw(rc_header.min, GUI_COL_TEXT, GUI_COL_ACCENT);
        }

        if (!collapsed && childCount() > 0) {
            for (int i = 0; i < childCount(); ++i) {
                auto ch = getChild(i);
                ch->draw();
            }
        }*/
        /*
        gfxm::rect rc = head->getBoundingRect();
        if (head->isHovered()) {
            if (selected) {
                guiDrawRect(rc, GUI_COL_BUTTON_HOVER);
            } else {
                guiDrawRect(rc, GUI_COL_BUTTON);
            }
        } else if(selected) {
            guiDrawRect(rc, GUI_COL_BUTTON_HOVER);
        }
        rc.min.x += GUI_MARGIN;
        if (childCount() > 0) {            
            //tree_indicator.draw(client_area.min, GUI_COL_TEXT, GUI_COL_ACCENT);
            caption.draw(rc, GUI_LEFT | GUI_VCENTER, GUI_COL_TEXT, GUI_COL_ACCENT);
        } else {
            caption.draw(rc, GUI_LEFT | GUI_VCENTER, GUI_COL_TEXT, GUI_COL_ACCENT);
        }*/

        GuiElement::onDraw();
    }
};

class GuiFileContainer : public GuiElement {
    int visible_items_begin = 0;
    int visible_items_end = 0;
    gfxm::vec2 smooth_scroll = gfxm::vec2(.0f, .0f);

    std::unique_ptr<GuiScrollBarV> scroll_bar_v;
    gfxm::rect rc_scroll_v;
    gfxm::rect rc_scroll_h;

    std::vector<std::unique_ptr<GuiFileListItem>> items;
public:
    gfxm::vec2 scroll_offset = gfxm::vec2(.0f, .0f);

    GuiFileContainer() {
        setSize(0, 0);
        setMaxSize(0, 0);
        setMinSize(0, 0);

        scroll_bar_v.reset(new GuiScrollBarV());
        scroll_bar_v->setOwner(this);
    }

    GuiFileListItem* addItem(const char* name, bool is_dir, const guiFileThumbnail* thumb = 0) {
        auto ptr = new GuiFileListItem(name, thumb);
        ptr->is_directory = is_dir;
        items.emplace_back(std::unique_ptr<GuiFileListItem>(ptr));
        addChild(ptr);
        ptr->setOwner(this);
        return ptr;
    }
    void removeItem(GuiFileListItem* item) {
        // TODO:
    }
    void clearItems() {
        for (int i = 0; i < items.size(); ++i) {
            items[i]->setOwner(0);
            removeChild(items[i].get());
        }
        items.clear();
    }

    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return;
        }
        
        scroll_bar_v->onHitTest(hit, x, y);
        if (hit.hasHit()) {
            return;
        }

        for (int i = visible_items_begin; i < visible_items_end; ++i) {
            auto ch = getChild(i);
            ch->onHitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
        }

        hit.add(GUI_HIT::CLIENT, this);
        return;
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::MOUSE_SCROLL: {
            scroll_offset.y -= params.getA<int32_t>();
            scroll_bar_v->setOffset(scroll_offset.y);
            } return true;
        case GUI_MSG::SB_THUMB_TRACK:
            scroll_offset.y = params.getA<float>();
            return true;
        }
        return false;
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        rc_bounds = rc;
        client_area = rc_bounds;

        gfxm::expand(client_area, -GUI_MARGIN);

        Font* font = getFont();

        gfxm::vec2 item_size;
        if (!children.empty()) {
            item_size = gui_to_px(children[0]->size, font, getClientSize());
        }

        gfxm::vec2 elem_size(item_size.x + GUI_MARGIN, item_size.y);
        int elems_per_row = std::floorf((client_area.max.x - client_area.min.x) / elem_size.x);
        int visible_row_count = (client_area.max.y - client_area.min.y) / elem_size.y + 2;
        int total_row_count = 0;
        if (elems_per_row > 0) {
            total_row_count = std::ceilf(childCount() / (float)(elems_per_row));
        }

        float max_scroll = gfxm::_max(.0f, (total_row_count * elem_size.y) - (client_area.max.y - client_area.min.y));
        rc_content.min = client_area.min - pos_content;
        rc_content.max = rc_content.min + gfxm::vec2(elems_per_row * elem_size.x, total_row_count * elem_size.y);
        scroll_bar_v->setScrollBounds(.0f, getContentHeight());
        scroll_bar_v->setScrollPageLength(getContentHeight());

        scroll_offset.y = gfxm::_min(max_scroll, gfxm::_max(.0f, scroll_offset.y));
        smooth_scroll = gfxm::vec2(
            gfxm::lerp(smooth_scroll.x, scroll_offset.x, .4f),
            gfxm::lerp(smooth_scroll.y, scroll_offset.y, .4f)
        );

        visible_items_begin = gfxm::_min((int)childCount(), (int)(smooth_scroll.y / elem_size.y) * elems_per_row);
        visible_items_end = gfxm::_min((int)childCount(), visible_items_begin + visible_row_count * elems_per_row);
        int small_offset = smooth_scroll.y - (int)(smooth_scroll.y / elem_size.y) * elem_size.y;

        gfxm::vec2 cur = client_area.min - gfxm::vec2(.0f, small_offset);
        for (int i = visible_items_begin; i < visible_items_end; ++i) {
            auto ch = getChild(i);
            if ((client_area.max.x - cur.x) < elem_size.x) {
                cur.x = client_area.min.x;
                cur.y += elem_size.y;
            }
            gfxm::rect rc = client_area;
            rc.min = cur;
            ch->layout(rc, flags);
            cur.x += elem_size.x;
        }
        /*
        for (int i = 0; i < childCount(); ++i) {
            auto ch = getChild(i);
            ch->layout(rc_child, flags);

            rc_child.min.y += ch->getBoundingRect().max.y - ch->getBoundingRect().min.y;
        }*/

        if (getContentHeight() > getClientHeight()) {
            scroll_bar_v->setEnabled(true);
        } else {
            scroll_bar_v->setEnabled(false);
        }
        
        scroll_bar_v->layout(client_area, 0);
    }
    void onDraw() override {
        guiDrawRect(rc_bounds, GUI_COL_HEADER);
        guiDrawPushScissorRect(rc_bounds);
        for (int i = visible_items_begin; i < visible_items_end; ++i) {
            auto ch = getChild(i);
            ch->draw();
        }
        scroll_bar_v->draw();
        guiDrawPopScissorRect();
        //guiDrawRectLine(rc_content, GUI_COL_RED);
    }
};
class GuiContainerInner : public GuiElement {
    gfxm::vec2 smooth_scroll = gfxm::vec2(.0f, .0f);

    std::unique_ptr<GuiScrollBarV> scroll_bar_v;
    gfxm::rect rc_scroll_v;
    gfxm::rect rc_scroll_h;
public:
    gfxm::vec2 scroll_offset = gfxm::vec2(.0f, .0f);

    GuiContainerInner() {
        setSize(0, 0);
        setMaxSize(0, 0);
        setMinSize(0, 0);


        scroll_bar_v.reset(new GuiScrollBarV());
        scroll_bar_v->setOwner(this);
    }

    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return;
        }
        
        scroll_bar_v->onHitTest(hit, x, y);
        if (hit.hasHit()) {
            return;
        }
        for (int i = 0; i < childCount(); ++i) {
            auto ch = getChild(i);
            ch->onHitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
        }

        hit.add(GUI_HIT::CLIENT, this);
        return;
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::MOUSE_SCROLL: {
            scroll_offset.y -= params.getA<int32_t>();
            scroll_bar_v->setOffset(scroll_offset.y);
            } return true;
        case GUI_MSG::SB_THUMB_TRACK:
            scroll_offset.y = params.getA<float>();
            return true;
        }
        return false;
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        rc_bounds = rc;
        client_area = rc_bounds;

        gfxm::rect rc_content = client_area;
        gfxm::expand(rc_content, -GUI_MARGIN);
        gfxm::rect rc_child = rc_content;

        for (int i = 0; i < childCount(); ++i) {
            auto ch = getChild(i);
            ch->layout(rc_child, flags);

            rc_child.min.y += ch->getBoundingRect().max.y - ch->getBoundingRect().min.y;
        }

        gfxm::vec2 content_size(rc_content.max.x - rc_content.min.x, rc_child.min.y);
        if (content_size.y > rc_content.max.y - rc_content.min.y) {
            scroll_bar_v->setEnabled(true);
            //rc_content.max.x -= 10.0f;
            //content_size = updateContentLayout();
        } else {
            scroll_bar_v->setEnabled(false);
        }
        
        scroll_bar_v->layout(client_area, 0);
    }
    void onDraw() override {
        guiDrawRect(client_area, GUI_COL_HEADER);
        guiDrawPushScissorRect(client_area);
        for (int i = 0; i < childCount(); ++i) {
            auto ch = getChild(i);
            ch->draw();
        }
        scroll_bar_v->draw();
        guiDrawPopScissorRect();
    }
};

class GuiTreeView : public GuiElement {
    std::unique_ptr<GuiScrollBarV> scroll_bar_v;
    gfxm::vec2 scroll_offset = gfxm::vec2(.0f, .0f);
    GuiTreeItem* selected_item = 0;
public:
    GuiTreeView() {
        setSize(0, 250);
        setMaxSize(0, 0);
        setMinSize(0, 0);

        scroll_bar_v.reset(new GuiScrollBarV());
        scroll_bar_v->setOwner(this);

        addItem("Models");
        auto b = addItem("Textures");
        b->addItem("bricks.png");
        b->addItem("terrain")->addItem("grass.png");
        addItem("Shaders")->addItem("default.glsl");
    }

    GuiTreeItem* addItem(const char* name) {
        auto item = new GuiTreeItem(name);
        addChild(item);
        return item;
    }

    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return;
        }
        
        scroll_bar_v->onHitTest(hit, x, y);
        if (hit.hasHit()) {
            return;
        }
        for (int i = 0; i < childCount(); ++i) {
            auto ch = getChild(i);
            ch->onHitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
        }

        hit.add(GUI_HIT::CLIENT, this);
        return;
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::MOUSE_SCROLL: {
            scroll_offset.y -= params.getA<int32_t>();
            scroll_bar_v->setOffset(scroll_offset.y);
            return true;
        }
        case GUI_MSG::SB_THUMB_TRACK: {
            scroll_offset.y = params.getA<float>();
            return true;
        }
        case GUI_MSG::NOTIFY: {
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::TREE_ITEM_CLICK: {
                auto item = params.getB<GuiTreeItem*>();
                if (selected_item) {
                    selected_item->setSelected(false);
                }
                item->setSelected(true);
                selected_item = item;
                return true;
            }
            }
            return false;
        }
        }
        return false;
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        GuiElement::onLayout(rc, flags);
        /*
        rc_bounds = rc;
        client_area = rc_bounds;

        gfxm::rect rc_content = client_area;
        gfxm::expand(rc_content, -GUI_MARGIN);
        gfxm::rect rc_child = rc_content;

        for (int i = 0; i < childCount(); ++i) {
            auto ch = getChild(i);
            ch->layout(rc_child, flags);

            rc_child.min.y += ch->getBoundingRect().max.y - ch->getBoundingRect().min.y;
        }

        gfxm::vec2 content_size(rc_content.max.x - rc_content.min.x, rc_child.min.y);
        if (content_size.y > rc_content.max.y - rc_content.min.y) {
            scroll_bar_v->setEnabled(true);
            //rc_content.max.x -= 10.0f;
            //content_size = updateContentLayout();
        } else {
            scroll_bar_v->setEnabled(false);
        }
        
        scroll_bar_v->layout(client_area, 0);*/
    }
    void onDraw() override {
        guiDrawRect(client_area, GUI_COL_HEADER);
        guiDrawPushScissorRect(client_area);
        for (int i = 0; i < childCount(); ++i) {
            auto ch = getChild(i);
            ch->draw();
        }
        scroll_bar_v->draw();
        guiDrawPopScissorRect();
    }
};

class GuiListView : public GuiElement {
    GuiContainerInner* container = 0;
public:
    GuiListView() {
        container = new GuiContainerInner();
        addChild(container);

        container->addChild(new GuiListItem("List item 0"));
        container->addChild(new GuiListItem("List item 1"));
        container->addChild(new GuiListItem("List item 2"));
    }
    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return;
        }

        container->onHitTest(hit, x, y);
        return;
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        container->layout(rc, flags);
        rc_bounds = container->getBoundingRect();
        client_area = rc_bounds;
    }
};

class GuiDemoWindow : public GuiWindow {
public:
    GuiDemoWindow()
    : GuiWindow("DemoWindow") {
        setPosition(850, 200);
        setSize(400, 600);
        
        pushBack(R"(Then Fingolfin beheld (as it seemed to him) the utter ruin of the Noldor,
and the defeat beyond redress of all their houses;
and filled with wrath and despair he mounted upon Rochallor his great horse and rode forth alone,
and none might restrain him.)",
            { "paragraph" }
        );
        //pushBack("Hello, World!");
        
        GuiElement* head = new GuiElement;
        head->setSize(gui::perc(100), gui::em(7));
        head->setStyleClasses({ "control", "notification" });
        addChild(head);

        pushBack(R"(He passed over Dor-nu-Fauglith like a wind amid the dust,
and all that beheld his onset fled in amaze, thinking that Orome himself was come:
for a great madness of rage was upon him, so that his eyes shone like the eyes of the Valar.
Thus he came alone to Angband's gates, and he sounded his horn,
and smote once more upon the brazen doors,
and challenged Morgoth to come forth to single combat. And Morgoth came.)",
            { "paragraph" }
        );

        GuiTextElement* text = new GuiTextElement;
        text->setContent("Example notification");
        text->setStyleClasses({ "header" });
        GuiTextElement* text2 = new GuiTextElement;
        text2->setContent("Notification body");
        text2->setStyleClasses({ "paragraph" });
        head->addChild(text);
        head->addChild(text2);

        addChild(new GuiLabel("Hello, World!"));
        addChild(new GuiInputText());
        addChild(new GuiInputFloat("Float", 0, 2));
        addChild(new GuiInputFloat2("Float2", 0, 2));
        addChild(new GuiInputFloat3("Float3", 0, 2));
        addChild(new GuiInputFloat4("Float4", 0, 2));
        addChild(new GuiComboBox());
        addChild(new GuiCollapsingHeader("CollapsingHeader", true));
        addChild(new GuiCheckBox());
        addChild(new GuiRadioButton());
        addChild(new GuiButton("Button A"));
        addChild(new GuiButton("Button B"));
        addChild(new GuiTreeView());
    }
};

#include <shellapi.h>
#include <ShObjIdl.h>
#include "gui/filesystem/gui_file_thumbnail.hpp"
#include <filesystem>
class GuiFileExplorerWindow : public GuiWindow {
    std::unique_ptr<GuiInputTextLine> dir_path;
    std::unique_ptr<GuiTreeView> tree_view;
    std::unique_ptr<GuiFileContainer> container;

    std::experimental::filesystem::path current_path;
    std::vector<GuiFileListItem*> selected_items;
public:
    GuiFileExplorerWindow()
        : GuiWindow("FileExplorer") {
        setSize(800, 600);
        std::string sfname;
        sfname.resize(MAX_PATH);
        GetFullPathName(".", MAX_PATH, &sfname[0], 0);
        current_path = sfname;

        auto btn_back = new GuiIconButton(guiLoadIcon("svg/entypo/arrow-bold-left.svg"));
        addChild(btn_back);
        auto btn_forward = new GuiIconButton(guiLoadIcon("svg/entypo/arrow-bold-right.svg"));
        addChild(btn_forward);
        btn_forward->setFlags(GUI_FLAG_SAME_LINE);
        auto btn_up = new GuiIconButton(guiLoadIcon("svg/entypo/arrow-bold-up.svg"));
        addChild(btn_up);
        btn_up->setFlags(GUI_FLAG_SAME_LINE);

        dir_path.reset(new GuiInputTextLine);
        addChild(dir_path.get());
        dir_path->setText(sfname.c_str());
        dir_path->setFlags(GUI_FLAG_SAME_LINE);

        tree_view.reset(new GuiTreeView());
        tree_view->setOwner(this);
        tree_view->min_size.x = 0;
        tree_view->min_size.y = 100;
        tree_view->size.x = 300;
        tree_view->size.y = 0;
        addChild(tree_view.get());

        container.reset(new GuiFileContainer());
        container->setOwner(this);
        container->addFlags(GUI_FLAG_SAME_LINE);
        addChild(container.get());

        openDir(std::experimental::filesystem::current_path());
    }

    void openDir(const std::experimental::filesystem::path& path) {
        for (auto itm : selected_items) {
            itm->setSelected(false);
        }
        selected_items.clear();

        current_path = std::experimental::filesystem::absolute(path);
        dir_path->setText(path.string().c_str());
        container->clearItems();
        {
            struct file_t {
                std::string name;
                bool is_dir;
            };
            std::vector<file_t> files;
            {
                HANDLE hFind = INVALID_HANDLE_VALUE;
                WIN32_FIND_DATA ffd = { 0 };
                hFind = FindFirstFile(MKSTR(current_path << "\\*").c_str(), &ffd);
                if (hFind != INVALID_HANDLE_VALUE) {
                    while (FindNextFile(hFind, &ffd) != 0) {
                        file_t f;
                        f.name = ffd.cFileName;
                        f.is_dir = false;
                        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                            f.is_dir = true;
                        }
                        files.push_back(f);
                    }
                    FindClose(hFind);
                }
            }

            std::sort(files.begin() + 1, files.end(), [](const file_t& a, const file_t& b) {
                if (a.is_dir && b.is_dir) {
                    return a.name < b.name;
                } else if(a.is_dir || b.is_dir) {
                    return a.is_dir > b.is_dir;
                } else {
                    return a.name < b.name;
                }
            });
            for (auto& f : files) {                
                std::experimental::filesystem::path absolute_path;
                if (strncmp(f.name.c_str(), "..", 3) == 0) {
                    absolute_path = path.parent_path();
                } else {
                    absolute_path = path / f.name.c_str();
                }

                //LOG(absolute_path.string());

                const guiFileThumbnail* thumb = 0;
                if (absolute_path != path.root_path()) {
                    thumb = guiFileThumbnailLoad(
                        absolute_path.parent_path().string().c_str(),
                        absolute_path.filename().string().c_str(),
                        54
                    );
                } else {
                    thumb = guiFileThumbnailLoad(
                        absolute_path.string().c_str(),
                        0,
                        54
                    );
                }
                auto itm = container->addItem(f.name.c_str(), f.is_dir, thumb);
                itm->path_canonical = std::experimental::filesystem::canonical(absolute_path).string();
            }
        }
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::NOTIFY:
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::FILE_ITEM_CLICK: {
                auto item = params.getB<GuiFileListItem*>();
                for (auto itm : selected_items) {
                    itm->setSelected(false);
                }
                selected_items.clear();
                selected_items.push_back(item);
                item->setSelected(true);
                return true;
            }
            case GUI_NOTIFY::FILE_ITEM_DOUBLE_CLICK: {
                auto itm = (GuiFileListItem*)params.getB<GuiFileListItem*>();
                auto& name = itm->getName();
                if(itm->is_directory) {
                    std::experimental::filesystem::path path_new = current_path;
                    if (name == std::string("..")) {
                        path_new = current_path.parent_path();
                    } else if(name == std::string(".")) {
                        path_new = current_path;
                    } else {
                        path_new /= name;
                    }
                    openDir(path_new);
                } else {
                    guiSendMessage(this, GUI_MSG::FILE_EXPL_OPEN_FILE, itm, 0, 0);
                }
                return true;
            }
            }
            break;
        }
        return GuiWindow::onMessage(msg, params);
    }
};


constexpr float GUI_NODE_CIRCLE_RADIUS = 7.f;
class GuiNodeInput : public GuiElement {
    GuiTextBuffer caption;
    gfxm::vec2 circle_pos;
    bool is_filled = false;
public:
    GuiNodeInput() {
        caption.replaceAll(getFont(), "In", strlen("In"));
    }

    const gfxm::vec2& getCirclePosition() { return circle_pos; }
    void setFilled(bool b) {
        is_filled = b;
    }

    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!guiHitTestCircle(circle_pos, GUI_NODE_CIRCLE_RADIUS * 1.5f, gfxm::vec2(x, y))) {
            return;
        }
        hit.add(GUI_HIT::CLIENT, this);
        return;
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::LCLICK:
            assert(getOwner());
            if (getOwner()) {
                getOwner()->notify<GuiNodeInput*>(GUI_NOTIFY::NODE_INPUT_CLICKED, this);
            }
            return true;
        case GUI_MSG::RBUTTON_DOWN:
            assert(getOwner());
            if (getOwner()) {
                getOwner()->notify<GuiNodeInput*>(GUI_NOTIFY::NODE_INPUT_BREAK, this);
            }
            return true;
        }
        return false;
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        rc_bounds = gfxm::rect(
            rc.min,
            gfxm::vec2(rc.max.x, rc.min.y + 20.0f)
        );
        client_area = rc_bounds;

        caption.prepareDraw(getFont(), false);

        circle_pos = gfxm::vec2(
            client_area.min.x,
            client_area.center().y
        );
    }
    void onDraw() override {
        if (is_filled) {
            guiDrawCircle(circle_pos, GUI_NODE_CIRCLE_RADIUS, true);
        } else {
            guiDrawCircle(circle_pos, GUI_NODE_CIRCLE_RADIUS, true, GUI_COL_BLACK);
            guiDrawCircle(circle_pos, GUI_NODE_CIRCLE_RADIUS, false);
        }
        if (isHovered()) {
            guiDrawCircle(circle_pos, GUI_NODE_CIRCLE_RADIUS * 1.5f);
        }
        caption.draw(getFont(), gfxm::vec2(client_area.min.x + GUI_NODE_CIRCLE_RADIUS + GUI_MARGIN, caption.findCenterOffsetY(client_area)), GUI_COL_TEXT, GUI_COL_TEXT);
    }
};
class GuiNodeOutput : public GuiElement {
    GuiTextBuffer caption;
    gfxm::vec2 circle_pos;
    bool is_filled = false;
public:
    GuiNodeOutput() {
        caption.replaceAll(getFont(), "Out", strlen("Out"));
    }

    const gfxm::vec2& getCirclePosition() { return circle_pos; }
    void setFilled(bool b) {
        is_filled = b;
    }

    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!guiHitTestCircle(circle_pos, GUI_NODE_CIRCLE_RADIUS * 1.5f, gfxm::vec2(x, y))) {
            return;
        }
        hit.add(GUI_HIT::CLIENT, this);
        return;
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::LCLICK:
            assert(getOwner());
            if (getOwner()) {
                getOwner()->notify<GuiNodeOutput*>(GUI_NOTIFY::NODE_OUTPUT_CLICKED, this);
            }
            return true;
        case GUI_MSG::RBUTTON_DOWN:
            assert(getOwner());
            if (getOwner()) {
                getOwner()->notify<GuiNodeOutput*>(GUI_NOTIFY::NODE_OUTPUT_BREAK, this);
            }
            return true;
        }
        return false;
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        rc_bounds = gfxm::rect(
            rc.min,
            gfxm::vec2(rc.max.x, rc.min.y + 20.0f)
        );
        client_area = rc_bounds;

        caption.prepareDraw(getFont(), false);

        circle_pos = gfxm::vec2(
            client_area.max.x,
            client_area.center().y
        );
    }
    void onDraw() override {
        if (is_filled) {
            guiDrawCircle(circle_pos, GUI_NODE_CIRCLE_RADIUS, true);
        } else {
            guiDrawCircle(circle_pos, GUI_NODE_CIRCLE_RADIUS, true, GUI_COL_BLACK);
            guiDrawCircle(circle_pos, GUI_NODE_CIRCLE_RADIUS, false);
        }
        if (isHovered()) {
            guiDrawCircle(circle_pos, GUI_NODE_CIRCLE_RADIUS * 1.5f);
        }
        caption.draw(
            getFont(),
            gfxm::vec2(
                client_area.max.x - caption.getBoundingSize().x - 7.f - GUI_MARGIN, 
                caption.findCenterOffsetY(client_area)
            ), GUI_COL_TEXT, GUI_COL_TEXT
        );
    }
};
class GuiNode : public GuiElement {
    GuiTextBuffer caption;

    uint32_t color = GUI_COL_RED;
    gfxm::rect rc_title;
    gfxm::rect rc_body;
    std::vector<std::unique_ptr<GuiNodeInput>> inputs;
    std::vector<std::unique_ptr<GuiNodeOutput>> outputs;

    bool is_selected = false;

    gfxm::vec2 last_mouse_pos;

    void requestSelectedState() {
        assert(getOwner());
        if (getOwner()) {
            getOwner()->sendMessage(GUI_MSG::NOTIFY, (uint64_t)GUI_NOTIFY::NODE_CLICKED, (uint64_t)this);
        }
    }
public:
    GuiNode() {
        caption.replaceAll(getFont(), "NodeName", strlen("NodeName"));
    }

    void setColor(uint32_t color) {
        this->color = color;
    }
    void setPosition(float x, float y) {
        pos = gui_vec2(x, y);
    }
    void setSelected(bool b) {
        is_selected = b;
    }

    GuiNodeInput* createInput() {
        auto ptr = new GuiNodeInput();
        ptr->setOwner(this);
        inputs.push_back(std::unique_ptr<GuiNodeInput>(ptr));
        return ptr;
    }
    GuiNodeOutput* createOutput() {
        auto ptr = new GuiNodeOutput();
        ptr->setOwner(this);
        outputs.push_back(std::unique_ptr<GuiNodeOutput>(ptr));
        return ptr;
    }
    int getInputCount() const { return inputs.size(); }
    int getOutputCount() const { return outputs.size(); }
    GuiNodeInput* getInput(int i) { return inputs[i].get(); }
    GuiNodeOutput* getOutput(int i) { return outputs[i].get(); }

    void onHitTest(GuiHitResult& hit, int x, int y) override {
        gfxm::rect expanded_client_rc = client_area;
        gfxm::expand(expanded_client_rc, 10.0f);
        if (!guiHitTestRect(expanded_client_rc, gfxm::vec2(x, y))) {
            return;
        }

        for (int i = 0; i < inputs.size(); ++i) {
            inputs[i]->onHitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
        }
        for (int i = 0; i < outputs.size(); ++i) {
            outputs[i]->onHitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
        }

        if (!guiHitTestRect(client_area, gfxm::vec2(x, y))) {
            return;
        }

        hit.add(GUI_HIT::CLIENT, this);
        return;
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::MOUSE_MOVE:
            if (isPulled()) {
                if (!is_selected) {
                    requestSelectedState();
                }
                gfxm::vec2 mouse_pos(params.getA<int32_t>(), params.getB<int32_t>());
                gfxm::vec2 diff = (mouse_pos - last_mouse_pos) * getContentViewScale();
                pos.x.value += diff.x;
                pos.y.value += diff.y;
            }
            last_mouse_pos = gfxm::vec2(params.getA<int32_t>(), params.getB<int32_t>());
            return true;
        case GUI_MSG::LCLICK:
            requestSelectedState();
            return true;
        }
        return false;
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        rc_bounds = gfxm::rect(
            rc.min,
            rc.min + gfxm::vec2(200, 300)
        );
        client_area = rc_bounds;
        rc_title = gfxm::rect(
            rc_bounds.min,
            gfxm::vec2(rc_bounds.max.x, rc_bounds.min.y + 30.0f)
        );
        rc_body = gfxm::rect(
            gfxm::vec2(rc_bounds.min.x, rc_bounds.min.y + 30.0f),
            rc_bounds.max
        );

        caption.prepareDraw(getFont(), false);

        float y_latest = rc_title.max.y + GUI_MARGIN;
        gfxm::vec2 cur = rc_body.min + gfxm::vec2(.0f, GUI_MARGIN);
        for (int i = 0; i < childCount(); ++i) {
            auto ch = getChild(i);
            ch->layout(rc_body, flags);
            cur.y += (ch->getBoundingRect().max.y - ch->getBoundingRect().min.y) + GUI_MARGIN;
            y_latest = ch->getBoundingRect().max.y;
        }
        for (int i = 0; i < inputs.size(); ++i) {
            inputs[i]->onLayout(rc_body, flags);
            cur.y 
                += (inputs[i]->getBoundingRect().max.y - inputs[i]->getBoundingRect().min.y)
                + GUI_MARGIN;
            y_latest = inputs[i]->getBoundingRect().max.y;
        }
        for (int i = 0; i < outputs.size(); ++i) {
            outputs[i]->onLayout(rc_body, flags);
            cur.y
                += (outputs[i]->getBoundingRect().max.y - outputs[i]->getBoundingRect().min.y)
                + GUI_MARGIN;
            y_latest = outputs[i]->getBoundingRect().max.y;
        }
        rc_body.max.y = y_latest + GUI_MARGIN;
        rc_bounds.max.y = rc_body.max.y;
        client_area.max.y = rc_body.max.y;
    }
    void onDraw() override {
        gfxm::vec2 shadow_offset(.0f, 10.0f);
        guiDrawRectRoundBorder(gfxm::rect(
            rc_title.min + shadow_offset, rc_body.max + shadow_offset
        ), 20.0f, 10.0f, 0xAA000000, 0x00000000);
        guiDrawRectRound(gfxm::rect(
            rc_title.min + shadow_offset, rc_body.max + shadow_offset
        ), 20.0f, 0xAA000000);

        guiDrawRectRound(rc_title, 20.0f, color, GUI_DRAW_CORNER_TOP);
        caption.draw(getFont(), gfxm::vec2(rc_title.min.x + 10.0f, caption.findCenterOffsetY(rc_title)), GUI_COL_TEXT, GUI_COL_ACCENT);

        guiDrawRectRound(rc_body, 20.0f, GUI_COL_BG, GUI_DRAW_CORNER_BOTTOM);
        if (is_selected) {
            guiDrawRectRoundBorder(gfxm::rect(
                rc_title.min, rc_body.max
            ), 20.0f, 10.0f, GUI_COL_WHITE, 0x00000000);
        }

        for (int i = 0; i < childCount(); ++i) {
            getChild(i)->draw();
        }
        for (int i = 0; i < inputs.size(); ++i) {
            inputs[i]->draw();
        }
        for (int i = 0; i < outputs.size(); ++i) {
            outputs[i]->draw();
        }
    }
};

struct GuiNodeLink {
    GuiNodeOutput* from;
    GuiNodeInput* to;
};
struct GuiNodeLinkPreview {
    GuiNodeOutput* out;
    GuiNodeInput* in;
    gfxm::vec2 from;
    gfxm::vec2 to;
    bool is_active;
    bool is_output;
};
class GuiNodeContainer : public GuiElement {
    std::vector<std::unique_ptr<GuiNode>> nodes;
    std::vector<GuiNodeLink> links;
    GuiNodeLinkPreview link_preview;
    gfxm::vec2 last_mouse_pos;

    GuiNode* selected_node = 0;

    void startLinkPreview(GuiNodeOutput* out, GuiNodeInput* in, const gfxm::vec2& from, bool is_output) {
        assert(out == 0 || in == 0);
        if (out) {
            link_preview.out = out;
            link_preview.in = 0;
        } else if(in) {
            link_preview.out = 0;
            link_preview.in = in;
        } else {
            assert(false);
            return;
        }
        link_preview.is_active = true;
        link_preview.from = from;
        link_preview.to = last_mouse_pos;
        link_preview.is_output = is_output;
    }
    void stopLinkPreview() {
        link_preview.is_active = false;
    }

    void setSelectedNode(GuiNode* node) {
        if (selected_node) {
            selected_node->setSelected(false);
        }
        selected_node = node;
        if (selected_node) {
            selected_node->setSelected(true);
        }
    }
public:
    GuiNodeContainer() {
        setSize(0, 0);
        stopLinkPreview();
    }

    GuiNode* createNode() {
        auto ptr = new GuiNode();
        ptr->setOwner(this);
        nodes.push_back(std::unique_ptr<GuiNode>(ptr));
        return ptr;
    }
    void destroyNode(GuiNode* node) {
        for (int i = 0; i < nodes.size(); ++i) {
            if (nodes[i].get() == node) {
                nodes.erase(nodes.begin() + i);
                break;
            }
        }
    }

    void link(GuiNode* node_a, int nout, GuiNode* node_b, int nin) {
        auto out = node_a->getOutput(nout);
        auto in = node_b->getInput(nin);
        link(out, in);
    }
    void link(GuiNodeOutput* out, GuiNodeInput* in) {
        assert(in != nullptr && out != nullptr);
        linkBreak(in);
        out->setFilled(true);
        in->setFilled(true);
        links.push_back(GuiNodeLink{ out, in });
    }
    void linkBreak(GuiNodeOutput* out) {
        for (int i = 0; i < links.size(); ++i) {
            if (links[i].from == out) {
                links[i].from->setFilled(false);
                links[i].to->setFilled(false);
                links.erase(links.begin() + i);
                --i;
            }
        }
    }
    void linkBreak(GuiNodeInput* in) {
        for (int i = 0; i < links.size(); ++i) {
            if (links[i].to == in) {
                links[i].from->setFilled(false);
                links[i].to->setFilled(false);
                links.erase(links.begin() + i);
                break;
            }
        }
    }

    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return;
        }
        
        //if (!link_preview.is_active) {
            guiPushViewTransform(getContentViewTransform());
            for (int i = nodes.size() - 1; i >= 0; --i) {
                auto ch = nodes[i].get();
                ch->onHitTest(hit, x, y);
                if (hit.hasHit()) {
                    return;
                }
            }
            guiPopViewTransform();
        //}

        hit.add(GUI_HIT::CLIENT, this);
        return;
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::LCLICK: {
            setSelectedNode(0);
            stopLinkPreview();
            } return true;
        case GUI_MSG::MOUSE_SCROLL: {
            gfxm::vec2 mouse2_lcl = guiGetMousePos();
            gfxm::vec3 mouse_lcl(
                mouse2_lcl.x,
                mouse2_lcl.y,
                .0f
            );
            if (params.getA<int32_t>() < .0f) {
                float new_scale = getContentViewScale() * 1.1f;
                setContentViewTranslation(
                    getContentViewTranslation() - mouse_lcl * getContentViewScale() * .1f
                );
                setContentViewScale(new_scale);
            } else {
                float new_scale = getContentViewScale() * 0.9f;
                setContentViewTranslation(
                    getContentViewTranslation() + mouse_lcl * getContentViewScale() * .1f
                );
                setContentViewScale(new_scale);
                
            }
            } return true;
        case GUI_MSG::SB_THUMB_TRACK:
            // TODO: scroll
            return true;
        case GUI_MSG::MOUSE_MOVE:
            if (isPulled()) {
                gfxm::vec2 mouse_pos(params.getA<int32_t>(), params.getB<int32_t>());
                gfxm::vec2 diff = (mouse_pos - last_mouse_pos) * getContentViewScale();
                translateContentView(-diff.x, -diff.y);
            }
            last_mouse_pos = gfxm::vec2(params.getA<int32_t>(), params.getB<int32_t>());
            return true;
        case GUI_MSG::UNFOCUS:
            stopLinkPreview();
            return true;
        case GUI_MSG::RBUTTON_DOWN:
            stopLinkPreview();
            return true;
        case GUI_MSG::NOTIFY: {
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::NODE_CLICKED: {
                GuiNode* node = params.getB<GuiNode*>();
                setSelectedNode(node);
                }return true;
            case GUI_NOTIFY::NODE_INPUT_CLICKED: {
                GuiNodeInput* inp = params.getB<GuiNodeInput*>();
                if (link_preview.is_active && link_preview.is_output) {
                    link(link_preview.out, inp);
                    stopLinkPreview();
                } else {
                    startLinkPreview(0, inp, inp->getCirclePosition(), false);
                }
                }return true;
            case GUI_NOTIFY::NODE_OUTPUT_CLICKED: {
                GuiNodeOutput* out = params.getB<GuiNodeOutput*>();
                if (link_preview.is_active && !link_preview.is_output) {
                    link(out, link_preview.in);
                    stopLinkPreview();
                } else {
                    startLinkPreview(out, 0, out->getCirclePosition(), true);
                }
                }return true;
            case GUI_NOTIFY::NODE_INPUT_BREAK: {
                GuiNodeInput* in = params.getB<GuiNodeInput*>();
                linkBreak(in);
                }return true;
            case GUI_NOTIFY::NODE_OUTPUT_BREAK: {
                GuiNodeOutput* out = params.getB<GuiNodeOutput*>();
                linkBreak(out);
                }return true;
            }            
            }break;
        }
        return false;
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        Font* font = getFont();

        rc_bounds = rc;
        client_area = rc_bounds;

        gfxm::rect rc_content = client_area;
        gfxm::expand(rc_content, -GUI_MARGIN);
        
        for (int i = 0; i < nodes.size(); ++i) {
            auto n = nodes[i].get();
            gfxm::vec2 px_pos = gui_to_px(n->pos, font, getClientSize());
            gfxm::vec2 px_size = gui_to_px(n->size, font, getClientSize());
            gfxm::rect rc_ = gfxm::rect(px_pos, px_pos + px_size);
            n->onLayout(rc_, flags);
        }

        if (link_preview.is_active) {
            gfxm::vec2 mpos = guiGetMousePos();
            gfxm::vec3 mpos3 = gfxm::inverse(getContentViewTransform()) * gfxm::vec4(mpos.x, mpos.y, .0f, 1.f);
            mpos = gfxm::vec2(mpos3.x, mpos3.y);
            link_preview.to = mpos;
        }
    }
    void onDraw() override {
        guiDrawRect(client_area, GUI_COL_HEADER);
        guiDrawPushScissorRect(client_area);

        guiPushViewTransform(getContentViewTransform());

        for (int i = 0; i < links.size(); ++i) {
            guiDrawCurveSimple(
                links[i].from->getCirclePosition(),
                links[i].to->getCirclePosition(), 3.f
            );
        }
        for (int i = 0; i < nodes.size(); ++i) {
            auto n = nodes[i].get();
            n->draw();
        }

        if (link_preview.is_active) {
            guiDrawCurveSimple(
                link_preview.from,
                link_preview.to, 3.0f, 0xFF777777
            );
        }
        guiPopViewTransform();

        // TODO
        guiDrawPopScissorRect();
    }
};

class GuiNodeEditorWindow : public GuiWindow {
    GuiNodeContainer* node_container;
public:
    GuiNodeEditorWindow()
        : GuiWindow("NodeEditor") {
        setSize(800, 600);

        node_container = (new GuiNodeContainer);
        addChild(node_container);
        auto node0 = node_container->createNode();
        node0->createInput();
        node0->createInput();
        node0->createInput();
        node0->createOutput();
        node0->setPosition(450, 200);
        node0->setColor(0xFF352A53);
        node0->addChild(new GuiLabel("Any gui element can be\ndisplayed inside a node"));
        auto node1 = node_container->createNode();
        node1->createOutput();
        node1->setPosition(100, 100);
        node1->setColor(0xFF4F5A63);
        auto node2 = node_container->createNode();
        node2->createInput();
        node2->setPosition(620, 400);
        node2->setColor(0xFF828CAA);
        auto node3 = node_container->createNode();
        node3->createOutput();
        node3->setPosition(100, 400);
        node3->setColor(0xFF4F5A63);
        //node_container->link(node1, 0, node0, 0);
        //node_container->link(node1, 0, node0, 1);
        //node_container->link(node1, 0, node0, 2);
    }
};

#include "gui/elements/animation/gui_timeline_editor.hpp"