#pragma once

#include <assert.h>
#include <string>


struct cp_rewind_point {
    int at;
    int line;
    int col;
    int read_buf_len;
    char last_read_char;
};

constexpr int READ_FLAG_RAW_STRING = 0x0001;
class char_provider {
    int flags;
    int line;
    int col;
    int at;
    char last_read_char = '\0';

    char eat_char_p1() {
        if (at == text_len) {
            if (last_read_char != '\n') {
                return '\n';
            } else {
                return '\0';
            }
        }
        char ch = text[at];
        while (ch == '\r') { // Discard caret return completely, it's useless and annoying
            ++at;
            ch = text[at];
        }
        if (ch == '?' && !(flags & READ_FLAG_RAW_STRING)) {
            ++at;
            ch = text[at];
            if (at == text_len) {
                ++col;
                return '?';
            }
            if (ch != '?') {
                ++col;
                return '?';
            }
            ++at;
            ch = text[at];
            if (at == text_len) {
                --at;
                ++col;
                return '?';
            }
            switch (ch) {
            case '=': ++at; col += 3; return '#';
            case '/': ++at; col += 3; return '\\';
            case '\'': ++at; col += 3; return '^';
            case '(': ++at; col += 3; return '[';
            case ')': ++at; col += 3; return ']';
            case '!': ++at; col += 3; return '|';
            case '<': ++at; col += 3; return '{';
            case '>': ++at; col += 3; return '}';
            case '-': ++at; col += 3; return '~';
            default:
                --at;
                ++col;
                return '?';
            }
        } else {
            ++at;
            ++col;
            return ch;
        }
    }
    char eat_char_p2() {
        while(true) {
            char ch = eat_char_p1();
            int at_rw = at;
            if(ch == '\\' && !(flags & READ_FLAG_RAW_STRING)) {
                ch = eat_char_p1();
                if (ch != '\n') {
                    at = at_rw;
                    last_read_char = '\\';
                    return '\\';
                }
                ++line;
                col = 0;
                continue; // do the whole process again
            } else {
                last_read_char = ch;
                return ch;
            }
        }
    }

    void skip_utf8bom_if_present() {
        if (text_len < 3) {
            return;
        }
        char ch0 = text[0];
        char ch1 = text[1];
        char ch2 = text[2];
        if (ch0 == (char)0xEF && ch1 == (char)0xBB && ch2 == (char)0xBF) {
            at += 3;
            printf("Found UTF8 BOM\n");
        }
    }
public:
    const char* text;
    int text_len;
    std::string read_buf;

    char_provider()
        : text(0), text_len(0), at(0), line(0), col(0), flags(0) {}
    char_provider(const char* text, int text_len)
        : text(text), text_len(text_len), at(0), line(1), col(0), flags(0) {
        skip_utf8bom_if_present();
        eat_char_p2();
    }

    int  get_flags() const { return flags; }
    void set_flags(int flags) { this->flags = flags; }

    int get_line() const { return line; }
    int get_col() const { return col; }
    bool is_eof() const {
        return last_read_char == '\0';
    }

    char getch() const {
        return last_read_char;
    }
    void adv(unsigned int offs = 1) {
        for (int i = 0; i < offs; ++i) {
            read_buf.push_back(last_read_char);
            if (last_read_char == '\n') {
                ++line;
                col = 0;
            }
            eat_char_p2();
        }
    }
    void rewind_and_skip_with_own_chars(const cp_rewind_point& a, const cp_rewind_point& b, const std::string& chars) {
        assert(a.at <= at);
        assert(a.at < b.at);
        rewind(a);
        at = b.at;
        line = b.line;
        col = b.col;
        last_read_char = b.last_read_char;
        read_buf.insert(read_buf.end(), chars.begin(), chars.end());
    }
    bool getstr(std::string& out, int len) {
        //assert(len);
        auto rw = get_rewind_point();
        for (int i = 0; i < len; ++i) {
            if (getch() == '\0') {
                rewind(rw);
                return false;
            }
            out.push_back(getch());
            adv(1);
        }
        return true;
    }

    cp_rewind_point get_rewind_point() {
        return cp_rewind_point{ at, line, col, (int)read_buf.size(), last_read_char };
    }
    int char_read_since_rewind_point(const cp_rewind_point& pt) {
        return (int)read_buf.size() - pt.read_buf_len;
    }
    std::string get_read_string(const cp_rewind_point& pt) const {
        assert(pt.read_buf_len <= read_buf.size());
        return std::string(read_buf.begin() + pt.read_buf_len, read_buf.end());
    }
    void rewind(const cp_rewind_point& pt) {
        at = pt.at;
        line = pt.line;
        col = pt.col;
        last_read_char = pt.last_read_char;
        assert(pt.read_buf_len <= read_buf.size());
        read_buf.resize(pt.read_buf_len);
    }
};