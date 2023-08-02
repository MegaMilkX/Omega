#pragma once

#include "parse_state.hpp"


enum e_class_key {
    ck_class,
    ck_struct,
    ck_union
};
struct class_specifier {
    e_class_key key;
    std::string name;
};

bool eat_access_specifier(parse_state& ps);
bool eat_class_key(parse_state& ps, e_class_key& out_key);
std::shared_ptr<symbol> eat_class_specifier(parse_state& ps);
std::shared_ptr<symbol> eat_class_specifier_limited(parse_state& ps);

bool eat_brace_or_equal_initializer(parse_state& ps);