#pragma once

#include <assert.h>
#include "reflection/reflection.hpp"
#include "log/log.hpp"


enum VALUE_TYPE {
    VALUE_FLOAT,
    VALUE_FLOAT2,
    VALUE_FLOAT3,
    VALUE_INT,
    VALUE_INT2,
    VALUE_INT3,
    VALUE_BOOL
};
struct value_ {
    VALUE_TYPE type;
    union {
        float       float_;
        gfxm::vec2  float2;
        gfxm::vec3  float3;
        int         int_;
        gfxm::ivec2 int2;
        gfxm::ivec3 int3;
        bool        bool_;
    };
    value_() {}
    value_(float v) : type(VALUE_FLOAT), float_(v) {}
    value_(gfxm::vec2 v) : type(VALUE_FLOAT2), float2(v) {}
    value_(gfxm::vec3 v) : type(VALUE_FLOAT3), float3(v) {}
    value_(int v) : type(VALUE_INT), int_(v) {}
    value_(gfxm::ivec2 v) : type(VALUE_INT2), int2(v) {}
    value_(gfxm::ivec3 v) : type(VALUE_INT3), int3(v) {}
    value_(bool v) : type(VALUE_BOOL), bool_(v) {}

    float to_float() const {
        if (type != VALUE_FLOAT) {
            assert(false);
            return .0f;
        }
        return float_;
    }

    bool convert(VALUE_TYPE type, value_& out) const {
        switch (this->type) {
        case VALUE_FLOAT:
            switch (type) {
            case VALUE_FLOAT: out = *this; return true;
            case VALUE_INT: out = value_(int(float_)); return true;
            case VALUE_BOOL: out = value_(float_ != .0f); return true;
            }
            break;
        case VALUE_FLOAT2:
            switch (type) {
            case VALUE_FLOAT2: out = *this; return true;
            case VALUE_INT2: out = value_(gfxm::ivec2(float2.x, float2.y)); return true;
            }
            break;
        case VALUE_FLOAT3:
            switch (type) {
            case VALUE_FLOAT3: out = *this; return true;
            case VALUE_INT3: out = value_(gfxm::ivec3(float3.x, float3.y, float3.z)); return true;
            }
            break;
        case VALUE_INT:
            switch (type) {
            case VALUE_INT: out = value_(*this); return true;
            case VALUE_FLOAT: out = value_(float(int_)); return true;
            case VALUE_BOOL: out = value_(int_ != 0); return true;
            }
            break;
        case VALUE_INT2:
            switch (type) {
            case VALUE_INT2: out = *this; return true;
            case VALUE_FLOAT2: out = value_(gfxm::vec2(int2.x, int2.y)); return true;
            }
            break;
        case VALUE_INT3:
            switch (type) {
            case VALUE_INT3: out = *this; return true;
            case VALUE_FLOAT3: out = value_(gfxm::vec3(int3.x, int3.y, int3.z)); return true;
            }
            break;
        case VALUE_BOOL:
            switch (type) {
            case VALUE_BOOL: out = *this; return true;
            case VALUE_INT: out = value_(bool_ ? 1 : 0); return true;
            case VALUE_FLOAT: out = value_(bool_ ? 1.0f : .0f); return true;
            }
            break;
        default:
            assert(false);
        }
        return false;
    }

    void dbgLog() const {
        switch (type) {
        case VALUE_FLOAT:
            LOG_DBG("float: " << float_);
            break;
        case VALUE_FLOAT2:
            LOG_DBG("float2: [" << float2.x << ", " << float2.y << "]");
            break;
        case VALUE_FLOAT3:
            LOG_DBG("float3: [" << float3.x << ", " << float3.y << ", " << float3.z << "]");
            break;
        case VALUE_INT:
            LOG_DBG("int: " << int_);
            break;
        case VALUE_INT2:
            LOG_DBG("int2: [" << int2.x << ", " << int2.y << "]");
            break;
        case VALUE_INT3:
            LOG_DBG("int3: [" << int3.x << ", " << int3.y << ", " << int3.z << "]");
            break;
        case VALUE_BOOL:
            LOG_DBG("bool: " << (bool_ ? "true" : "false"));
            break;
        default:
            LOG_DBG("undefined: undefined");
        }
    }
};