#pragma once

#include "symbol_table.hpp"

class parse_state;
class symbol_capture_scope {
    parse_state& ps;
    std::vector<symbol_ref> symbols;
public:
    symbol_capture_scope(parse_state& ps);
    ~symbol_capture_scope();

    void add(symbol_ref sym) {
        symbols.push_back(sym);
    }

    void cancel_symbols() {
        for (int i = 0; i < symbols.size(); ++i) {
            auto sym = symbols[i];
            sym->set_declared(false);
        }
    }
};


#define SYMBOL_CAPTURE_SCOPE(NAME) \
    symbol_capture_scope _symbol_capture_scope_ ## NAME(ps)
#define SYMBOL_CAPTURE_CANCEL(NAME) \
    _symbol_capture_scope_ ## NAME.cancel_symbols()