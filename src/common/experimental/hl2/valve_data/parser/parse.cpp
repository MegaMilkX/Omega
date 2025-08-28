#include "parse.hpp"


namespace valve {

std::string tok_to_string(parse_state& ps, token t) {
    return std::string((const char*)&ps.ls.data[t.begin], (const char*)&ps.ls.data[t.end]);
}
std::string tok_to_string_value(parse_state& ps, token t) {
    if (t.type == tok_string && ps.ls.data[t.begin] == '"' && ps.ls.data[t.end - 1] == '"') {
        t.begin += 1;
        t.end -= 1;
    }
    return std::string((const char*)&ps.ls.data[t.begin], (const char*)&ps.ls.data[t.end]);
}


bool parse_accept(parse_state& ps, tok_type type) {
    if (ps.peek().type == type) {
        ps.advance();
        return true;
    }
    return false;
}
bool parse_accept_punct(parse_state& ps, const char* punct) {
    const int MAX_PUNCT_LEN = 16; // That's a lot
    const int punct_len = strnlen(punct, MAX_PUNCT_LEN);
    token t = ps.peek();
    if (ps.peek().type == tok_punct) {
        if(0 == strncmp((const char*)&ps.ls.data[t.begin], punct, punct_len)) {
            ps.advance();
            return true;
        }
    }
    return false;
}


valve_data parse_object(parse_state& ps) {
    if (!parse_accept_punct(ps, "{")) {
        return nullptr;
    }

    valve_data obj = valve_object();
    while(parse_key_value_list(ps, obj)) {}

    if (!parse_accept_punct(ps, "}")) {
        ps.set_error("Expected a '}'");
        return nullptr;
    }
    return obj;
}

valve_data parse_value2(parse_state& ps) {
    token t = ps.peek();
    if (t.type != tok_string) {
        return nullptr;
    }

    valve_data vd(tok_to_string_value(ps, t));

    ps.advance();
    return vd;
}

valve_data parse_value(parse_state& ps) {
    valve_data vd = parse_object(ps);
    if(vd) return vd;
    vd = parse_value2(ps);
    if(vd) return vd;
    return nullptr;
}
bool parse_key_value(parse_state& ps, valve_data& out) {
    ps.consume();

    token key = ps.peek();
    if (key.type != tok_string) {
        ps.rewind();
        return false;
    }
    ps.advance();

    valve_data val = parse_value(ps);
    if (!val) {
        ps.rewind();
        return false;
    }

    std::string skey = tok_to_string_value(ps, key);
    for (int i = 0; i < skey.size(); ++i) {
        skey[i] = std::tolower(skey[i]);
    }
    out[skey] = val;

    ps.consume();
    return true;
}
bool parse_key_value_list(parse_state& ps, valve_data& object) {
    if (!parse_key_value(ps, object)) {
        return false;
    }
    while(parse_key_value(ps, object)) {}

    ps.consume();
    return true;
}
bool parse_object_array(parse_state& ps, valve_data& array_) {
    valve_data elem;
    if (!(elem = parse_object(ps))) {
        return false;
    }
    array_.push_back(elem);
    while (elem = parse_object(ps)) {
        array_.push_back(elem);
    }
    ps.consume();
    return true;
}

bool parse_material(valve_data& out, const char* data, size_t size) {
    valve::parse_state ps((const char*)data, size);
    return valve::parse_key_value_list(ps, out);
}

bool parse_entity_list(valve_data& out, const char* data, size_t size) {
    valve::parse_state ps((const char*)data, size);
    return valve::parse_object_array(ps, out);
}
bool parse_phy(valve_data& out, const char* data, size_t size) {
    valve::parse_state ps((const char*)data, size);
    return valve::parse_key_value_list(ps, out);
}

}

