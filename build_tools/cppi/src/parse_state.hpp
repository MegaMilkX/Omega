#pragma once

#include <stack>
#include <list>
#include <unordered_map>
#include "preprocessor/preprocessor.hpp"
#include "token.hpp"
#include "symbol_table.hpp"
#include "symbol_capture.hpp"


token convert_pp_token(PP_TOKEN& pp_tok);

enum e_parse_context {
    e_ctx_normal,
    e_ctx_template_argument_list
};

class parse_state {
    preprocessor* pp = 0;
    std::vector<token> token_cache;
    int token_cache_cur = 0;
    std::stack<int> rewind_stack;

    std::shared_ptr<symbol_table> root_symbol_table;
    std::stack<std::shared_ptr<symbol_table>> symbol_table_stack;

    std::stack<e_parse_context> context_stack;

    std::stack<symbol_capture_scope*> symbol_capture_stack;

    // If set to true - the parser will only read constructs marked with [[cppi_***]] tags
    bool is_limited_mode = false;
    // Perform name lookup and symbol registration
    bool is_semantic_mode = true;

    void capture_symbol(symbol_ref sym) {
        if (symbol_capture_stack.empty()) {
            return;
        }
        auto sym_cap = symbol_capture_stack.top();
        sym_cap->add(sym);
    }
public:
    token latest_token;
    bool no_reflect = false;

    parse_state() {}
    parse_state(preprocessor* pp, bool limited = false)
    : pp(pp), is_limited_mode(limited) {
        root_symbol_table = enter_scope();
    }

    void push_symbol_capture(symbol_capture_scope* cap) {
        symbol_capture_stack.push(cap);
    }
    void pop_symbol_capture() {
        assert(!symbol_capture_stack.empty());
        symbol_capture_stack.pop();
    }

    bool is_limited() const {
        return is_limited_mode;
    }
    void set_limited(bool state_) {
        is_limited_mode = state_;
    }

    bool is_semantic() const {
        return is_semantic_mode;
    }
    void set_semantic(bool b) {
        is_semantic_mode = b;
    }

    void push_context(e_parse_context ctx) {
        context_stack.push(ctx);
    }
    void pop_context() {
        context_stack.pop();
    }
    e_parse_context get_current_context() const {
        if (context_stack.empty()) {
            return e_ctx_normal;
        }
        return context_stack.top();
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
    
    template<typename T>
    symbol_ref create_symbol(symbol_table_ref scope, const char* name) {
        if (!scope) {
            scope = get_current_scope();
        }

        auto file = get_latest_token().file;
        auto sym = scope->create_symbol<T>(name, file, this->no_reflect);

        capture_symbol(sym);

        return sym;
    }

    void add_symbol(const std::shared_ptr<symbol>& sym) {
        get_current_scope()->add_symbol(sym, no_reflect);
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

    int get_token_cache_cur() const { return token_cache_cur; }
    int get_rewind_stack_len() const { return rewind_stack.size(); }
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

    token peek_token() {
        return next_token(true);
    }
    token next_token(bool peek = false) {
        if (token_cache_cur == token_cache.size()) {
            PP_TOKEN pp_tok = pp->next_token();
            token tok = convert_pp_token(pp_tok);
            token_cache.push_back(tok);
            if(!peek) ++token_cache_cur;
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
            if(!peek) ++token_cache_cur;
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

class context_scope {
    parse_state& ps;
public:
    context_scope(parse_state& ps, e_parse_context ctx)
        : ps(ps) {
        ps.push_context(ctx);
    }
    ~context_scope() {
        ps.pop_context();
    }
};
#define CONTEXT_SCOPE(CTX) \
    context_scope _context_scope_helper_ ## NAME(ps, CTX)

#define DBG_REWIND_ENABLE 0
class rewind_scope {
    std::string dbg_str;
    parse_state& ps;
    bool rewind = false;
    int cursor = 0;
    int rewind_stack_len = 0;
public:
    rewind_scope(parse_state& ps, const std::string& dbg = std::string())
    : ps(ps), dbg_str(dbg), cursor(ps.get_token_cache_cur()) {
#if DBG_REWIND_ENABLE
        printf("rw psh (%s): %i\n", dbg_str.c_str(), cursor);
#endif
        ps.push_rewind_point();
        rewind_stack_len = ps.get_rewind_stack_len();
    }
    ~rewind_scope() {
        if(!rewind) {
            if (rewind_stack_len != ps.get_rewind_stack_len()) {
                printf("RWPOP STACK LEN (%s): was %i, has %i\n", dbg_str.c_str(),  rewind_stack_len, ps.get_rewind_stack_len());
                assert(false);
            }
            ps.pop_rewind_point();
#if DBG_REWIND_ENABLE
            printf("rw pop (%s): %i\n", dbg_str.c_str(),  ps.get_token_cache_cur());
#endif
        } else {
            if (rewind_stack_len != ps.get_rewind_stack_len()) {
                printf("RWREW STACK LEN (%s): was %i, has %i\n", dbg_str.c_str(),  rewind_stack_len, ps.get_rewind_stack_len());
                assert(false);
            }
            ps.rewind_();
            if (cursor != ps.get_token_cache_cur()) {
                printf("RWREW ERROR (%s): was: %i, has: %i\n", dbg_str.c_str(), cursor, ps.get_token_cache_cur());
                assert(false);
            }
#if DBG_REWIND_ENABLE
            printf("rw rew (%s): %i\n", dbg_str.c_str(),  ps.get_token_cache_cur());
#endif
        }
    }
    void set_rewind() {
        rewind = true;
    }
};

#define REWIND_SCOPE(NAME) \
    rewind_scope _rewind_scope_helper_ ## NAME(ps, #NAME)
#define REWIND_ON_EXIT(NAME) \
    _rewind_scope_helper_ ## NAME.set_rewind()

