#pragma once

#include <assert.h>
#include <cctype>
#include <string.h>


inline int strtoint3(const char* str) {
    assert(strnlen_s(str, 3) == 3);
    int i = 0;
    i |= (int)str[0];
    i |= (int)str[1] << 8;
    i |= (int)str[2] << 16;
    return i;
}

inline bool is_space(char ch) {
    return std::isspace(ch);
}

inline bool is_dchar(char ch) {
    switch (ch) {
    case ' ':
    case '(':
    case ')':
    case '\\':
    case '\t':
    case '\v':
    case '\f':
    case '\n':
        return false;
    }
    return true;
}

inline bool is_h_char(char ch) {
    switch (ch) {
    case '\n':
    case '>':
        return false;
    }
    return true;
}
inline bool is_q_char(char ch) {
    switch (ch) {
    case '\n':
    case '"':
        return false;
    }
    return true;
}
inline bool is_c_char_basic(char ch) {
    switch (ch) {
    case '\'':
    case '\\':
    case '\n':
        return false;
    }
    return true;
}
inline bool is_s_char_basic(char ch) {
    switch (ch) {
    case '"':
    case '\\':
    case '\n':
        return false;
    }
    return true;
}
inline bool is_nondigit(char c) {
    return (c >= 97 && c <= 122) || (c >= 65 && c <= 90) || c == 95;
}
inline bool is_digit(char c) {
    return c >= 48 && c <= 57;
}
inline bool is_nonzero_digit(char c) {
    return c >= 49 && c <= 57;
}
inline bool is_binary_digit(char c) {
    switch (c) {
    case '0':
    case '1':
        return true;
    }
    return false;
}
inline bool is_octal_digit(char c) {
    switch (c) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
        return true;
    }
    return false;
}
inline bool is_hexadecimal_digit(char c) {
    switch (c) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
        return true;
    }
    return false;
}
inline bool is_sign(char ch) {
    switch (ch) {
    case '+':
    case '-':
        return true;
    }
    return false;
}
inline bool is_e_any(char ch) {
    return !std::isspace(ch);
}

inline std::string make_string_literal(const char* src) {
    std::string str;
    int len = strlen(src);
    str.push_back('"');
    for (int i = 0; i < len; ++i) {
        if (src[i] == '"' || src[i] == '\\') {
            str.push_back('\\');
        }
        str.push_back(src[i]);
    }
    str.push_back('"');
    return str;
}
