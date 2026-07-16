#pragma once

#include <stdint.h>
#include "common.hpp"


class varying;
struct type;
struct MetaObject;

struct property {
    union {
        struct {
            type_uid_t object_type_uid;
            uint32_t prop_idx;
        };
        uint64_t id = 0;
    };

    property() {}
    property(type_uid_t type_uid, uint32_t prop_idx)
        : object_type_uid(type_uid), prop_idx(prop_idx) {}

    const std::string& get_name() const;
    type get_type() const;

    void set(MetaObject* object, const varying& var);
    varying get(MetaObject* object);

    bool operator==(const property& other) const { return id == other.id; }
    bool operator!=(const property& other) const { return id != other.id; }
    bool operator<(const property& other) const { return id < other.id; }
    operator bool() const { return id != 0; }
};
template<>
struct std::hash<property> {
    size_t operator()(const property& p) const {
        return std::hash<uint64_t>()(p.id);
    }
};

