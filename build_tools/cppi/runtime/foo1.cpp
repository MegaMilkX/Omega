
#include <assert.h>
#include <stdio.h>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <cctype>
#include <stdio.h>
#define NOMINMAX
#include <Windows.h>

#define OPT(VARIANT) variant_(VARIANT, true)

static HANDLE hConsole = INVALID_HANDLE_VALUE;

static int strtoint3(const char* str) {
    assert(strnlen_s(str, 3) == 3);
    int i = 0;
    i |= (int)str[0];
    i |= (int)str[1] << 8;
    i |= (int)str[2] << 16;
    return i;
}

static bool is_space(char ch) {
    return std::isspace(ch);
}

static bool is_dchar(char ch) {
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

static bool is_h_char(char ch) {
    switch (ch) {
    case '\n':
    case '>':
        return false;
    }
    return true;
}
static bool is_q_char(char ch) {
    switch (ch) {
    case '\n':
    case '"':
        return false;
    }
    return true;
}
static bool is_c_char_basic(char ch) {
    switch (ch) {
    case '\'':
    case '\\':
    case '\n':
        return false;
    }
    return true;
}
static bool is_s_char_basic(char ch) {
    switch (ch) {
    case '"':
    case '\\':
    case '\n':
        return false;
    }
    return true;
}
static bool is_nondigit(char c) {
    return (c >= 97 && c <= 122) || (c >= 65 && c <= 90) || c == 95;
}
static bool is_digit(char c) {
    return c >= 48 && c <= 57;
}
static bool is_octal_digit(char c) {
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
static bool is_hexadecimal_digit(char c) {
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
static bool is_sign(char ch) {
    switch (ch) {
    case '+':
    case '-':
        return true;
    }
    return false;
}
static bool is_e_any(char ch) {
    return !std::isspace(ch);
}

static void phase_one(const char* in, size_t len, std::vector<char>& out) {
    std::unordered_map<uint32_t, char> trigraphs = {
        { strtoint3("??="), '#' },
        { strtoint3("??/"), '\\' },
        { strtoint3("??'"), '^' },
        { strtoint3("??("), '[' },
        { strtoint3("??)"), ']' },
        { strtoint3("??!"), '|' },
        { strtoint3("??<"), '{' },
        { strtoint3("??>"), '}' },
        { strtoint3("??-"), '~' }
    };

    int cur = 0;
    char ch = 0;
    auto next = [&](int count = 1) {
        cur += count;
        if (cur >= len) {
            ch = 0;
        } else {
            ch = in[cur];
        }
    };
    auto rewind = [&](int count) {
        cur -= count;
        ch = in[cur];
    };
    auto seek_to = [&](int at) {
        cur = at;
        ch = in[at];
    };
    auto eat_d_char_sequence = [&]()->int {
        int i = 0;
        while (is_dchar(ch)) {
            next();
            ++i;
        }
        rewind(i);
        return i;
    };
    auto eat_newline = [&]()->int {
        if (ch == '\n') {
            return 1;
        }
        if (ch == '\r') {
            next();
            if (ch == '\n') {
                rewind(2);
                return 2;
            }
            rewind(1);
        }
        return 0;
    };
    auto eat_trigraph = [&]()->int {
        if (ch == '?' && len - cur >= 3) {
            char ch2 = in[cur + 1];
            char ch3 = in[cur + 2];
            uint32_t i = 0;
            i |= ch;
            i |= ch2 << 8;
            i |= ch3 << 16;
            auto it = trigraphs.find(i);
            if (it != trigraphs.end()) {
                out.push_back(it->second);
                return 3;
            }
        }
        return 0;
    };
    auto eat_backslash_newline = [&]()->int {
        if (ch == '\\') {
            next();
            int adv = eat_newline();
            if (adv) {
                rewind(1);
                return 1 + adv;
            }
            rewind(1);
        }
        return 0;
    };
    auto eat_raw_string_literal_end = [&](const std::string& delim)->int {
        int base_cur = cur;
        if (ch != ')') {
            return 0;
        }
        next();
        int delim_len = eat_d_char_sequence();
        std::string delim_end(&in[cur], &in[cur + delim_len]);
        if (delim != delim_end) {
            seek_to(base_cur);
            return 0;
        }
        next(delim_len);
        if (ch != '"') {
            seek_to(base_cur);
            return 0;
        }
        int count = cur - base_cur;
        seek_to(base_cur);
        return count;
    };
    auto eat_raw_string_literal = [&]()->int {
        int base_cur = cur;
        if (ch != 'R') {
            return 0;
        }
        next();
        if (ch != '"') {
            seek_to(base_cur);
            return 0;
        }
        next();
        int delim_len = eat_d_char_sequence();
        std::string delim(&in[cur], &in[cur + delim_len]);
        next(delim_len);
        if (ch != '(') {
            seek_to(base_cur);
            return 0;
        }
        while (ch != '\0') {
            int adv = eat_raw_string_literal_end(delim);
            if (adv) {
                next(adv);
                break;
            }
            next();
        }
        int count = cur - base_cur;
        seek_to(base_cur);
        return count;
    };
    while (cur < len) {
        ch = in[cur];
        
        int adv = eat_trigraph();
        if (adv) {
            cur += adv;
            continue;
        }
        adv = eat_backslash_newline();
        if (adv) {
            cur += adv;
            continue;
        }
        adv = eat_raw_string_literal();
        if (adv) {
            out.insert(out.end(), &in[cur], &in[cur + adv]);
            cur += adv;
            continue;
        }
        out.push_back(ch);

        ++cur;
    }
}


enum entity {
    preprocessing_token,

    header_name,
    identifier,
    pp_number,
    character_literal,
    user_defined_character_literal,
    string_literal,
    user_defined_string_literal,
    preprocessing_op_or_punc,

    h_char_sequence,
    q_char_sequence,
    c_char_sequence,

    identifier_nondigit,
    universal_character_name,

    // simplest
    h_char,
    q_char,
    c_char,
    c_char_basic,
    escape_sequence,
    simple_escape_sequence,
    octal_escape_sequence,
    hexadecimal_escape_sequence,
    digit,
    octal_digit,
    hexadecimal_digit,
    nondigit,
    sign,
    encoding_prefix,
    s_char_sequence,
    raw_string,
    s_char,
    s_char_basic,

    // other
    e_any,
    whitespace,

    // special parsing tools
    nothing
};
enum class VARIANT_TYPE {
    UNK,
    ENTITY,
    STRING,
    CHAR,
    LIST
};
const char* VARIANT_TYPE_STR[] = {
    "UNK",
    "ENTITY",
    "STRING",
    "CHAR",
    "LIST"
};
struct variant_ {
    VARIANT_TYPE type = VARIANT_TYPE::UNK;
    std::string str;
    entity e = nothing;
    char ch = 0;
    std::vector<variant_> list;
    bool optional = false;
    variant_(entity e, bool optional = false) : type(VARIANT_TYPE::ENTITY), e(e), optional(optional) {}
    variant_(const char* str, bool optional = false) : type(VARIANT_TYPE::STRING), str(str), optional(optional) {}
    variant_(char ch, bool optional = false) : type(VARIANT_TYPE::CHAR), ch(ch), optional(optional) {}
    variant_(const std::initializer_list<variant_>& list) : type(VARIANT_TYPE::LIST), list(list), optional(false) {}
    bool is(VARIANT_TYPE t) const { return type == t; }
};

static std::unordered_map<entity, std::vector<variant_>> ent_table = {
    {
        preprocessing_token,
        {
            header_name,
            identifier,
            pp_number,
            character_literal,
            //user_defined_character_literal,
            string_literal,
            //user_defined_string_literal,
            preprocessing_op_or_punc,
            e_any
        }
    },
    {
        header_name,
        {
            { '<', h_char_sequence, '>' },
            { '"', q_char_sequence, '"' }
        }
    },
    {
        h_char_sequence,
        {
            h_char,
            { h_char_sequence, h_char }
        }
    },
    {
        q_char_sequence,
        {
            q_char,
            { q_char_sequence, q_char }
        }
    },
    {
        c_char_sequence,
        {
            c_char,
            { c_char_sequence, c_char }
        }
    },
    {
        c_char,
        {
            c_char_basic,
            escape_sequence,
            universal_character_name
        }
    },
    {
        escape_sequence,
        {
            simple_escape_sequence,
            octal_escape_sequence,
            hexadecimal_escape_sequence
        }
    },
    {
        simple_escape_sequence,
        { R"(\')", R"(\")", R"(\?)", R"(\\)", R"(\a)", R"(\b)", R"(\f)", R"(\n)", R"(\r)", R"(\t)", R"(\v)" }
    },
    {
        octal_escape_sequence,
        {
            { '\\', octal_digit },
            { '\\', octal_digit, octal_digit },
            { '\\', octal_digit, octal_digit, octal_digit }
        }
    },
    {
        hexadecimal_escape_sequence,
        {
            { "\\x", hexadecimal_digit },
            { hexadecimal_escape_sequence, hexadecimal_digit }
        }
    },
    {
        pp_number,
        {
            digit,
            { '.', digit },
            { pp_number, digit },
            { pp_number, identifier_nondigit },
            { pp_number, '\'', digit },
            { pp_number, '\'', nondigit },
            { pp_number, 'e', sign },
            { pp_number, 'E', sign },
            { pp_number, '.' }
        }
    },
    {
        character_literal,
        {
            { '\'', c_char_sequence, '\'' },
            { 'u', '\'', c_char_sequence, '\'' },
            { 'U', '\'', c_char_sequence, '\'' },
            { 'L', '\'', c_char_sequence, '\'' }
        }
    },
    {
        string_literal,
        {
            { OPT(encoding_prefix), '"', s_char_sequence, '"' },
            { OPT(encoding_prefix), 'R', raw_string }
        }
    },
    {
        encoding_prefix,
        { "u8", 'u', 'U', 'L' }
    },
    {
        s_char_sequence,
        {
            s_char,
            { s_char_sequence, s_char }
        }
    },
    {
        s_char,
        {
            s_char_basic,
            escape_sequence,
            universal_character_name
        }
    },
    {
        identifier,
        {
            identifier_nondigit,
            { identifier, identifier_nondigit },
            { identifier, digit }
        }
    },
    {
        identifier_nondigit,
        {
            nondigit,
            universal_character_name,
            // other implementation-defined characters
        }
    }
};

static const char* text = 0;
static int text_len = 0;
static int cur = 0;

int proc(int at, const variant_& v);
int proc(int at, entity e);
int proc(int at, const std::string& str);
int proc(int at, char ch);
int proc(int at, const variant_* elems, int count);

static int count_comment(int at) {
#define ADV(COUNT) \
    at += COUNT; \
    if(at >= text_len) { return 0; } \
    ch = text[at];

    int base = at;
    char ch = text[at];
    if (ch != '/') {
        return 0;
    }
    ADV(1);
    if (ch == '/') {
        ADV(1);
        while (ch != '\n') {
            ADV(1);
        }
        return at - base;
    } else if(ch =='*') {
        ADV(1);
        while (true) {
            int adv = proc(at, std::string("*/"));
            if (adv) {
                return at - base + adv;
            }
            ADV(1);
        }
    }
    return 0;
#undef ADV
}
static int count_whitespace(int at) {
    int base = at;
    while (at < text_len) {
        char ch = text[at];
        if (!std::isspace(ch)) {
            int adv = count_comment(at);
            if (adv) {
                at += adv;
                continue;
            }
            break;
        }
        ++at;
    }
    return at - base;
}

static int proc_d_char_sequence(int at) {
    if (at >= text_len) {
        return 0;
    }
    int begin = at;
    while (at < text_len) {
        char ch = text[at];
        switch (ch) {
        case ' ':
        case '(':
        case ')':
        case '\\':
        case '\t':
        case '\v':
        case '\f':
        case '\n':
            return at - begin;
        }
        ++at;
    }
    return at - begin;
}
static int proc_raw_string(int at) {
#define ADV(COUNT) \
    at += COUNT; \
    if(at >= text_len) { return 0; } \
    ch = text[at];

    if (at >= text_len) {
        return 0;
    }
    int begin = at;
    char ch = text[at];
    if (ch != '"') {
        return 0;
    }
    ADV(1);

    int delim_len = proc_d_char_sequence(at);
    std::string delim(text + at, text + at + delim_len);
    
    ADV(delim_len);
    if (ch != '(') {
        return 0;
    }
    
    ADV(1);
    while (at < text_len) {
        if (ch == ')') {
            ADV(1);
            if (0 != delim.compare(std::string(text + at, text + at + std::min(delim_len, text_len - at)))) {
                continue;
            }
            ADV(delim_len);
            if (ch == '"') {
                ADV(1);
                break;
            }
        }
        ADV(1);
    }
    return at - begin;
#undef ADV
}

static const char* const punctuators_[] = {
    "{", "}", "[", "]", "#", "##", "(", ")",
    "<:", ":>", "<%", "%>", "%:", "%:%:", ";", ":", "...",
    "new", "delete", "?", "::", ".", ".*",
    "+", "-", "*", "/", "%", "^", "&", "|", "~",
    "!", "=", "<", ">", "+=", "-=", "*=", "/=", "%=",
    "^=", "&=", "|=", "<<", ">>", ">>=", "<<=", "==", "!=",
    "<=", ">=", "&&", "||", "++", "--", ",", "->*", "->",
    "and", "and_eq", "bitand", "bitor", "compl", "not", "not_eq",
    "or", "or_eq", "xor", "xor_eq"
};
static const size_t punctuator_count = sizeof(punctuators_) / sizeof(punctuators_[0]);
static std::vector<const char*> punctuators_sorted(punctuator_count);
static int alt_token_less_colon_idx = 0;
static int proc_preprocessing_op_or_punc(int at) {
    for (int i = 0; i < punctuators_sorted.size(); ++i) {
        const char* punc = punctuators_sorted[i];
        int punc_len = strlen(punc);
        int len_available = text_len - at;
        if (len_available < punc_len) {
            continue;
        }
        if (strncmp(punc, text + at, punc_len) != 0) {
            continue;
        }
        // Checking for <:: case
        if (i == alt_token_less_colon_idx) {
            at += 2;
            if (at >= text_len) {
                return punc_len;
            }
            char ch = text[at];
            if (ch == ':') {
                ++at;
                if (at >= text_len) {
                    return 1; // <
                }
                ch = text[at];
                if (ch != ':' && ch != '>') {
                    return 1; // <
                }
            }
        }
        return punc_len;
    }
    return 0;
}


int proc(int at, const variant_& v) {
    if (at >= text_len) {
        return 0;
    }
    if (v.is(VARIANT_TYPE::ENTITY)) {
        return proc(at, v.e);
    } else if(v.is(VARIANT_TYPE::STRING)) {
        return proc(at, v.str);
    } else if(v.is(VARIANT_TYPE::CHAR)) {
        return proc(at, v.ch);
    }
    return 0;
}
int proc(int at, entity e) {
    if (at >= text_len) {
        return 0;
    }
    switch (e) {
    case h_char: return is_h_char(text[at]) ? 1 : 0;
    case q_char: return is_q_char(text[at]) ? 1 : 0;
    case c_char_basic: return is_c_char_basic(text[at]) ? 1 : 0;
    case s_char_basic: return is_s_char_basic(text[at]) ? 1 : 0;
    case raw_string: return proc_raw_string(at);
    case digit: return is_digit(text[at]) ? 1 : 0;
    case octal_digit: return is_octal_digit(text[at]) ? 1 : 0;
    case hexadecimal_digit: return is_hexadecimal_digit(text[at]) ? 1 : 0;
    case nondigit: return is_nondigit(text[at]) ? 1 : 0;
    case sign: return is_sign(text[at]) ? 1 : 0;
    case preprocessing_op_or_punc: return proc_preprocessing_op_or_punc(at);
    case e_any: return is_e_any(text[at]) ? 1 : 0;
    case whitespace: return count_whitespace(at);
    // TODO BEGIN
    case universal_character_name: return 0;
    // TODO END
    default: {
        auto it = ent_table.find(e);
        if (it == ent_table.end()) {
            assert(false);
            return 0;
        }
        auto& list = it->second;
        // single or-list
        std::vector<variant_*> non_recursive_options;
        std::vector<variant_*> recursive_options;
        for (int i = 0; i < list.size(); ++i) {
            auto& v = list[i];
            if (v.is(VARIANT_TYPE::LIST) && v.list[0].is(VARIANT_TYPE::ENTITY) && v.list[0].e == e) {
                recursive_options.push_back(&v);
            } else {
                non_recursive_options.push_back(&v);
            }
        }
        int non_recursive_adv = 0;
        int best_fit_id = -1;
        int max_list_len = 0;
        int max_adv = 0;
        for (int i = 0; i < non_recursive_options.size(); ++i) {
            auto pv = non_recursive_options[i];
            int adv = 0;
            if (pv->is(VARIANT_TYPE::LIST)) {
                adv = proc(at, &pv->list[0], pv->list.size());
            } else {
                adv = proc(at, *pv);
            }
            if (adv) {
                int list_len = 1;
                if (pv->is(VARIANT_TYPE::LIST)) {
                    list_len = pv->list.size();
                }
                if (max_list_len < list_len || (max_list_len == list_len && max_adv < adv)) {
                    best_fit_id = i;
                    max_list_len = list_len;
                    max_adv = adv;
                }
            }
        }
        non_recursive_adv = max_adv;
        if (non_recursive_adv == 0) {
            return 0;
        }
        int total_adv = non_recursive_adv;
        while (true) {
            int adv = 0;
            for (int i = 0; i < recursive_options.size(); ++i) {
                auto pv = recursive_options[i];
                adv = proc(at + total_adv, &pv->list[1], pv->list.size() - 1);
                total_adv += adv;
                if (adv) {
                    break;
                }
            }
            if (adv == 0) {
                // None of the recursive patterns fit anymore
                break;
            }
        }
        return total_adv;
    }
    }
    return 0;
}
int proc(int at, const std::string& str) {
    if (at + str.length() > text_len) {
        return 0;
    }
    std::string copy(text + at, text + at + str.length());
    for (int i = 0; i < str.length(); ++i) {
        if (str[i] != text[at + i]) {
            return 0;
        }
    }
    return str.length();
}
int proc(int at, char ch) {
    if (at >= text_len) {
        return 0;
    }
    return (ch == text[at]) ? 1 : 0;
}
int proc(int at, const variant_* elems, int count) {
    if (at >= text_len) {
        return 0;
    }
    // single and-list
    int total_adv = 0;
    for (int i = 0; i < count; ++i) {
        int adv = proc(at + total_adv, elems[i]);
        total_adv += adv;
        if (!adv && !elems[i].optional) {
            return 0;
        }
    }
    return total_adv;
}

static void phase_two(const char* in, size_t len, std::vector<char>& out) {
    text = in;
    text_len = (int)len;
    cur = 0;
    
    while (cur < text_len) {
        int adv = 0;
        adv = proc(cur, whitespace);
        if (adv) {
            SetConsoleTextAttribute(hConsole, 24);
            printf("%s", std::string(&text[cur], &text[cur + adv]).c_str());
            out.insert(out.end(), text + cur, text + cur + adv);
            cur += adv;
            continue;
        }
        adv = proc(cur, preprocessing_token);
        if (adv) {
            SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
            printf("%s", std::string(&text[cur], &text[cur + adv]).c_str());
            //SetConsoleTextAttribute(hConsole, 6);
            //printf("\n");
            out.insert(out.end(), text + cur, text + cur + adv);
            cur += adv;
            continue;
        }/*
        adv = proc(cur, identifier);
        if (adv) {
            SetConsoleTextAttribute(hConsole, 22);
            printf("%s\n", std::string(&text[cur], &text[cur + adv]).c_str());
            cur += adv;
            continue;
        }
        adv = proc(cur, e_any);
        if (adv) {
            SetConsoleTextAttribute(hConsole, 16);
            printf("%s\n", std::string(&text[cur], &text[cur + adv]).c_str());
            cur += adv;
            continue;
        }*/
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_RED);
        printf("phase_two() failed at:\n%s", std::string(text + cur, text + text_len).c_str());
        break;
    }
}

int foo1() {
    memcpy(punctuators_sorted.data(), punctuators_, sizeof(punctuators_));
    std::sort(punctuators_sorted.begin(), punctuators_sorted.end(), [](const char* a, const char* b)->bool {
        return strlen(a) > strlen(b);
    });
    for (int i = 0; i < punctuators_sorted.size(); ++i) {
        if (strcmp("<:", punctuators_sorted[i]) == 0) {
            alt_token_less_colon_idx = i;
        }
    }

    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    std::string fname = "test3.hpp";
    FILE* f = fopen(fname.c_str(), "rb");
    if (!f) {
        return -1;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (fsize <= 0) {
        return -1;
    }
    std::vector<char> buf;
    buf.resize(fsize);
    fread(buf.data(), fsize, 1, f);
    fclose(f);

    {
        std::vector<char> out;
        phase_one(buf.data(), buf.size(), out);
        FILE* f = fopen((fname + ".p1").c_str(), "wb");
        if (!f) {
            return -1;
        }
        fwrite(out.data(), out.size(), 1, f);
        fclose(f);
        buf = out;
    }
    {
        std::vector<char> out;
        phase_two(buf.data(), buf.size(), out);
        FILE* f = fopen((fname + ".p2").c_str(), "wb");
        if (!f) {
            return -1;
        }
        fwrite(out.data(), out.size(), 1, f);
        fclose(f);
    }
    SetConsoleTextAttribute(hConsole, 8);
    return 0;
}