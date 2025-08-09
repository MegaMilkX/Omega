#pragma once

#include <vector>
#include "lex.hpp"

#include "../valve_data.hpp"


namespace valve {

struct parse_state {
    lex_state ls;
    std::vector<token> token_cache; // tokens that have been lexed already
    uint32_t tok_at = 0;
    bool m_has_error = false;

    parse_state(const char* data, uint64_t size)
        : ls(data, size)
    {}

    void consume() {
        token_cache.erase(token_cache.begin(), token_cache.begin() + tok_at);
        tok_at = 0;
    }
    void rewind() {
        tok_at = 0;
    }
    token peek() {
        if (tok_at < token_cache.size()) {
            return token_cache[tok_at];
        }
        assert(tok_at == token_cache.size());
        tok_at = token_cache.size();
        token_cache.push_back(next_token());
        return token_cache.back();
    }
    void advance() {
        if (tok_at >= token_cache.size()) {
            assert(tok_at < token_cache.size());
            return;
        }
        ++tok_at;
    }
    token next_token() {
        while(1) {
            lex_token(ls);
            if (ls.tok.type == tok_whitespace
                || ls.tok.type == tok_comment
            ) {
                continue;
            }
            break;
        }
        return ls.tok;
    }

    void set_error(const char* desc) {
        LOG_ERR("parsing error: " << desc);
        m_has_error = true;
    }
    bool has_error() const {
        if (m_has_error) {
            return true;
        }
        return ls.has_error();
    }
};

std::string tok_to_string(parse_state& ps, token t);
std::string tok_to_string_value(parse_state& ps, token t);


bool parse_accept(parse_state& ps, tok_type type);
bool parse_accept_punct(parse_state& ps, const char* punct);

valve_data parse_object(parse_state& ps);

valve_data parse_value2(parse_state& ps);

valve_data parse_value(parse_state& ps);
bool parse_key_value(parse_state& ps, valve_data& out);
bool parse_key_value_list(parse_state& ps, valve_data& object);

bool parse_material(valve_data& out, const char* data, size_t size);
bool parse_entity_list(valve_data& out, const char* data, size_t size);

}

