#include "pp_parse.hpp"
#include "pp_rule.hpp"
#include <unordered_map>
#include <cctype>
#include "pp_exception.hpp"
#include <algorithm>
#include "pp_util.hpp"

#define OPT(VARIANT) pp_rule(VARIANT, true)
#define CONDITIONAL(VARIANT, CONDITION_FN) pp_rule(VARIANT, CONDITION_FN)
#define REPORT(VARIANT, REPORT_ID) pp_rule(VARIANT, REPORT_ID)


static std::unordered_map<entity, std::vector<pp_rule>> rule_table = {
    /*{
        preprocessing_file, {
            OPT(group)
        }
    },*/
    {
        group, {
            group_part,
            { group, group_part }
        }
    },
    {
        group_part, {
            if_section,
            control_line,
            text_line,
            { '#', non_directive }
        }
    },
    {
        if_section, {
            { if_group, OPT(elif_groups), OPT(else_group), endif_line }
        }
    },
    {
        if_group, {
            // TODO: constant_expression
            //{ '#', "if", constant_expression, new_line, OPT(group) }
        }
    },
    {
        elif_groups, {
            elif_group,
            { elif_groups, elif_group }
        }
    },
    {
        elif_group, {
            // TODO: constant_expression
            //{ '#', "elif", constant_expression, new_line, OPT(group) }
        }
    },
    {
        else_group, {
            { '#', "else", new_line, OPT(group) }
        }
    },
    {
        endif_line, {
            { '#', "endif", new_line }
        }
    },
    {
        control_line, {
            { '#', "include", pp_tokens, new_line },
            { '#', "define", identifier, replacement_list, new_line },
            { '#', "define", identifier, lparen, OPT(identifier_list), ')', replacement_list, new_line },
            { '#', "define", identifier, lparen, "...", ')', replacement_list, new_line },
            { '#', "define", identifier, lparen, identifier_list, "...", ')', replacement_list, new_line },
            { '#', "undef", identifier, new_line },
            { '#', "line", pp_tokens, new_line },
            { '#', "error", OPT(pp_tokens), new_line },
            { '#', "pragma", OPT(pp_tokens), new_line },
            { '#', new_line }
        }
    },
    {
        text_line, {
            { OPT(pp_tokens), new_line }
        }
    },
    {
        non_directive, {
            { pp_tokens, new_line }
        }
    },
    {
        identifier_list, {
            identifier,
            { identifier_list, identifier }
        }
    },
    {
        replacement_list, {
            OPT(pp_tokens) // TODO: THIS WILL FAIL, SINCE AMOUNT PARSED IS ZERO, BUT SHOULD COUNT AS A SUCCESS
        }
    },
    {
        pp_tokens, {
            preprocessing_token,
            { pp_tokens, preprocessing_token }
        }
    },
    {
        preprocessing_token, {
            CONDITIONAL(header_name, [](void)->bool { return false; }),
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
        header_name, {
            { '<', h_char_sequence, '>' },
            { '"', q_char_sequence, '"' }
        }
    },
    {
        h_char_sequence, {
            h_char,
            { h_char_sequence, h_char }
        }
    },
    {
        q_char_sequence, {
            q_char,
            { q_char_sequence, q_char }
        }
    },
    {
        c_char_sequence, {
            c_char,
            { c_char_sequence, c_char }
        }
    },
    {
        c_char, {
            c_char_basic,
            escape_sequence,
            universal_character_name
        }
    },
    {
        escape_sequence, {
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
        unknown_escape_sequence, {
            { '\\', s_char_basic }
        }
    },
    {
        octal_escape_sequence, {
            { '\\', octal_digit },
            { '\\', octal_digit, octal_digit },
            { '\\', octal_digit, octal_digit, octal_digit }
        }
    },
    {
        hexadecimal_escape_sequence, {
            { "\\x", hexadecimal_digit },
            { hexadecimal_escape_sequence, hexadecimal_digit }
        }
    },
    {
        pp_number, {
            digit,
            { '.', digit },
            { pp_number, digit },
            { pp_number, 'e', sign },
            { pp_number, 'E', sign },
            { pp_number, identifier_nondigit },
            { pp_number, '\'', digit },
            { pp_number, '\'', nondigit },
            { pp_number, '.' }
        }
    },
    {
        character_literal, {
            { '\'', c_char_sequence, '\'' },
            { 'u', '\'', c_char_sequence, '\'' },
            { 'U', '\'', c_char_sequence, '\'' },
            { 'L', '\'', c_char_sequence, '\'' }
        }
    },
    {
        string_literal, {
            pp_rule({ OPT(encoding_prefix), '"', OPT(s_char_sequence), '"' }),
            pp_rule({ OPT(encoding_prefix), 'R', raw_string })
        }
    },
    {
        encoding_prefix,
        { "u8", 'u', 'U', 'L' }
    },
    {
        s_char_sequence, {
            s_char,
            { s_char_sequence, s_char }
        }
    },
    {
        s_char, {
            s_char_basic,
            escape_sequence,
            universal_character_name,
            unknown_escape_sequence
        }
    },
    {
        identifier, {
            identifier_nondigit,
            { identifier, identifier_nondigit },
            { identifier, digit }
        }
    },
    {
        identifier_nondigit, {
            nondigit,
            universal_character_name,
            // other implementation-defined characters
        }
    }
};

#include "char_provider.hpp"

int pp_accept(char_provider& rs, char ch) {
    if (rs.getch() == ch) {
        rs.adv(1);
        return 1;
    }
    return 0;
}
int pp_accept(char_provider& rs, const std::string& str) {
    auto rw = rs.get_rewind_point();
    for (int i = 0; i < str.size(); ++i) {
        if (rs.getch() != str[i]) {
            rs.rewind(rw);
            return 0;
        }
        rs.adv(1);
    }
    return rs.char_read_since_rewind_point(rw);
}

int pp_parse(char_provider& rs, const pp_rule& v);
int pp_parse(char_provider& rs, entity e);
int pp_parse(char_provider& rs, const std::string& str);
int pp_parse(char_provider& rs, char ch);
int pp_parse(char_provider& rs, const pp_rule* elems, int count);

static int count_comment(char_provider& rs) {
#define ADV(COUNT) \
    rs.adv(COUNT); \
    if(rs.is_eof()) { return 0; } \
    ch = rs.getch();

    auto rw = rs.get_rewind_point();
    char ch = rs.getch();
    if (ch != '/') {
        return 0;
    }
    ADV(1);
    if (ch == '/') {
        ADV(1);
        while (ch != '\n') {
            ADV(1);
        }
        return rs.char_read_since_rewind_point(rw);
    } else if(ch =='*') {
        ADV(1);
        while (true) {
            int adv = pp_parse(rs, std::string("*/"));
            if (adv) {
                return rs.char_read_since_rewind_point(rw);
            }
            ADV(1);
        }
    }
    rs.rewind(rw);
    return 0;
#undef ADV
}

static int count_whitespace(char_provider& rs) {
    auto rw = rs.get_rewind_point();
    while (!rs.is_eof()) {
        char ch = rs.getch();
        if (ch == '\n') {
            break;
        }
        if (!std::isspace(ch)) {
            int adv = count_comment(rs);
            if (adv) {
                continue;
            }
            break;
        }
        rs.adv(1);
    }
    return rs.char_read_since_rewind_point(rw);
}
static int is_newline(char_provider& rs) {
    if (rs.is_eof()) {
        return 0;
    }
    char ch = rs.getch();
    if (ch == '\n') {
        rs.adv(1);
        return 1;
    }
    return 0;
}

int pp_bin_char_to_int(char ch) {
    switch (ch) {
    case '0': return 0;
    case '1': return 1;
    default:
        assert(false);
        return 0;
    }
}
int pp_octal_char_to_int(char ch) {
    switch (ch) {
    case '0': return 0;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    default:
        assert(false);
        return 0;
    }
}
int pp_hex_char_to_int(char ch) {
    switch (ch) {
    case '0': return 0;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
    case 'A': return 10;
    case 'B': return 11;
    case 'C': return 12;
    case 'D': return 13;
    case 'E': return 14;
    case 'F': return 15;
    case 'a': return 10;
    case 'b': return 11;
    case 'c': return 12;
    case 'd': return 13;
    case 'e': return 14;
    case 'f': return 15;
    default:
        assert(false);
        return 0;
    }
}
int pp_dec_char_to_int(char ch) {
    switch (ch) {
    case '0': return 0;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
    default:
        assert(false);
        return 0;
    }
}
int pp_parse_binary_literal(PP_TOKEN* out) {
    if (out->str.empty()) {
        return 0;
    }
    int begin = 0;
    int end = 0;
    if (out->str[0] != '0') {
        return false;
    }
    if (out->str[1] != 'b' && out->str[1] != 'B') {
        return false;
    }
    begin = 2;
    end = 2;

    if (!is_binary_digit(out->str[end])) {
        throw pp_exception("Binary number requires at least one digit", out->line, out->col);
    }
    ++end;

    while (end < out->str.size()) {
        char ch = out->str[end];
        if (is_binary_digit(ch) || ch == '\'') {
            ++end;
        } else if(is_digit(ch)) {
            throw pp_exception("Invalid binary digit", out->line, out->col);
        } else {
            break;
        }
    }

    intmax_t value = 0;
    int base = 1;
    for (int i = end - 1; i >= begin; --i) {
        char ch = out->str[i];
        if (ch == '\'') {
            continue;
        }
        int n = pp_bin_char_to_int(ch);
        n *= base;
        value += n;
        base *= 2;
    }
    out->integral = value;
    out->number_type = PP_NUMBER_INTEGRAL;
    return true;
}
int pp_parse_octal_literal(PP_TOKEN* out) {
    if (out->str.empty()) {
        return 0;
    }
    int begin = 0;
    int end = 0;
    if (out->str[0] != '0') {
        return false;
    }
    begin = 1;
    end = 1;

    while (end < out->str.size()) {
        char ch = out->str[end];
        if (is_octal_digit(ch) || ch == '\'') {
            ++end;
        } else if(is_digit(ch)) {
            throw pp_exception("Invalid octal digit", out->line, out->col);
        } else {
            break;
        }
    }

    intmax_t value = 0;
    int base = 1;
    for (int i = end - 1; i >= begin; --i) {
        char ch = out->str[i];
        if (ch == '\'') {
            continue;
        }
        int n = pp_octal_char_to_int(ch);
        n *= base;
        value += n;
        base *= 8;
    }
    out->integral = value;
    out->number_type = PP_NUMBER_INTEGRAL;
    return true;
}
bool pp_parse_hexadecimal_literal(PP_TOKEN* out) {
    if (out->str.empty()) {
        return 0;
    }
    int begin = 0;
    int end = 0;
    if (out->str[0] != '0') {
        return false;
    }
    if (out->str[1] != 'x' && out->str[1] != 'X') {
        return false;
    }
    begin = 2;
    end = 2;

    if (!is_hexadecimal_digit(out->str[end])) {
        throw pp_exception("Hexadecimal number requires at least one digit", out->line, out->col);
    }
    ++end;

    while (true) {
        char ch = out->str[end];
        if (is_hexadecimal_digit(ch) || ch == '\'') {
            ++end;
        } else {
            break;
        }
    }

    intmax_t value = 0;
    int base = 1;
    for (int i = end - 1; i >= begin; --i) {
        char ch = out->str[i];
        if (ch == '\'') {
            continue;
        }
        int n = pp_hex_char_to_int(ch);
        n *= base;
        value += n;
        base *= 16;
    }
    out->integral = value;
    out->number_type = PP_NUMBER_INTEGRAL;
    return true;
}
bool pp_parse_decimal_literal(PP_TOKEN* out) {
    if (out->str.empty()) {
        return 0;
    }
    int begin = 0;
    int end = 0;
    if (!is_nonzero_digit(out->str[0])) {
        return 0;
    }
    ++end;

    while (end < out->str.size()) {
        char ch = out->str[end];
        if (is_digit(ch) || ch == '\'') {
            ++end;
        } else {
            break;
        }
    }

    intmax_t value = 0;
    int base = 1;
    for (int i = end - 1; i >= begin; --i) {
        char ch = out->str[i];
        if (ch == '\'') {
            continue;
        }
        int n = pp_dec_char_to_int(ch);
        n *= base;
        value += n;
        base *= 10;
    }
    out->integral = value;
    out->number_type = PP_NUMBER_INTEGRAL;

    return true;
}
uintmax_t pp_digit_sequence_to_uint(const char* begin, const char* end) {
    uintmax_t value = 0;
    int base = 1;
    for (const char* cur = end - 1; cur != begin - 1; --cur) {
        char ch = *cur;
        if (ch == '\'') {
            continue;
        }
        int n = pp_dec_char_to_int(ch);
        n *= base;
        value += n;
        base *= 10;
    }
    return value;
}
const char* pp_parse_digit_sequence(const char* begin, const char* end) {
    if (begin == end) {
        return begin;
    }
    const char* cur = begin;
    while (cur != end) {
        if (!is_digit(*cur) && *cur != '\'') {
            break;
        }
        ++cur;
    }
    if (cur == begin) {
        return begin;
    }

    return cur;
}
const char* pp_parse_fractional_constant(const char* begin, const char* end) {
    const char* cur = begin;
    const char* left_end = pp_parse_digit_sequence(cur, end);
    cur = left_end;
    if (cur == end || *cur != '.') {
        return begin;
    }
    const char* dot_pos = cur;
    cur += 1;

    const char* right_end = pp_parse_digit_sequence(cur, end);
    cur = right_end;

    if (left_end == begin && right_end == dot_pos + 1) {
        return begin;
    }

    return cur;
}
const char* pp_parse_exponent_part(const char* begin, const char* end) {
    const char* cur = begin;
    if (cur == end) {
        return begin;
    }

    if (*cur != 'e' && *cur != 'E') {
        return begin;
    }
    ++cur;

    if (*cur == '-' || *cur == '+') {
        ++cur;
    }

    const char* dig_seq = pp_parse_digit_sequence(cur, end);
    if (cur == dig_seq) {
        throw pp_exception("Expected a digit sequence in the exponent part", 0, 0);
    }
    cur = dig_seq;

    return cur;
}
bool pp_parse_floating_literal(PP_TOKEN* out) {
    const char* begin = &out->str[0];
    const char* end = &out->str[0] + out->str.size();
    const char* cur = begin;

    if (begin != (cur = pp_parse_fractional_constant(begin, end))) {
        const char* exp_cur = pp_parse_exponent_part(cur, end);
        if (exp_cur != cur) {
            cur = exp_cur;
        }
        out->floating = std::stod(std::string(begin, cur));
        out->number_type = PP_NUMBER_FLOATING;
        // TODO: SUFFIX
        return true;
    } else if(begin != (cur = pp_parse_digit_sequence(begin, end))) {
        const char* exp_cur = pp_parse_exponent_part(cur, end);
        if (exp_cur == cur) {
            return false;
        }
        cur = exp_cur;
        out->floating = std::stod(std::string(begin, cur));
        out->number_type = PP_NUMBER_FLOATING;
        // TODO: SUFFIX
        return true;
    }
    
    return false;
}
int pp_parse_numeric_literal(char_provider& rs, PP_TOKEN* out) {
    int line = rs.get_line();
    int col = rs.get_col();

    int adv = 0;
    auto rw = rs.get_rewind_point();
    if (!(adv = pp_parse(rs, pp_number))) {
        return 0;
    }
    out->str = rs.get_read_string(rw);
    out->line = line;
    out->col = col;
    out->type = PP_NUMBER;
    
    if (pp_parse_floating_literal(out)) {
        return adv;
    }
    if (pp_parse_hexadecimal_literal(out)) {
        return adv;
    }
    if (pp_parse_binary_literal(out)) {
        return adv;
    }
    if (pp_parse_octal_literal(out)) {
        return adv;
    }
    if (pp_parse_decimal_literal(out)) {
        return adv;
    }

    return adv;
}

int pp_parse_token(char_provider& rs, PP_TOKEN* out, bool include_header_name_token) {
    int adv = 0;
    rs.read_buf.clear(); // TODO: Not very good to change read_buf directly
    auto rw = rs.get_rewind_point();
    int line = rs.get_line();
    int col = rs.get_col();

    if (adv = pp_parse(rs, whitespace)) {
        out->type = WHITESPACE;
    } else if(adv = pp_parse(rs, new_line)) {
        out->type = NEW_LINE;
    } else if(include_header_name_token && (adv = pp_parse(rs, header_name))) {
        out->type = HEADER_NAME;
    } else if(adv = pp_parse(rs, string_literal)) {
        out->type = STRING_LITERAL;
    }/* else if(adv = pp_parse(at, text, text_len, user_defined_string_literal)) {
        out->type = USER_DEFINED_STRING_LITERAL;
    }*/ else if(adv = pp_parse(rs, character_literal)) {
        out->type = CHARACTER_LITERAL;
    }/* else if(adv = pp_parse(at, text, text_len, user_defined_character_literal)) {
        out->type = USER_DEFINED_CHARACTER_LITERAL;
    }*/ else if(adv = pp_parse(rs, identifier)) {
        out->type = IDENTIFIER;
    } else if(adv = pp_parse_numeric_literal(rs, out)) {
        return adv;
    } else if(adv = pp_parse(rs, preprocessing_op_or_punc)) {
        out->type = PP_OP_OR_PUNC;
    } else if(adv = pp_parse(rs, e_any)) {
        out->type = PP_ANY;
    }

    out->line = line;
    out->col = col;

    if (!adv) {
        out->type = END_OF_FILE;
        return 0;
    }
    int char_count = rs.char_read_since_rewind_point(rw);
    out->str = rs.get_read_string(rw);
    return adv;
}

int parse_into_pp_tokens(const char* text, int text_len, std::vector<PP_TOKEN>& out, const PP_FILE* file) {
    char_provider rs(text, text_len);
    PP_TOKEN tok;
    if (file) {
        // TODO: token's file reference should be const
        tok.file = (PP_FILE*)file;
    }
    int adv = pp_parse_token(rs, &tok);
    while (tok.type != END_OF_FILE && tok.type != NEW_LINE) {
        out.push_back(tok);
        adv = pp_parse_token(rs, &tok);
    }
    return out.size();
}
int parse_into_pp_tokens_and_header_name_skip_first_whitespace(const char* text, int text_len, std::vector<PP_TOKEN>& out, const PP_FILE* file) {
    char_provider rs(text, text_len);
    PP_TOKEN tok;
    if (file) {
        // TODO: token's file reference should be const
        tok.file = (PP_FILE*)file;
    }
    int adv = pp_parse_token(rs, &tok, true);
    while (tok.type == WHITESPACE) {
        adv = pp_parse_token(rs, &tok, true);
    }

    while (tok.type != END_OF_FILE) {
        out.push_back(tok);
        adv = pp_parse_token(rs, &tok, true);
    }
    return out.size();
}

static int parse_d_char_sequence(char_provider& rs, std::string& out) {
    if (rs.is_eof()) {
        return 0;
    }
    auto rw = rs.get_rewind_point();
    while (!rs.is_eof()) {
        char ch = rs.getch();
        switch (ch) {
        case '(':
            return rs.char_read_since_rewind_point(rw);
        case ' ':
        case ')':
        case '\\':
        case '\t':
        case '\v':
        case '\f':
        case '\n':
            throw pp_exception("Invalid character in raw string delimeter", rs);
        }
        out.push_back(ch);
        rs.adv(1);
    }
    return rs.char_read_since_rewind_point(rw);
}

static int parse_raw_string(char_provider& rs) {
#define ADV(COUNT) \
    rs.adv(COUNT); \
    if(rs.is_eof()) { return 0; } \
    ch = rs.getch();

    if (rs.is_eof()) {
        return 0;
    }
    auto rw = rs.get_rewind_point();
    char ch = rs.getch();
    if (ch != '"') {
        return 0;
    }

    rs.set_flags(rs.get_flags() | READ_FLAG_RAW_STRING);
    auto rw0 = rs.get_rewind_point();
    ADV(1);

    std::string delim;
    int delim_len = parse_d_char_sequence(rs, delim);
    
    ch = rs.getch();
    if (ch != '(') {
        rs.set_flags(rs.get_flags() & ~READ_FLAG_RAW_STRING);
        throw pp_exception("Missing an opening parenthesis in raw string literal", rs);
    }
    
    ADV(1);
    while (!rs.is_eof()) {
        if (ch == ')') {
            ADV(1);
            auto rw = rs.get_rewind_point();
            std::string delim_end;
            if (!rs.getstr(delim_end, delim_len)) {
                continue;
            }
            if (0 != delim.compare(delim_end)) {
                rs.rewind(rw);
                continue;
            }

            if (rs.getch() == '"') {
                ADV(1);
                rs.set_flags(rs.get_flags() & ~READ_FLAG_RAW_STRING);
                return rs.char_read_since_rewind_point(rw0);
            }
        }
        ADV(1);
    }
    throw pp_exception("Raw string missing closing parenthesis and delimeter", rs);
    rs.set_flags(rs.get_flags() & ~READ_FLAG_RAW_STRING);
    return rs.char_read_since_rewind_point(rw);
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
inline int pp_op_or_punc_table_init() {
    memcpy(punctuators_sorted.data(), punctuators_, sizeof(punctuators_));
    std::sort(punctuators_sorted.begin(), punctuators_sorted.end(), [](const char* a, const char* b)->bool {
        return strlen(a) > strlen(b);
    });
    for (int i = 0; i < punctuators_sorted.size(); ++i) {
        if (strcmp("<:", punctuators_sorted[i]) == 0) {
            alt_token_less_colon_idx = i;
        }
    }
    return 0;
}
static int parse_preprocessing_op_or_punc(char_provider& rs) {
    {
        static int once = pp_op_or_punc_table_init();
    }
    auto rw = rs.get_rewind_point();
    for (int i = 0; i < punctuators_sorted.size(); ++i) {
        const char* punc = punctuators_sorted[i];
        int punc_len = strlen(punc);

        std::string s;
        if (!rs.getstr(s, punc_len)) {
            continue;
        }
        if (strncmp(punc, s.c_str(), punc_len) != 0) {
            rs.rewind(rw);
            continue;
        }
        char_provider rs_best = rs;

        // Checking for <:: case
        if (i == alt_token_less_colon_idx) {
            rs.adv(2);
            if (rs.is_eof()) {
                return punc_len;
            }
            char ch = rs.getch();
            if (ch == ':') {
                rs.adv(1);
                if (rs.is_eof()) {
                    rs.rewind(rw);
                    rs.adv();
                    return 1; // <
                }
                ch = rs.getch();
                if (ch != ':' && ch != '>') {
                    rs.rewind(rw);
                    rs.adv(1);
                    return 1; // <
                }
            }
            rs = rs_best;
        }
        return punc_len;
    }
    rs.rewind(rw);
    return 0;
}


int pp_parse(char_provider& rs, const pp_rule& v) {
    if (rs.is_eof()) {
        return 0;
    }
    if (v.is(VARIANT_TYPE::ENTITY)) {
        return pp_parse(rs, v.e);
    } else if(v.is(VARIANT_TYPE::STRING)) {
        return pp_parse(rs, v.str);
    } else if(v.is(VARIANT_TYPE::CHAR)) {
        return pp_parse(rs, v.ch);
    }
    return 0;
}


int pp_parse(char_provider& rs, entity e) {
    if (rs.is_eof()) {
        return 0;
    }
    switch (e) {
    case h_char:
        if (is_h_char(rs.getch())) {
            rs.adv(1);
            return 1;
        } else {
            return 0;
        }
    case q_char:
        if (is_q_char(rs.getch())) {
            rs.adv(1);
            return 1;
        } else {
            return 0;
        }
    case c_char_basic:
        if (is_c_char_basic(rs.getch())) {
            rs.adv(1);
            return 1;
        } else {
            return 0;
        }
    case s_char_basic: 
        if (is_s_char_basic(rs.getch())) {
            rs.adv(1);
            return 1;
        } else {
            return 0;
        }
    case raw_string: return parse_raw_string(rs);
    case digit:
        if (is_digit(rs.getch())) {
            rs.adv(1);
            return 1;
        } else {
            return 0;
        }
    case octal_digit: 
        if (is_octal_digit(rs.getch())) {
            rs.adv(1);
            return 1;
        } else {
            return 0;
        }
    case hexadecimal_digit:
        if (is_hexadecimal_digit(rs.getch())) {
            rs.adv(1);
            return 1;
        } else {
            return 0;
        }
    case nondigit:
        if (is_nondigit(rs.getch())) {
            rs.adv(1);
            return 1;
        } else {
            return 0;
        }
    case sign: 
        if (is_sign(rs.getch())) {
            rs.adv(1);
            return 1;
        } else {
            return 0;
        }
    case preprocessing_op_or_punc: return parse_preprocessing_op_or_punc(rs);
    case e_any:
        if (is_e_any(rs.getch())) {
            rs.adv(1);
            return 1;
        } else {
            return 0;
        }
    case whitespace: { return count_whitespace(rs); }
    case new_line: return is_newline(rs);
    // TODO BEGIN
    case universal_character_name: return 0;
    // TODO END
    default: {
        auto it = rule_table.find(e);
        if (it == rule_table.end()) {
            assert(false);
            return 0;
        }
        auto& list = it->second;
        // single or-list
        std::vector<pp_rule*> non_recursive_options;
        std::vector<pp_rule*> recursive_options;
        for (int i = 0; i < list.size(); ++i) {
            auto& v = list[i];
            if (v.fn_condition && !v.fn_condition()) {
                continue;
            }
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
        auto rw = rs.get_rewind_point();
        char_provider rs_best;
        for (int i = 0; i < non_recursive_options.size(); ++i) {
            rs.rewind(rw);
            auto pv = non_recursive_options[i];
            int adv = 0;
            if (pv->is(VARIANT_TYPE::LIST)) {
                adv = pp_parse(rs, &pv->list[0], pv->list.size());
            } else {
                adv = pp_parse(rs, *pv);
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
                    rs_best = rs;
                }
            }
        }
        non_recursive_adv = max_adv;
        if (non_recursive_adv == 0) {
            rs.rewind(rw);
            return 0;
        } else {
            rs = rs_best;
        }

        int total_adv = non_recursive_adv;
        while (true) {
            int adv = 0;
            for (int i = 0; i < recursive_options.size(); ++i) {
                auto pv = recursive_options[i];
                adv = pp_parse(rs, &pv->list[1], pv->list.size() - 1);
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

int pp_parse(char_provider& rs, const std::string& str) {
    auto rw = rs.get_rewind_point();
    std::string copy;
    if (!rs.getstr(copy, str.length())) {
        return 0;
    }
    for (int i = 0; i < str.length(); ++i) {
        if (str[i] != copy[i]) {
            rs.rewind(rw);
            return 0;
        }
    }
    return rs.char_read_since_rewind_point(rw);
}
int pp_parse(char_provider& rs, char ch) {
    if (rs.is_eof()) {
        return 0;
    }
    if (ch == rs.getch()) {
        rs.adv(1);
        return 1;
    } else {
        return 0;
    }
}
int pp_parse(char_provider& rs, const pp_rule* elems, int count) {
    if (rs.is_eof()) {
        return 0;
    }
    // single and-list
    auto rw = rs.get_rewind_point();
    for (int i = 0; i < count; ++i) {
        int adv = pp_parse(rs, elems[i]);
        if (!adv && !elems[i].optional) {
            rs.rewind(rw);
            return 0;
        }
    }
    return rs.char_read_since_rewind_point(rw);
}