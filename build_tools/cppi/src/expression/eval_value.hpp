#pragma once

#include <stdint.h>


struct eval_value {
    enum type_t {
        e_empty,
        e_bool,
        e_null,
        e_int8,
        e_uint8,
        e_int16,
        e_uint16,
        e_int32,
        e_uint32,
        e_int64,
        e_uint64,
        e_float,
        e_double,
        e_long_double,
        e_string
    } type;

    union {
        int8_t      int8;
        int16_t     int16;
        int32_t     int32;
        int64_t     int64;
        uint8_t     uint8;
        uint16_t    uint16;
        uint32_t    uint32;
        uint64_t    uint64;
        float       float_;
        double      double_;
        long double long_double;
        bool        boolean;
    };

    eval_value()
        : type(e_empty) {}
    eval_value(std::nullptr_t n)
        : type(e_null) {}
    eval_value(int8_t v)
        : type(e_int8), int8(v) {}
    eval_value(int16_t v)
        : type(e_int16), int16(v) {}
    eval_value(int32_t v)
        : type(e_int32), int32(v) {}
    eval_value(int64_t v)
        : type(e_int64), int64(v) {}
    eval_value(uint8_t v)
        : type(e_uint8), uint8(v) {}
    eval_value(uint16_t v)
        : type(e_uint16), uint16(v) {}
    eval_value(uint32_t v)
        : type(e_uint32), uint32(v) {}
    eval_value(uint64_t v)
        : type(e_uint64), uint64(v) {}
    eval_value(float v)
        : type(e_float), float_(v) {}
    eval_value(double v)
        : type(e_double), double_(v) {}
    eval_value(long double v)
        : type(e_long_double), long_double(v) {}
    eval_value(bool v)
        : type(e_bool), boolean(v) {}

    bool is_floating() const {
        return type == e_float || type == e_double || type == e_long_double;
    }
    bool is_integral() const {
        return
            type == e_int8 ||
            type == e_int16 ||
            type == e_int32 ||
            type == e_int64 ||
            type == e_uint8 ||
            type == e_uint16 ||
            type == e_uint32 ||
            type == e_uint64;
    }
    bool is_signed() const {
        return 
            type == e_int8 ||
            type == e_int16 ||
            type == e_int32 ||
            type == e_int64;
    }

    void dbg_print() const {
        switch (type) {
        case e_empty: dbg_printf_color("[empty]", DBG_WHITE); break; 
        case e_bool: dbg_printf_color("%s", DBG_KEYWORD, boolean ? "true" : "false"); break;
        case e_null: dbg_printf_color("nullptr", DBG_WHITE); break;
        case e_int8: dbg_printf_color("%i", DBG_WHITE, int8); break;
        case e_uint8: dbg_printf_color("%u", DBG_WHITE, uint8); break;
        case e_int16: dbg_printf_color("%i", DBG_WHITE, int16); break;
        case e_uint16: dbg_printf_color("%u", DBG_WHITE, uint16); break;
        case e_int32: dbg_printf_color("%i", DBG_WHITE, int32); break;
        case e_uint32: dbg_printf_color("%u", DBG_WHITE, uint32); break;
        case e_int64: dbg_printf_color("%i", DBG_WHITE, int64); break;
        case e_uint64: dbg_printf_color("%u", DBG_WHITE, uint64); break;
        case e_float: dbg_printf_color("%f", DBG_WHITE, float_); break;
        case e_double: dbg_printf_color("%", DBG_WHITE, double_); break;
        case e_long_double: dbg_printf_color("%", DBG_WHITE, long_double); break;
        case e_string: dbg_printf_color("[string]", DBG_WHITE); break;
        default:
            dbg_printf_color("[not_implemented]", DBG_WHITE);
        }
    }
};

