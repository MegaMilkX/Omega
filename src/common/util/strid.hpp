#pragma once

#include <string>
#include "log/log.hpp"

uint32_t getStringId(const char* str);
std::string getStringIdString(uint32_t id);

class string_id {
    uint32_t id;
public:
    string_id() : id(getStringId("")){}
    explicit string_id(const char* str)
    : id(getStringId(str)) {}
    explicit string_id(const std::string& str)
    : id(getStringId(str.c_str())) {}
    string_id(const string_id& other)
    : id(other.id) {}

    uint32_t to_uint32() const {
        return id;
    }
    std::string to_string() const {
        return getStringIdString(id);
    }

    string_id& operator=(const char* str) {
        id = getStringId(str);
    }
    string_id& operator=(const std::string& str) {
        id = getStringId(str.c_str());
    }

    bool operator==(const string_id& other) const {
        return id == other.id;
    }
    bool operator<(const string_id& other) const {
        return id < other.id;
    }
    operator std::string() const {
        return getStringIdString(id);
    }
    operator uint32_t() const {
        return id;
    }
};
template <>
struct std::hash<string_id> {
    std::size_t operator()(const string_id& k) const {
        return std::hash<uint32_t>()(k.to_uint32());
    }
};