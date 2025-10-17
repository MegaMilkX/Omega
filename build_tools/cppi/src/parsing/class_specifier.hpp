#pragma once

#include <memory>
#include "parse_state.hpp"
#include "types.hpp"
#include "ast/ast.hpp"


struct class_specifier {
    e_class_key key;
    std::string name;
};
class symbol;
struct base_specifier {
    std::shared_ptr<symbol> symbol; // The base type, if resolved, or a template
    std::vector<token> token_cache; // To reparse unpacked variadics
};

bool eat_access_specifier(parse_state& ps);
bool eat_class_key(parse_state& ps, e_class_key& out_key);
std::shared_ptr<symbol> eat_class_specifier(parse_state& ps);
std::shared_ptr<symbol> eat_class_specifier_limited(parse_state& ps);
std::shared_ptr<symbol> eat_class_specifier_limited(parse_state& ps, attribute_specifier& attr_spec);

bool eat_brace_or_equal_initializer(parse_state& ps);