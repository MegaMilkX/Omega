#pragma once

#include <vector>
#include <functional>
#include "pp_types.hpp"

enum class REPORT {
    NONE,
    STRING_LITERAL
};
enum class VARIANT_TYPE {
    UNK,
    ENTITY,
    STRING,
    CHAR,
    LIST
};
const char* VARIANT_TYPE_STR[] = {
    "UNK",
    "ENTITY",
    "STRING",
    "CHAR",
    "LIST"
};

constexpr int RULE_FLAG_NONE                = 0x0;
constexpr int RULE_FLAG_IGNORE_PHASES_1_2   = 0x01;
struct pp_rule {
    VARIANT_TYPE type = VARIANT_TYPE::UNK;
    std::string str;
    entity e = nothing;
    char ch = 0;
    std::vector<pp_rule> list;
    std::function<bool(void)> fn_condition = nullptr;
    bool optional = false;
    int flags = RULE_FLAG_NONE;
    pp_rule(entity e, const std::function<bool(void)>& fn_condition) : type(VARIANT_TYPE::ENTITY), e(e), fn_condition(fn_condition) {}
    pp_rule(entity e, bool optional = false) : type(VARIANT_TYPE::ENTITY), e(e), optional(optional) {}
    pp_rule(entity e, int flags) : type(VARIANT_TYPE::ENTITY), e(e), flags(flags) {}
    pp_rule(const char* str, bool optional = false) : type(VARIANT_TYPE::STRING), str(str), optional(optional) {}
    pp_rule(char ch, bool optional = false) : type(VARIANT_TYPE::CHAR), ch(ch), optional(optional) {}
    pp_rule(const std::initializer_list<pp_rule>& list) : type(VARIANT_TYPE::LIST), list(list), optional(false) {}
    bool is(VARIANT_TYPE t) const { return type == t; }
};
