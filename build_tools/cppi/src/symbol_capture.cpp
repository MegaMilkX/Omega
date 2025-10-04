#include "symbol_capture.hpp"

#include "parse_state.hpp"



symbol_capture_scope::symbol_capture_scope(parse_state& ps)
    : ps(ps) {
    ps.push_symbol_capture(this);
}
symbol_capture_scope::~symbol_capture_scope() {
    ps.pop_symbol_capture();
}