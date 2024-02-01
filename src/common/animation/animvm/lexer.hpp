#pragma once

#include <cstdarg>
#include <string>


namespace animvm {


    enum e_token {
        e_invalid,
        e_unknown,
        e_eof,
        e_whitespace,
        e_identifier,
        e_keyword,
        e_literal,
        e_literal_floating,
        e_literal_integral,
        e_operator,
        e_lparen,
        e_rparen,
        e_lsparen,
        e_rsparen,
        e_lcparen,
        e_rcparen
    };

    enum keyword {
        kw_not_a_keyword,
        kw_float,
        kw_int,
        kw_bool,
        kw_return
    };

    struct token {
        e_token type;
        keyword kw;
        std::string str;
        union {
            float   _float;
            int     _int;
        };

        operator bool() const {
            return type != e_invalid;
        }
    };

    class parse_exception : public std::exception {
        std::string msg;
    public:
        parse_exception(const token& tok, const char* format, ...) {
            char buf[256];
            char buf2[256];
            va_list arg_ptr;
            va_start(arg_ptr, format);
            vsnprintf(buf2, 256, format, arg_ptr);
            va_end(arg_ptr);
            snprintf(buf, 256, "line %i, col %i, '%s': %s", 0, 0, tok.str.c_str(), buf2);
            this->msg = buf;
        }
        char const* what() const override {
            return msg.c_str();
        }
    };

    struct lex_state {
        const char* cur;
        const char* token_start = 0;
        token tok;

        lex_state(const char* src)
            : cur(src) {}

        void adv(int count) {
            while (count) {
                if (cur[0] == 0) {
                    return;
                }
                ++cur;
                --count;
            }
        }
        char ch() const { return cur[0]; }

        void begin_token(e_token type) {
            tok.type = type;
            tok.str.clear();
            token_start = cur;
        }
        void change_token_type(e_token type) {
            tok.type = type;
        }
        bool end_token() {
            if (token_start == cur) {
                tok.type = e_invalid;
                return false;
            }
            tok.str = std::string(token_start, cur);
            return true;
        }
        void fail_token() {
            cur = token_start;
            tok.type = e_invalid;
        }
        int tok_len() const { return int(cur - token_start); }

        bool accept(char ch_) {
            if (ch() != ch_) {
                return false;
            }
            adv(1);
            return true;
        }
        bool accept_alpha() {
            if (!isalpha(ch())) {
                return false;
            }
            adv(1);
            return true;
        }
        bool accept_alnum() {
            if (!isalnum(ch())) {
                return false;
            }
            adv(1);
            return true;
        }
        bool accept_num() {
            const char* nums = "0123456789";
            const size_t count = strlen(nums);
            for (int i = 0; i < count; ++i) {
                if (ch() == nums[i]) {
                    adv(1);
                    return true;
                }
            }
            return false;
        }
        bool accept_underscore() {
            if (ch() != '_') {
                return false;
            }
            adv(1);
            return true;
        }
        bool accept_whitespace() {
            if (!iswspace(ch())) {
                return false;
            }
            adv(1);
            return true;
        }
    };


    token lex_token(lex_state& ls);


}