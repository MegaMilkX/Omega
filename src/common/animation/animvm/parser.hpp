#pragma once

#include <assert.h>
#include <vector>
#include <stack>
#include "lexer.hpp"
#include "ast.hpp"


namespace animvm {

    
    struct parse_state {
        lex_state ls;
        std::vector<token> token_cache;
        int token_cache_cur = 0;
        std::stack<int> rewind_stack;

        parse_state(const char* src)
            : ls(src) {}

        bool is_eof() {
            return tok().type == e_eof;
        }

        const token& tok() {
            if (token_cache_cur >= token_cache.size()) {
                token tt;
                while ((tt = lex_token(ls)).type == e_whitespace) {}
                token_cache.push_back(tt);
                token& t = token_cache[token_cache_cur];
                
                if (t.type == e_literal_integral) {
                    t._int = std::stoi(t.str);
                } else if (t.type == e_literal_floating) {
                    t._float = std::stod(t.str);
                } else if(t.type == e_operator) {
                    if (t.str == "(") {
                        t.type = e_lparen;
                    } else if(t.str == ")") {
                        t.type = e_rparen;
                    } else if(t.str == "[") {
                        t.type = e_lsparen;
                    } else if(t.str == "]") {
                        t.type = e_rsparen;
                    } else if(t.str == "{") {
                        t.type = e_lcparen;
                    } else if(t.str == "}") {
                        t.type = e_rcparen;
                    }
                } else if(t.type == e_identifier) {
                    const char* str_keywords[] = {
                        "float", "int", "bool",
                        "return"
                    };
                    keyword keywords[] = {
                        kw_float, kw_int, kw_bool,
                        kw_return
                    };
                    for (int i = 0; i < sizeof(keywords) / sizeof(keywords[0]); ++i) {
                        if (t.str == str_keywords[i]) {
                            t.kw = keywords[i];
                            t.type = e_keyword;
                            break;
                        }
                    }
                }
            }
            return token_cache[token_cache_cur];
        }
        void adv(int count) {
            while (count) {
                if (tok().type == e_eof) {
                    return;
                }
                ++token_cache_cur;
                --count;
            }
        }

        void push_rewind() {
            rewind_stack.push(token_cache_cur);
        }
        void pop_rewind() {
            rewind_stack.pop();
            if (rewind_stack.empty() && token_cache_cur > 0) {
                token_cache.erase(token_cache.begin(), token_cache.begin() + token_cache_cur);
                token_cache_cur = 0;
            }
        }
        int count_read() const {
            if (rewind_stack.empty()) {
                return 0;
            }
            return token_cache_cur - rewind_stack.top();
        }
        std::vector<token> pop_rewind_get_tokens_read() {
            std::vector<token> tokens;
            for (int i = 0; i < token_cache_cur; ++i) {
                tokens.push_back(token_cache[i]);
            }
            pop_rewind();
            return tokens;
        }
        void rewind() {
            token_cache_cur = rewind_stack.top();
            pop_rewind();
        }

        bool accept(e_token type) {
            if (tok().type != type) {
                return false;                
            }
            adv(1);
            return true;
        }
        bool accept(keyword kw) {
            if (tok().kw != kw) {
                return false;
            }
            adv(1);
            return true;
        }
        bool accept(const char* str) {
            if (tok().str != str) {
                return false;
            }
            adv(1);
            return true;
        }
        void expect_throw(const char* str) {
            if (tok().str != str) {
                throw parse_exception("expect_throw() condition not met");
            }
            adv(1);
            return;
        }
    };


    bool parse_expression(parse_state& ps, ast_node& node);
    bool parse_statement(parse_state& ps, ast_node& node);


}
