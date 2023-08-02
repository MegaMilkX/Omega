#include "pp_constexpr.hpp"
#include "pp_exception.hpp"
#include "pp_macro_expansion.hpp"


bool pp_eat_defined_operator(pp_state& pps, intmax_t* ret) {
    pps.set_pp_rewind_point();
    PP_TOKEN& tok = pps.next_pp_token(true);
    if (tok.type != IDENTIFIER || tok.str != "defined") {
        pps.pp_rewind();
        return false;
    }
    
    std::string macro_name;
    tok = pps.next_pp_token(true);
    if (tok.type == IDENTIFIER) {
        macro_name = tok.str;
    } else if(tok.type == PP_OP_OR_PUNC && tok.str == "(") {
        tok = pps.next_pp_token(true);
        if (tok.type != IDENTIFIER) {
            throw pp_exception("defined() operator has no parameter provided", tok);
        }
        macro_name = tok.str;
        tok = pps.next_pp_token(true);
        if (tok.type != PP_OP_OR_PUNC || tok.str != ")") {
            throw pp_exception("Expected a closing parenthesis", tok);
        }
    } else {
        throw pp_exception("Expected an opening parenthesis or identifier", tok);
    }

    auto it = pps.macros.find(macro_name);
    if (it == pps.macros.end()) {
        *ret = 0;
    } else {
        *ret = 1;
    }
    return true;
}
bool pp_eat_literal(pp_state& pps, intmax_t* ret) {
    pps.set_pp_rewind_point();
    PP_TOKEN& tok = pps.next_pp_token(true);
    if (tok.type == PP_NUMBER) {
        // TODO:
        if (tok.number_type != PP_NUMBER_INTEGRAL) {
            throw pp_exception("Only integral numeric literals are allowed in a preprocessor constant expression", pps);
        }
        *ret = tok.integral;
        return true;
    } else if(tok.type == CHARACTER_LITERAL) {
        throw pp_exception_notimplemented("Character literals not supported yet in pp constant-expression", tok);
    } else if(tok.type == IDENTIFIER) {
        if (tok.str == "true") {
            *ret = 1;
            return true;
        } else if(tok.str == "false") {
            *ret = 0;
            return true;
        }
    }

    pps.pp_rewind();
    return false;
}
enum UNARY_OP {
    UNARY_OP_NOT,
    UNARY_OP_BINARY_NOT,
    UNARY_OP_PLUS,
    UNARY_OP_MINUS
};
bool pp_eat_unary_operator(pp_state& pps, UNARY_OP* op) {
    pps.set_pp_rewind_point();
    PP_TOKEN& tok = pps.next_pp_token(true);
    if (tok.type != PP_OP_OR_PUNC) {
        pps.pp_rewind();
        return false;
    }
    if (tok.str == "!") {
        *op = UNARY_OP_NOT;
        return true;
    }
    if (tok.str == "~") {
        *op = UNARY_OP_BINARY_NOT;
        return true;
    }
    if (tok.str == "+") {
        *op = UNARY_OP_PLUS;
        return true;
    }
    if (tok.str == "-") {
        *op = UNARY_OP_MINUS;
        return true;
    }
    pps.pp_rewind();
    return false;
}
bool pp_eat_parenthesized_constant_expression(pp_state& pps, intmax_t* ret) {
    pps.set_pp_rewind_point();
    PP_TOKEN& tok = pps.next_pp_token(true);
    if (tok.type != PP_OP_OR_PUNC || tok.str != "(") {
        pps.pp_rewind();
        return false;
    }
    if (!pp_eat_constant_expression(pps, ret)) {
        throw pp_exception("Expected a constant expression", tok);
    }
    tok = pps.next_pp_token(true);
    if (tok.type != PP_OP_OR_PUNC || tok.str != ")") {
        throw pp_exception("Expected a closing parenthesis", tok);
    }
    return true;
}
bool pp_eat_primary_expression(pp_state& pps, intmax_t* ret);
bool pp_eat_primary_expression_identifier(pp_state& pps, intmax_t* ret) {
    pps.set_pp_rewind_point();
    PP_TOKEN& tok = pps.next_pp_token(true);
    if (tok.type == IDENTIFIER) {
        std::vector<PP_TOKEN> list;
        // If macro expansion result has a defined operator in it
        // should catch it and not expand its operand
        // but only if we're parsing a preprocessor constant expression
        // in other words, only when expanding tokens after #if, #elif
        // This is not defined behavior, but microsoft code relies on it
        if (try_macro_expansion(pps, tok, list, true)) {
            pps.insert_pp_tokens(list, true);
            return pp_eat_primary_expression(pps, ret);
        } else {
            ret = 0;
            return true;
        }
    } else {
        pps.pp_rewind();
        return false;
    }
}
bool pp_eat_primary_expression(pp_state& pps, intmax_t* ret) {
    if (pp_eat_defined_operator(pps, ret)) {
        return true;
    }
    if (pp_eat_literal(pps, ret)) {
        return true;
    }
    if (pp_eat_parenthesized_constant_expression(pps, ret)) {
        return true;
    }
    UNARY_OP op;
    if (pp_eat_unary_operator(pps, &op)) {
        if (!pp_eat_primary_expression(pps, ret)) {
            throw pp_exception("Expected an expression", pps);
        }
        switch (op) {
        case UNARY_OP_NOT:
            *ret = !(*ret);
            break;
        case UNARY_OP_BINARY_NOT:
            *ret = ~(*ret);
            break;
        case UNARY_OP_PLUS:
            *ret = +(*ret);
            break;
        case UNARY_OP_MINUS:
            *ret = -(*ret);
            break;
        }
        return true;
    }
    if (pp_eat_primary_expression_identifier(pps, ret)) {
        return true;
    }
    return false;
}
bool pp_eat_multiplicative_expression(pp_state& pps, intmax_t* ret) {
    if (!pp_eat_primary_expression(pps, ret)) {
        return false;
    }

    while (true) {
        pps.set_pp_rewind_point();
        PP_TOKEN& tok = pps.next_pp_token(true);
        enum op_t {
            mul,
            div,
            mod
        };
        if (tok.type != PP_OP_OR_PUNC) {
            pps.pp_rewind();
            break;
        }
        op_t op;
        if (tok.str == "*") { op = mul; }
        else if(tok.str == "/") { op = div; }
        else if(tok.str == "%") { op = mod; }
        else {
            pps.pp_rewind();
            break;
        }
        intmax_t r = 0;
        if (!pp_eat_primary_expression(pps, &r)) {
            throw pp_exception("Expected an expression", pps);
        }
        switch (op) {
        case mul: *ret = *ret * r; break;
        case div: *ret = *ret / r; break;
        case mod: *ret = *ret % r; break;
        }
    }
    return true;
}
bool pp_eat_additive_expression(pp_state& pps, intmax_t* ret) {
    if (!pp_eat_multiplicative_expression(pps, ret)) {
        return false;
    }
    while (true) {
        pps.set_pp_rewind_point();
        PP_TOKEN& tok = pps.next_pp_token(true);
        enum op_t {
            plus,
            minus
        };
        if (tok.type != PP_OP_OR_PUNC) {
            pps.pp_rewind();
            break;
        }
        op_t op;
        if (tok.str == "+") op = plus;
        else if (tok.str == "-") op = minus;
        else {
            pps.pp_rewind();
            break;
        }
        intmax_t r = 0;
        if (!pp_eat_multiplicative_expression(pps, &r)) {
            throw pp_exception("Expected an expression", pps);
        }
        switch (op) {
        case plus: *ret = *ret + r; break;
        case minus: *ret = *ret - r; break;
        }
    }
    return true;
}
bool pp_eat_shift_expression(pp_state& pps, intmax_t* ret) {
    if (!pp_eat_additive_expression(pps, ret)) {
        return false;
    }
    while (true) {
        pps.set_pp_rewind_point();
        PP_TOKEN& tok = pps.next_pp_token(true);
        enum op_t {
            sh_left,
            sh_right
        };
        if (tok.type != PP_OP_OR_PUNC) {
            pps.pp_rewind();
            break;
        }
        op_t op;
        if (tok.str == "<<") op = sh_left;
        else if (tok.str == ">>") op = sh_right;
        else {
            pps.pp_rewind();
            break;
        }
        intmax_t r = 0;
        if (!pp_eat_additive_expression(pps, &r)) {
            throw pp_exception("Expected an expression", pps);
        }
        switch (op) {
        case sh_left: *ret = *ret << r; break;
        case sh_right: *ret = *ret >> r; break;
        }
    }
    return true;
}
bool pp_eat_relational_expression(pp_state& pps, intmax_t* ret) {
    if (!pp_eat_shift_expression(pps, ret)) {
        return false;
    }
    while (true) {
        pps.set_pp_rewind_point();
        PP_TOKEN& tok = pps.next_pp_token(true);
        enum op_t {
            less,
            more,
            less_eq,
            more_eq
        };
        if (tok.type != PP_OP_OR_PUNC) {
            pps.pp_rewind();
            break;
        }
        op_t op;
        if (tok.str == "<") op = less;
        else if (tok.str == ">") op = more;
        else if (tok.str == "<=") op = less_eq;
        else if (tok.str == ">=") op = more_eq;
        else {
            pps.pp_rewind();
            break;
        }
        intmax_t r = 0;
        if (!pp_eat_shift_expression(pps, &r)) {
            throw pp_exception("Expected an expression", pps);
        }
        switch (op) {
        case less: *ret = *ret < r ? 1 : 0; break;
        case more: *ret = *ret > r ? 1 : 0; break;
        case less_eq: *ret = *ret <= r ? 1 : 0; break;
        case more_eq: *ret = *ret >= r ? 1 : 0; break;
        }
    }
    return true;
}
bool pp_eat_equality_expression(pp_state& pps, intmax_t* ret) {
    if (!pp_eat_relational_expression(pps, ret)) {
        return false;
    }
    while (true) {
        pps.set_pp_rewind_point();
        PP_TOKEN& tok = pps.next_pp_token(true);
        enum op_t {
            eq_,
            not_eq_
        };
        if (tok.type != PP_OP_OR_PUNC) {
            pps.pp_rewind();
            break;
        }
        op_t op;
        if (tok.str == "==") op = eq_;
        else if (tok.str == "!=") op = not_eq_;
        else {
            pps.pp_rewind();
            break;
        }
        intmax_t r = 0;
        if (!pp_eat_relational_expression(pps, &r)) {
            throw pp_exception("Expected an expression", pps);
        }
        switch (op) {
        case eq_: *ret = *ret == r ? 1 : 0; break;
        case not_eq_: *ret = *ret != r ? 1 : 0; break;
        }
    }
    return true;
}
bool pp_eat_and_expression(pp_state& pps, intmax_t* ret) {
    if (!pp_eat_equality_expression(pps, ret)) {
        return false;
    }
    while (true) {
        pps.set_pp_rewind_point();
        PP_TOKEN& tok = pps.next_pp_token(true);
        if (tok.type != PP_OP_OR_PUNC) {
            pps.pp_rewind();
            break;
        }
        if (tok.str != "&") {
            pps.pp_rewind();
            break;
        }
        intmax_t r = 0;
        if (!pp_eat_equality_expression(pps, &r)) {
            throw pp_exception("Expected an expression", pps);
        }
        *ret = *ret & r;
    }
    return true;
}
bool pp_eat_exclusive_or_expression(pp_state& pps, intmax_t* ret) {
    if (!pp_eat_and_expression(pps, ret)) {
        return false;
    }
    while (true) {
        pps.set_pp_rewind_point();
        PP_TOKEN& tok = pps.next_pp_token(true);
        if (tok.type != PP_OP_OR_PUNC) {
            pps.pp_rewind();
            break;
        }
        if (tok.str != "^") {
            pps.pp_rewind();
            break;
        }
        intmax_t r = 0;
        if (!pp_eat_and_expression(pps, &r)) {
            throw pp_exception("Expected an expression", pps);
        }
        *ret = *ret ^ r;
    }
    return true;
}
bool pp_eat_inclusive_or_expression(pp_state& pps, intmax_t* ret) {
    if (!pp_eat_exclusive_or_expression(pps, ret)) {
        return false;
    }
    while (true) {
        pps.set_pp_rewind_point();
        PP_TOKEN& tok = pps.next_pp_token(true);
        if (tok.type != PP_OP_OR_PUNC) {
            pps.pp_rewind();
            break;
        }
        if (tok.str != "|") {
            pps.pp_rewind();
            break;
        }
        intmax_t r = 0;
        if (!pp_eat_exclusive_or_expression(pps, &r)) {
            throw pp_exception("Expected an expression", pps);
        }
        *ret = *ret | r;
    }
    return true;
}
bool pp_eat_logical_and_expression(pp_state& pps, intmax_t* ret) {
    if (!pp_eat_inclusive_or_expression(pps, ret)) {
        return false;
    }
    while (true) {
        pps.set_pp_rewind_point();
        PP_TOKEN& tok = pps.next_pp_token(true);
        if (tok.type != PP_OP_OR_PUNC) {
            pps.pp_rewind();
            break;
        }
        if (tok.str != "&&") {
            pps.pp_rewind();
            break;
        }
        intmax_t r = 0;
        if (!pp_eat_inclusive_or_expression(pps, &r)) {
            throw pp_exception("Expected an expression", pps);
        }
        *ret = *ret && r;
    }
    return true;
}
bool pp_eat_logical_or_expression(pp_state& pps, intmax_t* ret) {
    if (!pp_eat_logical_and_expression(pps, ret)) {
        return false;
    }
    while (true) {
        pps.set_pp_rewind_point();
        PP_TOKEN& tok = pps.next_pp_token(true);
        if (tok.type != PP_OP_OR_PUNC) {
            pps.pp_rewind();
            break;
        }
        if (tok.str != "||") {
            pps.pp_rewind();
            break;
        }
        intmax_t r = 0;
        if (!pp_eat_logical_and_expression(pps, &r)) {
            throw pp_exception("Expected an expression", pps);
        }
        *ret = *ret || r;
    }
    return true;
}
bool pp_eat_conditional_expression(pp_state& pps, intmax_t* ret) {
    if (!pp_eat_logical_or_expression(pps, ret)) {
        return false;
    }
    pps.set_pp_rewind_point();
    PP_TOKEN& tok = pps.next_pp_token(true);
    if (tok.type != PP_OP_OR_PUNC || tok.str != "?") {
        pps.pp_rewind();
        return true;
    }
    intmax_t a;
    if (!pp_eat_constant_expression(pps, &a)) {
        throw pp_exception("Expected an expression", pps);
    }
    tok = pps.next_pp_token(true);
    if (tok.type != PP_OP_OR_PUNC || tok.str != ":") {
        throw pp_exception("Expected a colon token", tok);
    }
    intmax_t b;
    if (!pp_eat_constant_expression(pps, &b)) {
        throw pp_exception("Expected an expression", pps);
    }
    if (*ret != 0) {
        *ret = a;
    } else {
        *ret = b;
    }
    return true;
}
bool pp_eat_constant_expression(pp_state& pps, intmax_t* ret) {
    if (pp_eat_conditional_expression(pps, ret)) {
        return true;
    }
    return false;
}
