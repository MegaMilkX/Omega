#pragma once

#include <stack>
#include <list>
#include <unordered_map>
#include "preprocessor.hpp"
#include "token.hpp"
#include "symbol_table.hpp"

enum decl_node_type {
    decl_node_type_global,
    decl_node_type_typedef,
    decl_node_type_class,
    decl_node_type_enum,
    decl_node_type_namespace,
    decl_node_type_function,
    decl_node_type_variable,
    decl_node_type_template // TODO: don't use scopes for templates
};
inline const char* scope_type_to_string(decl_node_type type) {
    switch (type) {
    case decl_node_type_global:
        return "global_scope";
    case decl_node_type_typedef:
        return "typedef";
    case decl_node_type_class:
        return "class";
    case decl_node_type_enum:
        return "enum";
    case decl_node_type_namespace:
        return "namespace";
    case decl_node_type_function:
        return "function";
    case decl_node_type_variable:
        return "variable";
    case decl_node_type_template:
        return "template";
    default:
        assert(false);
        return "unknown";
    }
}

struct name_lookup_key {
    std::list<std::string> name_list;
    bool from_global_scope = false;

    void dbg_print() const {
        if (name_list.empty()) {
            return;
        }
        if (from_global_scope) {
            printf("::");
        }
        auto it = name_list.begin();
        printf("%s", (*it).c_str());
        while (it != name_list.end()) {
            printf("::%s", (*it).c_str());
            ++it;
        }
    }
};

token convert_pp_token(PP_TOKEN& pp_tok);

class parse_state {
    preprocessor* pp = 0;
    std::vector<token> token_cache;
    int token_cache_cur = 0;
    std::stack<int> rewind_stack;

    std::shared_ptr<symbol_table> root_symbol_table;
    std::stack<std::shared_ptr<symbol_table>> symbol_table_stack;
public:
    //std::vector<std::shared_ptr<symbol>> linear_symbol_table;
    token latest_token;
    bool no_reflect = false;

    parse_state() {}
    parse_state(preprocessor* pp)
    : pp(pp) {
        root_symbol_table = enter_scope();
    }

    void init(preprocessor* pp) {
        token_cache.clear();
        token_cache_cur = 0;
        while (!rewind_stack.empty()) {
            rewind_stack.pop();
        }
        root_symbol_table.reset();
        while (!symbol_table_stack.empty()) {
            symbol_table_stack.pop();
        }
        //linear_symbol_table.clear();
        latest_token = token();

        this->pp = pp;
        root_symbol_table = enter_scope();
    }

    void add_symbol(const std::shared_ptr<symbol>& sym) {
        get_current_scope()->add_symbol(sym, no_reflect);/*
        if (!sym->global_qualified_name.empty()) {
            linear_symbol_table.push_back(sym);
        }*/
    }
    void enter_scope(std::shared_ptr<symbol_table> scope) {
        symbol_table_stack.push(scope);
    }
    std::shared_ptr<symbol_table> enter_scope(scope_type type = scope_default) {
        std::shared_ptr<symbol_table> sptr(new symbol_table);
        sptr->set_type(type);
        if (!symbol_table_stack.empty()) {
            sptr->set_enclosing(symbol_table_stack.top());
        }
        symbol_table_stack.push(sptr);
        return sptr;
    }
    void exit_scope() {
        assert(symbol_table_stack.size() > 1);
        symbol_table_stack.pop();
    }
    std::shared_ptr<symbol_table> get_current_scope() {
        return symbol_table_stack.top();
    }
    std::shared_ptr<symbol_table> get_root_scope() {
        return root_symbol_table;
    }

    void push_rewind_point() {
        rewind_stack.push(token_cache_cur);
    }
    void rewind_() {
        if (rewind_stack.empty()) {
            assert(false);
            return;
        }
        token_cache_cur = rewind_stack.top();
        rewind_stack.pop();
        for (int i = token_cache_cur; i < token_cache.size(); ++i) {
            if (token_cache[i].transient) {
                token_cache.erase(token_cache.begin() + i);
                --i;
            }
        }

        int i = token_cache_cur - 1;
        if (i < token_cache.size() && i > 0) {
            latest_token = token_cache[i];
        } else {
            latest_token = token();
        }
    }
    void pop_rewind_point() {
        assert(!rewind_stack.empty());
        rewind_stack.pop();
    }
    void pop_rewind_get_tokens(std::vector<token>& tokens) {
        assert(!rewind_stack.empty());
        for (int i = rewind_stack.top(); i < token_cache_cur; ++i) {
            tokens.push_back(token_cache[i]);
        }
        pop_rewind_point();
    }

    token next_token() {
        if (token_cache_cur == token_cache.size()) {
            PP_TOKEN pp_tok = pp->next_token();
            token tok = convert_pp_token(pp_tok);
            token_cache.push_back(tok);
            ++token_cache_cur;
            if (tok.type == tt_string_literal) {
                token tok_next;
                while (true) {
                    pp_tok = pp->next_token();
                    tok_next = convert_pp_token(pp_tok);
                    if (tok_next.type != tt_string_literal) {
                        token_cache.push_back(tok_next);
                        break;
                    }
                    token_cache.back().strlit_content += tok_next.strlit_content;
                    tok.strlit_content += tok_next.strlit_content;
                }
            }
            latest_token = tok;
            return tok;
        } else {
            token tok = token_cache[token_cache_cur];
            latest_token = tok;
            ++token_cache_cur;
            return tok;
        }
    }
    void push_token(const token& tok, bool transient = false) {
        token_cache.insert(token_cache.begin() + token_cache_cur, tok);
        token_cache[token_cache_cur].transient = transient;
    }
    void push_tokens(const std::vector<token>& tokens, bool transient) {
        token_cache.insert(token_cache.begin() + token_cache_cur, tokens.begin(), tokens.end());
        for (int i = token_cache_cur; i < token_cache_cur + tokens.size(); ++i) {
            token_cache[i].transient = transient;
        }
    }
    token& get_latest_token() {
        assert(!token_cache.empty() && token_cache_cur > 0);
        return token_cache[token_cache_cur - 1];
    }
};
