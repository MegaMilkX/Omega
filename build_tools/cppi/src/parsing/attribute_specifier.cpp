#include "attribute_specifier.hpp"
#include "common.hpp"


bool eat_attribute_argument_clause(parse_state& ps, attribute* attr) {
    ps.push_rewind_point();
    token tok = ps.next_token();
    if (tok.str != "(") {
        ps.rewind_();
        return false;
    }

    ATTRIB_TYPE attrib_type = get_attribute_type(attr->qualified_name.c_str());
    if (attrib_type == ATTRIB_TOKENS) {
        ps.push_rewind_point();
        if (!eat_balanced_token_seq(ps)) {
            throw parse_exception("Expected a balanced token sequence", tok);
        }
        std::vector<token> tokens;
        ps.pop_rewind_get_tokens(tokens);
        attr->add_tokens(&tokens[0], tokens.size());
    } else if(attrib_type == ATTRIB_STRING_LITERAL) {
        token tok;
        if (!eat_token(ps, tt_string_literal, &tok)) {
            throw parse_exception("Expected a string literal", tok);
        }
        attr->value.set_string(tok.strlit_content);
    }

    tok = ps.next_token();
    if (tok.str != ")") {
        throw parse_exception("Expected a ')'", tok);
    }
    ps.pop_rewind_point();
    return true;
}
attribute* eat_attribute_token(parse_state& ps, attribute_specifier& attr_spec) {
    ps.push_rewind_point();
    token tok = ps.next_token();
    if (tok.type != tt_identifier && tok.type != tt_keyword) {
        ps.rewind_();
        return 0;
    }
    attribute* attr = attr_spec.getadd_attrib(tok.str);

    while (true) {
        ps.push_rewind_point();
        tok = ps.next_token();
        if (tok.str != "::") {
            ps.rewind_();
            ps.pop_rewind_point();
            return attr;
        }
        tok = ps.next_token();
        if (tok.type != tt_identifier && tok.type != tt_keyword) {
            throw parse_exception("Expected an identifier", tok);
        }
        attr = attr->getadd_attrib(tok.str);

        ps.pop_rewind_point();
    }
}
bool eat_attribute(parse_state& ps, attribute_specifier& attr_spec) {
    attribute* pattr = 0;
    if (!(pattr = eat_attribute_token(ps, attr_spec))) {
        return false;
    }
    eat_attribute_argument_clause(ps, pattr);
    return true;
}
bool eat_attribute_list(parse_state& ps, attribute_specifier& attr_spec) {
    if (!eat_attribute(ps, attr_spec)) {
        return true;
    }
    while (true) {
        ps.push_rewind_point();
        token tok = ps.next_token();
        if (tok.str == "...") {
            ps.pop_rewind_point();
            return true;
        }
        if(tok.str != ",") {
            ps.rewind_();
            return true;
        }
        if (eat_attribute(ps, attr_spec)) {
            ps.pop_rewind_point();
            continue;
        } else {
            ps.rewind_();
            return true;
        }
    }
}
bool eat_attribute_specifier(parse_state& ps) {
    attribute_specifier attr_spec;
    return eat_attribute_specifier(ps, attr_spec);
}
bool eat_attribute_specifier_seq(parse_state& ps) {
    attribute_specifier attr_spec;
    return eat_attribute_specifier_seq(ps, attr_spec);
}
bool eat_attribute_specifier(parse_state& ps, attribute_specifier& attr_spec) {
    ps.push_rewind_point();
    if (!accept(ps, "[")) {
        ps.rewind_();
        return false;
    }
    if (!accept(ps, "[")) {
        ps.rewind_();
        return false;
    }

    if (!eat_attribute_list(ps, attr_spec)) {
        throw parse_exception("Expected an attribute_list", ps.latest_token);
    }

    expect(ps, "]");
    expect(ps, "]");

    ps.pop_rewind_point();
    return true;
}
bool eat_attribute_specifier_seq(parse_state& ps, attribute_specifier& attr_spec) {
    if (!eat_attribute_specifier(ps, attr_spec)) {
        return false;
    }
    while (eat_attribute_specifier(ps, attr_spec)) {}

    return true;
}