inline bool eval_convert_promote(eval_value::type_t type, eval_value* v) {
    switch (type) {
    case eval_value::e_int8:
        switch (v->type) {
        case eval_value::e_bool: v->int8 = v->boolean; break;
        case eval_value::e_null: v->int8 = 0; break;
        }
        break;
    case eval_value::e_uint8:
        switch (v->type) {
        case eval_value::e_bool: v->uint8 = v->boolean; break;
        case eval_value::e_null: v->uint8 = 0; break;
        case eval_value::e_int8: v->uint8 = v->int8; break;
        }
        break;
    case eval_value::e_int16:
        switch (v->type) {
        case eval_value::e_bool: v->int16 = v->boolean; break;
        case eval_value::e_null: v->int16 = 0; break;
        case eval_value::e_int8: v->int16 = v->int8; break;
        case eval_value::e_uint8: v->int16 = v->uint8; break;
        }
        break;
    case eval_value::e_uint16:
        switch (v->type) {
        case eval_value::e_bool: v->uint16 = v->boolean; break;
        case eval_value::e_null: v->uint16 = 0; break;
        case eval_value::e_int8: v->uint16 = v->int8; break;
        case eval_value::e_uint8: v->uint16 = v->uint8; break;
        case eval_value::e_int16: v->uint16 = v->int16; break;
        }
        break;
    case eval_value::e_int32:
        switch (v->type) {
        case eval_value::e_bool: v->int32 = v->boolean; break;
        case eval_value::e_null: v->int32 = 0; break;
        case eval_value::e_int8: v->int32 = v->int8; break;
        case eval_value::e_uint8: v->int32 = v->uint8; break;
        case eval_value::e_int16: v->int32 = v->int16; break;
        case eval_value::e_uint16: v->int32 = v->uint16; break;
        }
        break;
    case eval_value::e_uint32:
        switch (v->type) {
        case eval_value::e_bool: v->uint32 = v->boolean; break;
        case eval_value::e_null: v->uint32 = 0; break;
        case eval_value::e_int8: v->uint32 = v->int8; break;
        case eval_value::e_uint8: v->uint32 = v->uint8; break;
        case eval_value::e_int16: v->uint32 = v->int16; break;
        case eval_value::e_uint16: v->uint32 = v->uint16; break;
        case eval_value::e_int32: v->uint32 = v->int32; break;
        }
        break;
    case eval_value::e_int64:
        switch (v->type) {
        case eval_value::e_bool: v->int64 = v->boolean; break;
        case eval_value::e_null: v->int64 = 0; break;
        case eval_value::e_int8: v->int64 = v->int8; break;
        case eval_value::e_uint8: v->int64 = v->uint8; break;
        case eval_value::e_int16: v->int64 = v->int16; break;
        case eval_value::e_uint16: v->int64 = v->uint16; break;
        case eval_value::e_int32: v->int64 = v->int32; break;
        case eval_value::e_uint32: v->int64 = v->uint32; break;
        }
        break;
    case eval_value::e_uint64:
        switch (v->type) {
        case eval_value::e_bool: v->uint64 = v->boolean; break;
        case eval_value::e_null: v->uint64 = 0; break;
        case eval_value::e_int8: v->uint64 = v->int8; break;
        case eval_value::e_uint8: v->uint64 = v->uint8; break;
        case eval_value::e_int16: v->uint64 = v->int16; break;
        case eval_value::e_uint16: v->uint64 = v->uint16; break;
        case eval_value::e_int32: v->uint64 = v->int32; break;
        case eval_value::e_uint32: v->uint64 = v->uint32; break;
        case eval_value::e_int64: v->uint64 = v->int64; break;
        }
        break;
    case eval_value::e_float:
        switch (v->type) {
        case eval_value::e_bool: v->float_ = v->boolean; break;
        case eval_value::e_null: v->float_ = 0; break;
        case eval_value::e_int8: v->float_ = v->int8; break;
        case eval_value::e_uint8: v->float_ = v->uint8; break;
        case eval_value::e_int16: v->float_ = v->int16; break;
        case eval_value::e_uint16: v->float_ = v->uint16; break;
        case eval_value::e_int32: v->float_ = v->int32; break;
        case eval_value::e_uint32: v->float_ = v->uint32; break;
        case eval_value::e_int64: v->float_ = v->int64; break;
        case eval_value::e_uint64: v->float_ = v->uint64; break;
        }
        break;
    case eval_value::e_double:
        switch (v->type) {
        case eval_value::e_bool: v->double_ = v->boolean; break;
        case eval_value::e_null: v->double_ = 0; break;
        case eval_value::e_int8: v->double_ = v->int8; break;
        case eval_value::e_uint8: v->double_ = v->uint8; break;
        case eval_value::e_int16: v->double_ = v->int16; break;
        case eval_value::e_uint16: v->double_ = v->uint16; break;
        case eval_value::e_int32: v->double_ = v->int32; break;
        case eval_value::e_uint32: v->double_ = v->uint32; break;
        case eval_value::e_int64: v->double_ = v->int64; break;
        case eval_value::e_uint64: v->double_ = v->uint64; break;
        case eval_value::e_float: v->double_ = v->float_; break;
        }
        break;
    case eval_value::e_long_double:
        switch (v->type) {
        case eval_value::e_bool: v->long_double = v->boolean; break;
        case eval_value::e_null: v->long_double = 0; break;
        case eval_value::e_int8: v->long_double = v->int8; break;
        case eval_value::e_uint8: v->long_double = v->uint8; break;
        case eval_value::e_int16: v->long_double = v->int16; break;
        case eval_value::e_uint16: v->long_double = v->uint16; break;
        case eval_value::e_int32: v->long_double = v->int32; break;
        case eval_value::e_uint32: v->long_double = v->uint32; break;
        case eval_value::e_int64: v->long_double = v->int64; break;
        case eval_value::e_uint64: v->long_double = v->uint64; break;
        case eval_value::e_float: v->long_double = v->float_; break;
        case eval_value::e_double: v->long_double = v->double_; break;
        }
        break;
    default:
        return false;
    }
    return true;
}

inline void eval_convert_sort(eval_value*& a, eval_value*& b) {
    if (a->type < b->type) {
        std::swap(a, b);
    }
}

inline bool eval_convert_operands(eval_value& l, eval_value& r) {
    eval_value* a = &l;
    eval_value* b = &r;
    eval_convert_sort(a, b);
    if (!eval_convert_promote(a->type, b)) {
        return false;
    }
    return true;
}

