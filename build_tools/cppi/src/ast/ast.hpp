#pragma once

#include <assert.h>
#include <typeinfo>
#include <vector>
#include <memory>
#include <variant>
#include "types.hpp"
#include "parsing/attribute.hpp"
#include "decl_spec_compat.hpp"
#include "expression/expression.hpp"


namespace ast {

    template<typename T>
    using uptr = std::unique_ptr<T>;


    struct node {
        virtual ~node() {}

        virtual const type_info& get_type() const = 0;

        template<typename T>
        bool is() const { return get_type() == typeid(T); }

        virtual int precedence() const { return 0; }

        virtual void dbg_print(int indent = 0) const {
            dbg_printf_color_indent("[not implemented]", indent, DBG_RED);
        }
    };

}


class ast_node {
    ast::uptr<ast::node> ptr;
public:
    ast_node() = default;

    // move
    ast_node(ast_node&& other) noexcept = default;
    ast_node& operator=(ast_node&&) noexcept = default;

    // no copy
    ast_node(const ast_node&) = delete;
    ast_node& operator=(const ast_node&) = delete;

    explicit ast_node(std::unique_ptr<ast::node> n) : ptr(std::move(n)) {}
    explicit ast_node(ast::node* n) : ptr(n) {}

    template<typename T, typename... ARGS>
    static ast_node make(ARGS&&... args) {
        return ast_node(std::make_unique<T>(std::forward<ARGS>(args)...));
    }
    static ast_node null() {
        return ast_node(nullptr);
    }


    bool is_null() const { return ptr == nullptr; }

    operator bool() const { return ptr != nullptr; }
    
    template<typename T>
    bool is() const { return ptr && ptr->is<T>(); }
    
    template<typename T>
    T* as() {
        if (!ptr) {
            return nullptr;
        }
        return dynamic_cast<T*>(ptr.get());
    }
    template<typename T>
    const T* as() const {
        if (!ptr) {
            return nullptr;
        }
        return dynamic_cast<const T*>(ptr.get());
    }

    eval_result try_evaluate() {
        expression* expr = as<expression>();
        if (!expr) {
            return eval_result::not_a_constant_expr();
        }
        return expr->evaluate();
    }

    void dbg_print(int indent = 0) const {
        if (!ptr) {
            dbg_printf_color_indent("[null]", indent, DBG_RED);
        } else {
            ptr->dbg_print(indent);
        }
    }
};


namespace ast {

#define AST_GET_TYPE \
    const type_info& get_type() const override { return typeid(*this); }
#define AST_DBG_PRINT_AUTO \
    void dbg_print(int indent = 0) const override { \
        dbg_printf_color_indent("%s", indent, DBG_RED | DBG_GREEN | DBG_BLUE, typeid(*this).name()); \
    }
#define AST_BIN_EXPR_DBG_PRINT_AUTO(NAME) \
    void dbg_print(int indent = 0) const override { \
        dbg_printf_color_indent(#NAME ":\n", indent, DBG_RED | DBG_GREEN | DBG_BLUE | DBG_BG_BLUE); \
        left.dbg_print(indent + 1); \
        printf("\n"); \
        right.dbg_print(indent + 1); \
    }

#define AST_PRECEDENCE(N) \
    int precedence() const override { return N; }
  
    inline void dbg_print_operand(int indent, const ast::node* me, const ast_node& operand) {
        bool paren = me->precedence() < operand.as<ast::node>()->precedence();
        if (paren) {
            dbg_printf_color_indent("(", indent, DBG_OPERATOR);
        }
        operand.dbg_print(0);
        if (paren) {
            dbg_printf_color_indent(")", indent, DBG_OPERATOR);
        }
    }

struct dummy : public node {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
};

struct attributes : public node {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;

    attributes(attribute_specifier&& attribs)
        : attribs(std::move(attribs)) {}

    attribute_specifier attribs;
};

struct literal : public node, public expression {
    AST_GET_TYPE;
    AST_PRECEDENCE(0);

    literal(const token& tok)
        : tok(tok) {}

    void dbg_print(int indent = 0) const override {
        if(tok.type == tt_literal) {
            dbg_printf_color_indent("%s", indent, DBG_LITERAL, tok.str.c_str());
        } else if (tok.type == tt_string_literal) {
            dbg_printf_color_indent("%s", indent, DBG_STRING_LITERAL, tok.str.c_str());
        } else {
            dbg_printf_color_indent("%s", indent, DBG_RED | DBG_GREEN | DBG_BLUE, typeid(*this).name());
        }
    }

    eval_result evaluate() override {
        if (tok.type != tt_literal) {
            return eval_result::not_implemented();
        }

        switch (tok.lit_type) {
        case l_int32: return eval_value(tok.i32);
        case l_int64: return eval_value(tok.i64);
        case l_uint32: return eval_value(tok.ui32);
        case l_uint64: return eval_value(tok.ui64);
        case l_float: return eval_value(tok.f32);
        case l_double: return eval_value(tok.double_);
        case l_long_double: return eval_value(tok.long_double_);
        // TODO:
        default:
            return eval_result::not_implemented();
        }
    }

    // TODO: Interpret the token
    token tok;
};

struct boolean_literal : public literal {
    AST_GET_TYPE;
    AST_PRECEDENCE(0);

    boolean_literal(const token& tok)
        : literal(tok) {
        if (tok.kw_type == kw_true) {
            value = true;
        } else if (tok.kw_type == kw_false) {
            value = false;
        } else {
            throw parse_exception("implementation failure: boolean_literal token must be kw_true or kw_false", tok);
        }
    }

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("%s", indent, DBG_KEYWORD, value ? "true" : "false");
    }
    
    eval_result evaluate() override {
        return eval_value(value);
    }

    bool value;
};

struct identifier : public node, public expression {
    AST_GET_TYPE;
    AST_PRECEDENCE(0);

    identifier(token tok)
        : tok(tok) {}
    identifier(token tok, symbol_ref sym)
        : tok(tok), sym(sym) {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("%s", indent, DBG_RED | DBG_GREEN | DBG_INTENSITY, tok.str.c_str());
    }

    bool is_class_name() const {
        if(!sym) return false;
        return sym->is<symbol_class>();
    }
    bool is_enum_name() const {
        if(!sym) return false;
        return sym->is<symbol_enum>();
    }
    bool is_template_name() const {
        if(!sym) return false;
        return sym->is<symbol_template>();
    }

    token tok;
    symbol_ref sym;
};

struct list : public node {
    AST_GET_TYPE;

    void add(ast_node&& n) {
        list.push_back(std::move(n));
    }
    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("list:\n", indent, DBG_RED | DBG_GREEN | DBG_BLUE | DBG_BG_BLUE);
        for (int i = 0; i < list.size(); ++i) {
            list[i].dbg_print(indent + 1);
            if (i < list.size() - 1) {
                printf("\n");
            }
        }
    }

    std::vector<ast_node> list;
};

template<typename T>
struct sequence : public node {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;

    uptr<T> node_;
    uptr<sequence<T>> next;
};

struct this_ : public node, public expression {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_PRECEDENCE(0);
};
struct size_of : public node, public expression {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_PRECEDENCE(3);

    size_of(ast_node&& arg, bool is_vararg_count = false)
        : arg(std::move(arg)), is_vararg_count(is_vararg_count) {}

    ast_node arg;
    bool is_vararg_count;
};
struct align_of : public node, public expression {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_PRECEDENCE(3);

    align_of(ast_node&& arg)
        : arg(std::move(arg)) {}

    ast_node arg;
};

struct pseudo_destructor_name : public node {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;

    pseudo_destructor_name(ast_node&& type)
        : type(std::move(type)) {}

    ast_node type;
};

struct expr_noexcept : public node, public expression {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_PRECEDENCE(0);

    expr_noexcept(ast_node&& expr)
        : expr(std::move(expr)) {}

    ast_node expr;
};

struct expr_delete : public node, public expression {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_PRECEDENCE(3);

    expr_delete(ast_node&& expr, bool is_array, bool is_scoped)
        : expr(std::move(expr)), is_array(is_array), is_scoped(is_scoped) {}

    ast_node expr;
    bool is_array;
    bool is_scoped;
};

struct expr_primary : public node, public expression {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_PRECEDENCE(0);

    // TODO:
};
struct expr_array_subscript : public node, public expression {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_PRECEDENCE(2);

    expr_array_subscript(ast_node& operand, ast_node& arg)
        : operand(std::move(operand)), arg(std::move(arg)) {}

    ast_node operand;
    ast_node arg;
};
struct expr_fn_call : public node, public expression {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_PRECEDENCE(2);

    expr_fn_call(ast_node& operand, ast_node& arg)
        : operand(std::move(operand)), arg(std::move(arg)) {}

    ast_node operand;
    ast_node arg;
};
struct expr_member_access : public node, public expression {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_PRECEDENCE(2);

    expr_member_access(ast_node& operand, ast_node& arg)
        : operand(std::move(operand)), arg(std::move(arg)) {}

    ast_node operand;
    ast_node arg;
};
struct expr_ptr_member_access : public node, public expression {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_PRECEDENCE(2);

    expr_ptr_member_access(ast_node& operand, ast_node& arg)
        : operand(std::move(operand)), arg(std::move(arg)) {}

    ast_node operand;
    ast_node arg;
};
struct expr_postfix : public node, public expression {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_PRECEDENCE(2);

    expr_postfix(ast_node&& operand, ast_node&& arg)
        : operand(std::move(operand)), arg(std::move(arg)) {}

    ast_node operand;
    ast_node arg;
};
struct expr_unary : public node, public expression {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_PRECEDENCE(3);

    expr_unary(e_unary_op op, ast_node&& expr)
        : op(op), expr(std::move(expr)) {}

    e_unary_op op;
    ast_node expr;
};

struct expr_deref : public expr_unary {
    AST_GET_TYPE;
    expr_deref(ast_node&& expr)
        : expr_unary(e_unary_deref, std::move(expr)) {}
    eval_result evaluate() override {
        eval_result e = expr.try_evaluate();
        if(!e) return e;

        // TODO: ?

        return eval_result::not_implemented();
    }
    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("%s", indent, DBG_OPERATOR, unary_op_to_string(op));
        dbg_print_operand(0, this, expr);
    }
};

struct expr_amp : public expr_unary {
    AST_GET_TYPE;
    expr_amp(ast_node&& expr)
        : expr_unary(e_unary_amp, std::move(expr)) {}
    eval_result evaluate() override {
        eval_result e = expr.try_evaluate();
        if(!e) return e;

        // TODO: ?

        return eval_result::not_implemented();
    }
    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("%s", indent, DBG_OPERATOR, unary_op_to_string(op));
        dbg_print_operand(0, this, expr);
    }
};

struct expr_plus : public expr_unary {
    AST_GET_TYPE;
    expr_plus(ast_node&& expr)
        : expr_unary(e_unary_plus, std::move(expr)) {}
    eval_result evaluate() override {
        eval_result e = expr.try_evaluate();
        if(!e) return e;

        switch (e.value.type) {
        case eval_value::e_bool: return eval_value(+e.value.boolean); break;
        //case eval_value::e_null: return eval_value(+nullptr); break;
        case eval_value::e_int8: return eval_value(+e.value.int8); break;
        case eval_value::e_uint8: return eval_value(+e.value.uint8); break;
        case eval_value::e_int16: return eval_value(+e.value.int16); break;
        case eval_value::e_uint16: return eval_value(+e.value.uint16); break;
        case eval_value::e_int32: return eval_value(+e.value.int32); break;
        case eval_value::e_uint32: return eval_value(+e.value.uint32); break;
        case eval_value::e_int64: return eval_value(+e.value.int64); break;
        case eval_value::e_uint64: return eval_value(+e.value.uint64); break;
        case eval_value::e_float: return eval_value(+e.value.float_); break;
        case eval_value::e_double: return eval_value(+e.value.double_); break;
        case eval_value::e_long_double: return eval_value(+e.value.long_double); break;
        }

        return eval_result::not_implemented();
    }
    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("%s", indent, DBG_OPERATOR, unary_op_to_string(op));
        dbg_print_operand(0, this, expr);
    }
};

struct expr_minus : public expr_unary {
    AST_GET_TYPE;
    expr_minus(ast_node&& expr)
        : expr_unary(e_unary_minus, std::move(expr)) {}
    eval_result evaluate() override {
        eval_result e = expr.try_evaluate();
        if(!e) return e;

        switch (e.value.type) {
        case eval_value::e_bool: return eval_value(-e.value.boolean); break;
        //case eval_value::e_null: return eval_value(-nullptr); break;
        case eval_value::e_int8: return eval_value(-e.value.int8); break;
        case eval_value::e_uint8: return eval_value(-e.value.uint8); break;
        case eval_value::e_int16: return eval_value(-e.value.int16); break;
        case eval_value::e_uint16: return eval_value(-e.value.uint16); break;
        case eval_value::e_int32: return eval_value(-e.value.int32); break;
        case eval_value::e_uint32: return eval_value(-e.value.uint32); break;
        case eval_value::e_int64: return eval_value(-e.value.int64); break;
        case eval_value::e_uint64: return eval_value(-e.value.uint64); break;
        case eval_value::e_float: return eval_value(-e.value.float_); break;
        case eval_value::e_double: return eval_value(-e.value.double_); break;
        case eval_value::e_long_double: return eval_value(-e.value.long_double); break;
        }

        return eval_result::not_implemented();
    }
    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("%s", indent, DBG_OPERATOR, unary_op_to_string(op));
        dbg_print_operand(0, this, expr);
    }
};

struct expr_lnot : public expr_unary {
    AST_GET_TYPE;
    expr_lnot(ast_node&& expr)
        : expr_unary(e_unary_lnot, std::move(expr)) {}
    eval_result evaluate() override {
        eval_result e = expr.try_evaluate();
        if(!e) return e;

        switch (e.value.type) {
        case eval_value::e_bool: return eval_value(!e.value.boolean); break;
        case eval_value::e_null: return eval_value(!nullptr); break;
        case eval_value::e_int8: return eval_value(!e.value.int8); break;
        case eval_value::e_uint8: return eval_value(!e.value.uint8); break;
        case eval_value::e_int16: return eval_value(!e.value.int16); break;
        case eval_value::e_uint16: return eval_value(!e.value.uint16); break;
        case eval_value::e_int32: return eval_value(!e.value.int32); break;
        case eval_value::e_uint32: return eval_value(!e.value.uint32); break;
        case eval_value::e_int64: return eval_value(!e.value.int64); break;
        case eval_value::e_uint64: return eval_value(!e.value.uint64); break;
        case eval_value::e_float: return eval_value(!e.value.float_); break;
        case eval_value::e_double: return eval_value(!e.value.double_); break;
        case eval_value::e_long_double: return eval_value(!e.value.long_double); break;
        }

        return eval_result::not_implemented();
    }
    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("%s", indent, DBG_OPERATOR, unary_op_to_string(op));
        dbg_print_operand(0, this, expr);
    }
};

struct expr_not : public expr_unary {
    AST_GET_TYPE;
    expr_not(ast_node&& expr)
        : expr_unary(e_unary_bnot, std::move(expr)) {}
    eval_result evaluate() override {
        eval_result e = expr.try_evaluate();
        if(!e) return e;

        switch (e.value.type) {
        case eval_value::e_bool: return eval_value(~e.value.boolean); break;
        //case eval_value::e_null: return eval_value(~nullptr); break;
        case eval_value::e_int8: return eval_value(~e.value.int8); break;
        case eval_value::e_uint8: return eval_value(~e.value.uint8); break;
        case eval_value::e_int16: return eval_value(~e.value.int16); break;
        case eval_value::e_uint16: return eval_value(~e.value.uint16); break;
        case eval_value::e_int32: return eval_value(~e.value.int32); break;
        case eval_value::e_uint32: return eval_value(~e.value.uint32); break;
        case eval_value::e_int64: return eval_value(~e.value.int64); break;
        case eval_value::e_uint64: return eval_value(~e.value.uint64); break;
        //case eval_value::e_float: return eval_value(~e.value.float_); break;
        //case eval_value::e_double: return eval_value(~e.value.double_); break;
        //case eval_value::e_long_double: return eval_value(~e.value.long_double); break;
        }

        return eval_result::not_implemented();
    }
    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("%s", indent, DBG_OPERATOR, unary_op_to_string(op));
        dbg_print_operand(0, this, expr);
    }
};

struct expr_typeid : public node, public expression {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_PRECEDENCE(0);

    expr_typeid(ast_node&& arg)
        : arg(std::move(arg)) {}

    ast_node arg;
};
struct expr_cast : public node, public expression {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_PRECEDENCE(3);

    expr_cast(ast_node&& type_id_, ast_node&& expr_, e_cast kind = e_cast_cstyle)
        : type_id_(std::move(type_id_)), expr_(std::move(expr_)), kind(kind) {}

    ast_node type_id_;
    ast_node expr_;
    e_cast kind;
};
struct expr_pm : public node, public expression {
    AST_GET_TYPE;
    AST_BIN_EXPR_DBG_PRINT_AUTO(expr_pm);
    AST_PRECEDENCE(4);

    expr_pm(ast_node&& left, ast_node&& right)
        : left(std::move(left)), right(std::move(right)) {}

    ast_node left;
    ast_node right;
};
struct expr_mul : public node, public expression {
    AST_GET_TYPE;
    //AST_BIN_EXPR_DBG_PRINT_AUTO(expr_mul);
    AST_PRECEDENCE(5);

    expr_mul(e_operator op, ast_node&& left, ast_node&& right)
        : op(op), left(std::move(left)), right(std::move(right)) {}

    eval_result evaluate() override {
        eval_result l = left.try_evaluate();
        eval_result r = right.try_evaluate();
        if(!l) return l;
        if(!r) return r;

        eval_convert_operands(l.value, r.value);

        switch (op) {
        case e_mul:
            switch (l.value.type) {
            case eval_value::e_bool: return eval_value(l.value.boolean * r.value.boolean); break;
            case eval_value::e_null: return eval_value(nullptr); break;
            case eval_value::e_int8: return eval_value(l.value.int8 * r.value.int8); break;
            case eval_value::e_uint8: return eval_value(l.value.uint8 * r.value.uint8); break;
            case eval_value::e_int16: return eval_value(l.value.int16 * r.value.int16); break;
            case eval_value::e_uint16: return eval_value(l.value.uint16 * r.value.uint16); break;
            case eval_value::e_int32: return eval_value(l.value.int32 * r.value.int32); break;
            case eval_value::e_uint32: return eval_value(l.value.uint32 * r.value.uint32); break;
            case eval_value::e_int64: return eval_value(l.value.int64 * r.value.int64); break;
            case eval_value::e_uint64: return eval_value(l.value.uint64 * r.value.uint64); break;
            case eval_value::e_float: return eval_value(l.value.float_ * r.value.float_); break;
            case eval_value::e_double: return eval_value(l.value.double_ * r.value.double_); break;
            case eval_value::e_long_double: return eval_value(l.value.long_double * r.value.long_double); break;
            }
            break;
        case e_div:
            switch (l.value.type) {
            case eval_value::e_bool: return eval_value(l.value.boolean / r.value.boolean); break;
            case eval_value::e_null: return eval_value(nullptr); break;
            case eval_value::e_int8: return eval_value(l.value.int8 / r.value.int8); break;
            case eval_value::e_uint8: return eval_value(l.value.uint8 / r.value.uint8); break;
            case eval_value::e_int16: return eval_value(l.value.int16 / r.value.int16); break;
            case eval_value::e_uint16: return eval_value(l.value.uint16 / r.value.uint16); break;
            case eval_value::e_int32: return eval_value(l.value.int32 / r.value.int32); break;
            case eval_value::e_uint32: return eval_value(l.value.uint32 / r.value.uint32); break;
            case eval_value::e_int64: return eval_value(l.value.int64 / r.value.int64); break;
            case eval_value::e_uint64: return eval_value(l.value.uint64 / r.value.uint64); break;
            case eval_value::e_float: return eval_value(l.value.float_ / r.value.float_); break;
            case eval_value::e_double: return eval_value(l.value.double_ / r.value.double_); break;
            case eval_value::e_long_double: return eval_value(l.value.long_double / r.value.long_double); break;
            }
            break;
        case e_mod:
            switch (l.value.type) {
            case eval_value::e_bool: return eval_value(l.value.boolean % r.value.boolean); break;
            case eval_value::e_null: return eval_value(nullptr); break;
            case eval_value::e_int8: return eval_value(l.value.int8 % r.value.int8); break;
            case eval_value::e_uint8: return eval_value(l.value.uint8 % r.value.uint8); break;
            case eval_value::e_int16: return eval_value(l.value.int16 % r.value.int16); break;
            case eval_value::e_uint16: return eval_value(l.value.uint16 % r.value.uint16); break;
            case eval_value::e_int32: return eval_value(l.value.int32 % r.value.int32); break;
            case eval_value::e_uint32: return eval_value(l.value.uint32 % r.value.uint32); break;
            case eval_value::e_int64: return eval_value(l.value.int64 % r.value.int64); break;
            case eval_value::e_uint64: return eval_value(l.value.uint64 % r.value.uint64); break;
            }
            break;
        default:
            assert(false);
            return eval_result::error();
        }
        return eval_result::error();
    }

    void dbg_print(int indent = 0) const override {
        dbg_print_operand(indent, this, left);
        dbg_printf_color_indent(" %s ", 0, DBG_OPERATOR, operator_to_string(op));
        dbg_print_operand(0, this, right);
    }

    ast_node left;
    ast_node right;
    e_operator op;
};
struct expr_add : public node, public expression {
    AST_GET_TYPE;
    //AST_BIN_EXPR_DBG_PRINT_AUTO(expr_add);
    AST_PRECEDENCE(6);

    expr_add(e_operator op, ast_node&& left, ast_node&& right)
        : op(op), left(std::move(left)), right(std::move(right)) {}

    eval_result evaluate() override {
        eval_result l = left.try_evaluate();
        eval_result r = right.try_evaluate();
        if(!l) return l;
        if(!r) return r;

        eval_convert_operands(l.value, r.value);

        switch (op) {
        case e_add:
            switch (l.value.type) {
            case eval_value::e_bool: return eval_value(l.value.boolean + r.value.boolean); break;
            case eval_value::e_null: return eval_value(nullptr); break;
            case eval_value::e_int8: return eval_value(l.value.int8 + r.value.int8); break;
            case eval_value::e_uint8: return eval_value(l.value.uint8 + r.value.uint8); break;
            case eval_value::e_int16: return eval_value(l.value.int16 + r.value.int16); break;
            case eval_value::e_uint16: return eval_value(l.value.uint16 + r.value.uint16); break;
            case eval_value::e_int32: return eval_value(l.value.int32 + r.value.int32); break;
            case eval_value::e_uint32: return eval_value(l.value.uint32 + r.value.uint32); break;
            case eval_value::e_int64: return eval_value(l.value.int64 + r.value.int64); break;
            case eval_value::e_uint64: return eval_value(l.value.uint64 + r.value.uint64); break;
            case eval_value::e_float: return eval_value(l.value.float_ + r.value.float_); break;
            case eval_value::e_double: return eval_value(l.value.double_ + r.value.double_); break;
            case eval_value::e_long_double: return eval_value(l.value.long_double + r.value.long_double); break;
            }
            break;
        case e_sub:
            switch (l.value.type) {
            case eval_value::e_bool: return eval_value(l.value.boolean - r.value.boolean); break;
            case eval_value::e_null: return eval_value(nullptr); break;
            case eval_value::e_int8: return eval_value(l.value.int8 - r.value.int8); break;
            case eval_value::e_uint8: return eval_value(l.value.uint8 - r.value.uint8); break;
            case eval_value::e_int16: return eval_value(l.value.int16 - r.value.int16); break;
            case eval_value::e_uint16: return eval_value(l.value.uint16 - r.value.uint16); break;
            case eval_value::e_int32: return eval_value(l.value.int32 - r.value.int32); break;
            case eval_value::e_uint32: return eval_value(l.value.uint32 - r.value.uint32); break;
            case eval_value::e_int64: return eval_value(l.value.int64 - r.value.int64); break;
            case eval_value::e_uint64: return eval_value(l.value.uint64 - r.value.uint64); break;
            case eval_value::e_float: return eval_value(l.value.float_ - r.value.float_); break;
            case eval_value::e_double: return eval_value(l.value.double_ - r.value.double_); break;
            case eval_value::e_long_double: return eval_value(l.value.long_double - r.value.long_double); break;
            }
            break;
        default:
            assert(false);
            return eval_result::error();
        }
        return eval_result::error();
    }

    void dbg_print(int indent = 0) const override {
        dbg_print_operand(indent, this, left);
        dbg_printf_color_indent(" %s ", 0, DBG_OPERATOR, operator_to_string(op));
        dbg_print_operand(0, this, right);
    }

    ast_node left;
    ast_node right;
    e_operator op;
};
struct expr_shift : public node, public expression {
    AST_GET_TYPE;
    //AST_BIN_EXPR_DBG_PRINT_AUTO(expr_shift);
    AST_PRECEDENCE(7);

    expr_shift(e_operator op, ast_node&& left, ast_node&& right)
        : op(op), left(std::move(left)), right(std::move(right)) {}
    
    eval_result evaluate() override {
        eval_result l = left.try_evaluate();
        eval_result r = right.try_evaluate();
        if(!l) return l;
        if(!r) return r;

        eval_convert_operands(l.value, r.value);

        switch (op) {
        case e_lshift:
            switch (l.value.type) {
            case eval_value::e_bool: return eval_value(l.value.boolean << r.value.boolean); break;
            case eval_value::e_null: return eval_value(nullptr); break;
            case eval_value::e_int8: return eval_value(l.value.int8 << r.value.int8); break;
            case eval_value::e_uint8: return eval_value(l.value.uint8 << r.value.uint8); break;
            case eval_value::e_int16: return eval_value(l.value.int16 << r.value.int16); break;
            case eval_value::e_uint16: return eval_value(l.value.uint16 << r.value.uint16); break;
            case eval_value::e_int32: return eval_value(l.value.int32 << r.value.int32); break;
            case eval_value::e_uint32: return eval_value(l.value.uint32 << r.value.uint32); break;
            case eval_value::e_int64: return eval_value(l.value.int64 << r.value.int64); break;
            case eval_value::e_uint64: return eval_value(l.value.uint64 << r.value.uint64); break;
            }
            break;
        case e_rshift:
            switch (l.value.type) {
            case eval_value::e_bool: return eval_value(l.value.boolean >> r.value.boolean); break;
            case eval_value::e_null: return eval_value(nullptr); break;
            case eval_value::e_int8: return eval_value(l.value.int8 >> r.value.int8); break;
            case eval_value::e_uint8: return eval_value(l.value.uint8 >> r.value.uint8); break;
            case eval_value::e_int16: return eval_value(l.value.int16 >> r.value.int16); break;
            case eval_value::e_uint16: return eval_value(l.value.uint16 >> r.value.uint16); break;
            case eval_value::e_int32: return eval_value(l.value.int32 >> r.value.int32); break;
            case eval_value::e_uint32: return eval_value(l.value.uint32 >> r.value.uint32); break;
            case eval_value::e_int64: return eval_value(l.value.int64 >> r.value.int64); break;
            case eval_value::e_uint64: return eval_value(l.value.uint64 >> r.value.uint64); break;
            }
            break;
        default:
            assert(false);
            return eval_result::error();
        }
        return eval_result::error();
    }

    void dbg_print(int indent = 0) const override {
        dbg_print_operand(indent, this, left);
        dbg_printf_color_indent(" %s ", 0, DBG_OPERATOR, operator_to_string(op));
        dbg_print_operand(0, this, right);
    }

    ast_node left;
    ast_node right;
    e_operator op;
};
struct expr_rel : public node, public expression {
    AST_GET_TYPE;
    //AST_BIN_EXPR_DBG_PRINT_AUTO(expr_rel);
    AST_PRECEDENCE(9);

    expr_rel(e_operator op, ast_node&& left, ast_node&& right)
        : op(op), left(std::move(left)), right(std::move(right)) {}
    
    eval_result evaluate() override {
        eval_result l = left.try_evaluate();
        eval_result r = right.try_evaluate();
        if(!l) return l;
        if(!r) return r;

        eval_convert_operands(l.value, r.value);

        switch (op) {
        case e_less:
            switch (l.value.type) {
            case eval_value::e_bool: return eval_value(l.value.boolean < r.value.boolean); break;
            case eval_value::e_null: return eval_value(nullptr); break;
            case eval_value::e_int8: return eval_value(l.value.int8 < r.value.int8); break;
            case eval_value::e_uint8: return eval_value(l.value.uint8 < r.value.uint8); break;
            case eval_value::e_int16: return eval_value(l.value.int16 < r.value.int16); break;
            case eval_value::e_uint16: return eval_value(l.value.uint16 < r.value.uint16); break;
            case eval_value::e_int32: return eval_value(l.value.int32 < r.value.int32); break;
            case eval_value::e_uint32: return eval_value(l.value.uint32 < r.value.uint32); break;
            case eval_value::e_int64: return eval_value(l.value.int64 < r.value.int64); break;
            case eval_value::e_uint64: return eval_value(l.value.uint64 < r.value.uint64); break;
            case eval_value::e_float: return eval_value(l.value.float_ < r.value.float_); break;
            case eval_value::e_double: return eval_value(l.value.double_ < r.value.double_); break;
            case eval_value::e_long_double: return eval_value(l.value.long_double < r.value.long_double); break;
            }
            break;
        case e_more:
            switch (l.value.type) {
            case eval_value::e_bool: return eval_value(l.value.boolean > r.value.boolean); break;
            case eval_value::e_null: return eval_value(nullptr); break;
            case eval_value::e_int8: return eval_value(l.value.int8 > r.value.int8); break;
            case eval_value::e_uint8: return eval_value(l.value.uint8 > r.value.uint8); break;
            case eval_value::e_int16: return eval_value(l.value.int16 > r.value.int16); break;
            case eval_value::e_uint16: return eval_value(l.value.uint16 > r.value.uint16); break;
            case eval_value::e_int32: return eval_value(l.value.int32 > r.value.int32); break;
            case eval_value::e_uint32: return eval_value(l.value.uint32 > r.value.uint32); break;
            case eval_value::e_int64: return eval_value(l.value.int64 > r.value.int64); break;
            case eval_value::e_uint64: return eval_value(l.value.uint64 > r.value.uint64); break;
            case eval_value::e_float: return eval_value(l.value.float_ > r.value.float_); break;
            case eval_value::e_double: return eval_value(l.value.double_ > r.value.double_); break;
            case eval_value::e_long_double: return eval_value(l.value.long_double > r.value.long_double); break;
            }
            break;
        case e_lesseq:
            switch (l.value.type) {
            case eval_value::e_bool: return eval_value(l.value.boolean <= r.value.boolean); break;
            case eval_value::e_null: return eval_value(nullptr); break;
            case eval_value::e_int8: return eval_value(l.value.int8 <= r.value.int8); break;
            case eval_value::e_uint8: return eval_value(l.value.uint8 <= r.value.uint8); break;
            case eval_value::e_int16: return eval_value(l.value.int16 <= r.value.int16); break;
            case eval_value::e_uint16: return eval_value(l.value.uint16 <= r.value.uint16); break;
            case eval_value::e_int32: return eval_value(l.value.int32 <= r.value.int32); break;
            case eval_value::e_uint32: return eval_value(l.value.uint32 <= r.value.uint32); break;
            case eval_value::e_int64: return eval_value(l.value.int64 <= r.value.int64); break;
            case eval_value::e_uint64: return eval_value(l.value.uint64 <= r.value.uint64); break;
            case eval_value::e_float: return eval_value(l.value.float_ <= r.value.float_); break;
            case eval_value::e_double: return eval_value(l.value.double_ <= r.value.double_); break;
            case eval_value::e_long_double: return eval_value(l.value.long_double <= r.value.long_double); break;
            }
            break;
        case e_moreeq:
            switch (l.value.type) {
            case eval_value::e_bool: return eval_value(l.value.boolean >= r.value.boolean); break;
            case eval_value::e_null: return eval_value(nullptr); break;
            case eval_value::e_int8: return eval_value(l.value.int8 >= r.value.int8); break;
            case eval_value::e_uint8: return eval_value(l.value.uint8 >= r.value.uint8); break;
            case eval_value::e_int16: return eval_value(l.value.int16 >= r.value.int16); break;
            case eval_value::e_uint16: return eval_value(l.value.uint16 >= r.value.uint16); break;
            case eval_value::e_int32: return eval_value(l.value.int32 >= r.value.int32); break;
            case eval_value::e_uint32: return eval_value(l.value.uint32 >= r.value.uint32); break;
            case eval_value::e_int64: return eval_value(l.value.int64 >= r.value.int64); break;
            case eval_value::e_uint64: return eval_value(l.value.uint64 >= r.value.uint64); break;
            case eval_value::e_float: return eval_value(l.value.float_ >= r.value.float_); break;
            case eval_value::e_double: return eval_value(l.value.double_ >= r.value.double_); break;
            case eval_value::e_long_double: return eval_value(l.value.long_double >= r.value.long_double); break;
            }
            break;
        default:
            assert(false);
            return eval_result::error();
        }
        return eval_result::error();
    }

    void dbg_print(int indent = 0) const override {
        dbg_print_operand(indent, this, left);
        dbg_printf_color_indent(" %s ", 0, DBG_OPERATOR, operator_to_string(op));
        dbg_print_operand(0, this, right);
    }

    ast_node left;
    ast_node right;
    e_operator op;
};
struct expr_eq : public node, public expression {
    AST_GET_TYPE;
    //AST_BIN_EXPR_DBG_PRINT_AUTO(expr_eq);
    AST_PRECEDENCE(10);

    expr_eq(e_operator op, ast_node&& left, ast_node&& right)
        : op(op), left(std::move(left)), right(std::move(right)) {}
    
    eval_result evaluate() override {
        eval_result l = left.try_evaluate();
        eval_result r = right.try_evaluate();
        if(!l) return l;
        if(!r) return r;

        eval_convert_operands(l.value, r.value);

        switch (op) {
        case e_equal:
            switch (l.value.type) {
            case eval_value::e_bool: return eval_value(l.value.boolean == r.value.boolean); break;
            case eval_value::e_null: return eval_value(nullptr); break;
            case eval_value::e_int8: return eval_value(l.value.int8 == r.value.int8); break;
            case eval_value::e_uint8: return eval_value(l.value.uint8 == r.value.uint8); break;
            case eval_value::e_int16: return eval_value(l.value.int16 == r.value.int16); break;
            case eval_value::e_uint16: return eval_value(l.value.uint16 == r.value.uint16); break;
            case eval_value::e_int32: return eval_value(l.value.int32 == r.value.int32); break;
            case eval_value::e_uint32: return eval_value(l.value.uint32 == r.value.uint32); break;
            case eval_value::e_int64: return eval_value(l.value.int64 == r.value.int64); break;
            case eval_value::e_uint64: return eval_value(l.value.uint64 == r.value.uint64); break;
            }
            break;
        case e_notequal:
            switch (l.value.type) {
            case eval_value::e_bool: return eval_value(l.value.boolean != r.value.boolean); break;
            case eval_value::e_null: return eval_value(nullptr); break;
            case eval_value::e_int8: return eval_value(l.value.int8 != r.value.int8); break;
            case eval_value::e_uint8: return eval_value(l.value.uint8 != r.value.uint8); break;
            case eval_value::e_int16: return eval_value(l.value.int16 != r.value.int16); break;
            case eval_value::e_uint16: return eval_value(l.value.uint16 != r.value.uint16); break;
            case eval_value::e_int32: return eval_value(l.value.int32 != r.value.int32); break;
            case eval_value::e_uint32: return eval_value(l.value.uint32 != r.value.uint32); break;
            case eval_value::e_int64: return eval_value(l.value.int64 != r.value.int64); break;
            case eval_value::e_uint64: return eval_value(l.value.uint64 != r.value.uint64); break;
            }
            break;
        default:
            assert(false);
            return eval_result::error();
        }
        return eval_result::error();
    }

    void dbg_print(int indent = 0) const override {
        dbg_print_operand(indent, this, left);
        dbg_printf_color_indent(" %s ", 0, DBG_OPERATOR, operator_to_string(op));
        dbg_print_operand(0, this, right);
    }

    ast_node left;
    ast_node right;
    e_operator op;
};
struct expr_and : public node, public expression {
    AST_GET_TYPE;
    //AST_BIN_EXPR_DBG_PRINT_AUTO(expr_and);
    AST_PRECEDENCE(11);

    expr_and(ast_node&& left, ast_node&& right)
        : left(std::move(left)), right(std::move(right)) {}
    
    eval_result evaluate() override {
        eval_result l = left.try_evaluate();
        eval_result r = right.try_evaluate();
        if(!l) return l;
        if(!r) return r;

        eval_convert_operands(l.value, r.value);

        switch (l.value.type) {
        case eval_value::e_bool: return eval_value(l.value.boolean & r.value.boolean); break;
        case eval_value::e_null: return eval_value(nullptr); break;
        case eval_value::e_int8: return eval_value(l.value.int8 & r.value.int8); break;
        case eval_value::e_uint8: return eval_value(l.value.uint8 & r.value.uint8); break;
        case eval_value::e_int16: return eval_value(l.value.int16 & r.value.int16); break;
        case eval_value::e_uint16: return eval_value(l.value.uint16 & r.value.uint16); break;
        case eval_value::e_int32: return eval_value(l.value.int32 & r.value.int32); break;
        case eval_value::e_uint32: return eval_value(l.value.uint32 & r.value.uint32); break;
        case eval_value::e_int64: return eval_value(l.value.int64 & r.value.int64); break;
        case eval_value::e_uint64: return eval_value(l.value.uint64 & r.value.uint64); break;
        }

        return eval_result::error();
    }

    void dbg_print(int indent = 0) const override {
        dbg_print_operand(indent, this, left);
        dbg_printf_color_indent(" %s ", 0, DBG_OPERATOR, operator_to_string(e_and));
        dbg_print_operand(0, this, right);
    }

    ast_node left;
    ast_node right;
};
struct expr_exor : public node, public expression {
    AST_GET_TYPE;
    //AST_BIN_EXPR_DBG_PRINT_AUTO(expr_exor);
    AST_PRECEDENCE(12);

    expr_exor(ast_node&& left, ast_node&& right)
        : left(std::move(left)), right(std::move(right)) {}
    
    eval_result evaluate() override {
        eval_result l = left.try_evaluate();
        eval_result r = right.try_evaluate();
        if(!l) return l;
        if(!r) return r;

        eval_convert_operands(l.value, r.value);

        switch (l.value.type) {
        case eval_value::e_bool: return eval_value(l.value.boolean ^ r.value.boolean); break;
        case eval_value::e_null: return eval_value(nullptr); break;
        case eval_value::e_int8: return eval_value(l.value.int8 ^ r.value.int8); break;
        case eval_value::e_uint8: return eval_value(l.value.uint8 ^ r.value.uint8); break;
        case eval_value::e_int16: return eval_value(l.value.int16 ^ r.value.int16); break;
        case eval_value::e_uint16: return eval_value(l.value.uint16 ^ r.value.uint16); break;
        case eval_value::e_int32: return eval_value(l.value.int32 ^ r.value.int32); break;
        case eval_value::e_uint32: return eval_value(l.value.uint32 ^ r.value.uint32); break;
        case eval_value::e_int64: return eval_value(l.value.int64 ^ r.value.int64); break;
        case eval_value::e_uint64: return eval_value(l.value.uint64 ^ r.value.uint64); break;
        }

        return eval_result::error();
    }

    void dbg_print(int indent = 0) const override {
        dbg_print_operand(indent, this, left);
        dbg_printf_color_indent(" %s ", 0, DBG_OPERATOR, operator_to_string(e_exor));
        dbg_print_operand(0, this, right);
    }

    ast_node left;
    ast_node right;
};  
struct expr_ior : public node, public expression {
    AST_GET_TYPE;
    //AST_BIN_EXPR_DBG_PRINT_AUTO(expr_ior);
    AST_PRECEDENCE(13);

    expr_ior(ast_node&& left, ast_node&& right)
        : left(std::move(left)), right(std::move(right)) {}
    
    eval_result evaluate() override {
        eval_result l = left.try_evaluate();
        eval_result r = right.try_evaluate();
        if(!l) return l;
        if(!r) return r;

        eval_convert_operands(l.value, r.value);

        switch (l.value.type) {
        case eval_value::e_bool: return eval_value(l.value.boolean | r.value.boolean); break;
        case eval_value::e_null: return eval_value(nullptr); break;
        case eval_value::e_int8: return eval_value(l.value.int8 | r.value.int8); break;
        case eval_value::e_uint8: return eval_value(l.value.uint8 | r.value.uint8); break;
        case eval_value::e_int16: return eval_value(l.value.int16 | r.value.int16); break;
        case eval_value::e_uint16: return eval_value(l.value.uint16 | r.value.uint16); break;
        case eval_value::e_int32: return eval_value(l.value.int32 | r.value.int32); break;
        case eval_value::e_uint32: return eval_value(l.value.uint32 | r.value.uint32); break;
        case eval_value::e_int64: return eval_value(l.value.int64 | r.value.int64); break;
        case eval_value::e_uint64: return eval_value(l.value.uint64 | r.value.uint64); break;
        }

        return eval_result::error();
    }

    void dbg_print(int indent = 0) const override {
        dbg_print_operand(indent, this, left);
        dbg_printf_color_indent(" %s ", 0, DBG_OPERATOR, operator_to_string(e_ior));
        dbg_print_operand(0, this, right);
    }

    ast_node left;
    ast_node right;
};
struct expr_land : public node, public expression {
    AST_GET_TYPE;
    //AST_BIN_EXPR_DBG_PRINT_AUTO(expr_land);
    AST_PRECEDENCE(14);

    expr_land(ast_node&& left, ast_node&& right)
        : left(std::move(left)), right(std::move(right)) {}
    
    eval_result evaluate() override {
        eval_result l = left.try_evaluate();
        eval_result r = right.try_evaluate();
        if(!l) return l;
        if(!r) return r;

        eval_convert_operands(l.value, r.value);

        switch (l.value.type) {
        case eval_value::e_bool: return eval_value(l.value.boolean && r.value.boolean); break;
        case eval_value::e_null: return eval_value(nullptr); break;
        case eval_value::e_int8: return eval_value(l.value.int8 && r.value.int8); break;
        case eval_value::e_uint8: return eval_value(l.value.uint8 && r.value.uint8); break;
        case eval_value::e_int16: return eval_value(l.value.int16 && r.value.int16); break;
        case eval_value::e_uint16: return eval_value(l.value.uint16 && r.value.uint16); break;
        case eval_value::e_int32: return eval_value(l.value.int32 && r.value.int32); break;
        case eval_value::e_uint32: return eval_value(l.value.uint32 && r.value.uint32); break;
        case eval_value::e_int64: return eval_value(l.value.int64 && r.value.int64); break;
        case eval_value::e_uint64: return eval_value(l.value.uint64 && r.value.uint64); break;
        }

        return eval_result::error();
    }

    void dbg_print(int indent = 0) const override {
        dbg_print_operand(indent, this, left);
        dbg_printf_color_indent(" %s ", 0, DBG_OPERATOR, operator_to_string(e_land));
        dbg_print_operand(0, this, right);
    }

    ast_node left;
    ast_node right;
};
struct expr_lor : public node, public expression {
    AST_GET_TYPE;
    //AST_BIN_EXPR_DBG_PRINT_AUTO(expr_lor);
    AST_PRECEDENCE(15);

    expr_lor(ast_node&& left, ast_node&& right)
        : left(std::move(left)), right(std::move(right)) {}
    
    eval_result evaluate() override {
        eval_result l = left.try_evaluate();
        eval_result r = right.try_evaluate();
        if(!l) return l;
        if(!r) return r;

        eval_convert_operands(l.value, r.value);

        switch (l.value.type) {
        case eval_value::e_bool: return eval_value(l.value.boolean || r.value.boolean); break;
        case eval_value::e_null: return eval_value(nullptr); break;
        case eval_value::e_int8: return eval_value(l.value.int8 || r.value.int8); break;
        case eval_value::e_uint8: return eval_value(l.value.uint8 || r.value.uint8); break;
        case eval_value::e_int16: return eval_value(l.value.int16 || r.value.int16); break;
        case eval_value::e_uint16: return eval_value(l.value.uint16 || r.value.uint16); break;
        case eval_value::e_int32: return eval_value(l.value.int32 || r.value.int32); break;
        case eval_value::e_uint32: return eval_value(l.value.uint32 || r.value.uint32); break;
        case eval_value::e_int64: return eval_value(l.value.int64 || r.value.int64); break;
        case eval_value::e_uint64: return eval_value(l.value.uint64 || r.value.uint64); break;
        }

        return eval_result::error();
    }

    void dbg_print(int indent = 0) const override {
        dbg_print_operand(indent, this, left);
        dbg_printf_color_indent(" %s ", 0, DBG_OPERATOR, operator_to_string(e_lor));
        dbg_print_operand(0, this, right);
    }

    ast_node left;
    ast_node right;
};
struct expr_cond : public node, public expression {
    AST_GET_TYPE;
    
    expr_cond(ast_node&& cond, ast_node&& a, ast_node&& b)
        : cond(std::move(cond)), a(std::move(a)), b(std::move(b)) {}

    ast_node cond;
    ast_node a;
    ast_node b;

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("expr_cond:\n", indent, DBG_RED | DBG_GREEN | DBG_BLUE | DBG_BG_BLUE);
        cond.dbg_print(indent + 1);
        printf("\n");
        a.dbg_print(indent + 1);
        printf("\n");
        b.dbg_print(indent + 1);
    }
};
struct expr_assign : public node, public expression {
    AST_GET_TYPE;
    AST_BIN_EXPR_DBG_PRINT_AUTO(expr_assign);

    expr_assign(ast_node&& left, ast_node&& right)
        : left(std::move(left)), right(std::move(right)) {}

    ast_node left;
    ast_node right;
};

struct init_list : public node {
    AST_GET_TYPE;

    init_list() {}
    void add(ast_node&& n) {
        list.push_back(std::move(n));
    }
    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("init_list:\n", indent, DBG_RED | DBG_GREEN | DBG_BLUE | DBG_BG_BLUE);
        for (int i = 0; i < list.size(); ++i) {
            list[i].dbg_print(indent + 1);
            if (i < list.size() - 1) {
                printf("\n");
            }
        }
    }

    std::vector<ast_node> list;
};
struct braced_init_list : public node {
    AST_GET_TYPE;

    braced_init_list(ast_node&& list)
        : list(std::move(list)) {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("braced_init_list:\n", indent, DBG_RED | DBG_GREEN | DBG_BLUE | DBG_BG_BLUE);
        list.dbg_print(indent + 1);
    }

    ast_node list;
};

struct simple_type_specifier : public node {
    AST_GET_TYPE;

    simple_type_specifier(e_keyword kw)
        : kw(kw) {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("%s", indent, DBG_KEYWORD, keyword_to_string(kw));
    }

    e_keyword kw;
};

struct cv_qualifier : public node {
    AST_GET_TYPE;

    cv_qualifier(e_cv cv)
        : cv(cv) {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("%s", indent, DBG_KEYWORD, cv_to_string(cv));
    }

    e_cv cv;
};

struct cv_qualifier_seq : public node {
    AST_GET_TYPE;

    void push_back(ast_node&& cv) {
        seq.push_back(std::move(cv));
    }
    void dbg_print(int indent = 0) const override {
        for (int i = 0; i < seq.size(); ++i) {
            seq[i].dbg_print(0);
            if (i < seq.size() - 1) {
                dbg_printf_color_indent(" ", 0, DBG_WHITE);
            }
        }
    }

    std::vector<ast_node> seq;
};

struct elaborated_type_specifier : public node {
    AST_GET_TYPE;

    elaborated_type_specifier(e_class_key key, ast_node&& name)
        : type(e_elaborated_class), class_key(key), name(std::move(name)) {}
    elaborated_type_specifier(e_enum_key key, ast_node&& name)
        : type(e_elaborated_enum), enum_key(key), name(std::move(name)) {}

    void dbg_print(int indent = 0) const override {
        if (type == e_elaborated_class) {
            dbg_printf_color_indent(class_key_to_string(class_key), indent, DBG_KEYWORD);
        } else if (type == e_elaborated_enum) {
            dbg_printf_color_indent(enum_key_to_string(enum_key), indent, DBG_KEYWORD);
        } else {
            assert(false);
            dbg_printf_color_indent("elaborated_type_specifier type error", indent, DBG_RED);
        }
        dbg_printf_color_indent(" ", 0, DBG_WHITE);
        name.dbg_print(0);
        dbg_printf_color_indent("/*elaborated_type_specifier*/", 0, DBG_COMMENT);
    }

    e_elaborated_type type;
    union {
        e_class_key class_key;
        e_enum_key enum_key;
    };
    ast_node name;
};

struct declarator_part : public node {
    AST_GET_TYPE;

    declarator_part()
        : next(nullptr) {}
    declarator_part(ast_node&& next)
        : next(std::move(next)) {}

    virtual void dbg_print_declarator_part(int indent = 0) const = 0;

    ast_node next;
};

struct noptr_declarator;
struct parameters_and_qualifiers;
struct ptr_operator : public node {
    AST_GET_TYPE;

    ptr_operator(e_ptr_operator type)
        : type(type) {}

    void dbg_print(int indent = 0) const override {
        switch (type) {
        case e_ptr:
            dbg_printf_color_indent("*", indent, DBG_OPERATOR);
            break;
        case e_ref:
            dbg_printf_color_indent("&", indent, DBG_OPERATOR);
            break;
        case e_rvref:
            dbg_printf_color_indent("&&", indent, DBG_OPERATOR);
            break;
        default: assert(false);
        }
    }

    e_ptr_operator type;
};

struct type_specifier_seq : public node {
    AST_GET_TYPE;

    type_specifier_seq() {}

    void push_back(ast_node&& n) {
        seq.push_back(std::move(n));
    }

    void dbg_print(int indent = 0) const override {
        for (int i = 0; i < seq.size(); ++i) {
            seq[i].dbg_print(0);
            if (i < seq.size() - 1) {
                dbg_printf_color_indent(" ", 0, DBG_WHITE);
            }
        }
    }

    std::vector<ast_node> seq;
};

#include <deque>
struct parameters_and_qualifiers;
struct identifier;
struct ptr_declarator;
struct declarator : public declarator_part {
    AST_GET_TYPE;

    declarator()
        : decl(ast_node::null()) {}
    declarator(ast_node&& decl)
        : decl(std::move(decl)), declarator_part(ast_node::null()) {}
    declarator(ast_node&& decl, ast_node&& n)
        : decl(std::move(decl)), declarator_part(std::move(n)) {}

    bool is_abstract() const {
        return decl.is_null();
    }

    void dbg_print(int indent = 0) const override {
        std::vector<const declarator_part*> parts;
        const declarator_part* nxt = next.as<declarator_part>();
        while (nxt) {
            parts.push_back(nxt);
            nxt = nxt->next.as<declarator_part>();
        }

        struct PART {
            const declarator_part* p;
            int depth;
        };
        std::deque<PART> sorted;
        sorted.push_back({ this, 0 });
        const declarator_part* prev = this;
        int nesting_depth = 0; // reverse, 0 is the most nested
        for (int i = 0; i < parts.size(); ++i) {
            auto p = parts[i];
            if (p->is<ptr_declarator>()) {
                sorted.push_front({ p, nesting_depth });
            } else if (p->is<noptr_declarator>() || p->is<parameters_and_qualifiers>()) {
                if (prev->is<ptr_declarator>()) {
                    ++nesting_depth;
                }
                sorted.push_back({ p, nesting_depth });
            }
            prev = p;
        }

        int ind = indent;
        for (int i = 0; i < sorted.size(); ++i) {
            //printf("%i", sorted[i].depth);
            if (nesting_depth > sorted[i].depth) {
                dbg_printf_color_indent("(", ind, DBG_OPERATOR);
                nesting_depth = sorted[i].depth;
                ind = 0;
            } else if (nesting_depth < sorted[i].depth) {
                dbg_printf_color_indent(")", 0, DBG_OPERATOR);
                nesting_depth = sorted[i].depth;
            }
            sorted[i].p->dbg_print_declarator_part(ind);
            ind = 0;
            if (i < sorted.size() - 1 &&
                sorted[i].p->is<ast::ptr_declarator>() &&
                sorted[i + 1].p->is<ast::ptr_declarator>() &&
                sorted[i].depth == sorted[i + 1].depth
            ) {
                dbg_printf_color_indent(" ", 0, DBG_WHITE);
            }
        }
    }
    void dbg_print_declarator_part(int indent = 0) const override {
        if(!decl) return;
        decl.dbg_print(indent);
    }

    bool is_function() const {
        return next.is<ast::parameters_and_qualifiers>();
    }
    bool is_plain_identifier() const {
        return decl.is<ast::identifier>() && next.is_null();
    }

    ast_node decl;
};

struct ptr_declarator : public declarator_part {
    AST_GET_TYPE;

    ptr_declarator()
        : declarator_part(ast_node::null()) {}
    ptr_declarator(ast_node&& ptr_op, ast_node&& n)
        : declarator_part(std::move(n)), ptr_op(std::move(ptr_op)) {}

    void dbg_print(int indent = 0) const override {
        ptr_op.dbg_print(indent);
    }
    void dbg_print_declarator_part(int indent = 0) const override {
        ptr_op.dbg_print(indent);
    }

    ast_node ptr_op;
};
// noptr-declarator
struct noptr_declarator : public declarator_part {
    AST_GET_TYPE;

    noptr_declarator()
        : declarator_part(ast_node::null()) {}
    noptr_declarator(ast_node&& n)
        : declarator_part(std::move(n)) {}

    void dbg_print(int indent = 0) const override {
        /*if (next) {
            //dbg_printf_color_indent(" ", 0, DBG_WHITE);
            next.dbg_print(0);
        }*/
        dbg_printf_color_indent("[]", indent, DBG_RED | DBG_GREEN | DBG_BLUE | DBG_BG_BLUE);
    }
    void dbg_print_declarator_part(int indent = 0) const override {
        dbg_printf_color_indent("[]", indent, DBG_OPERATOR);
    }
};

struct operator_function_id : public node {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
};
struct conversion_declarator : public node {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
};
struct conversion_type_id : public node {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
};
struct conversion_function_id : public node {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
};

struct storage_class_specifier : public node {
    AST_GET_TYPE;

    storage_class_specifier(e_keyword kw)
        : kw(kw) {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("%s", indent, DBG_KEYWORD, keyword_to_string(kw));
    }

    e_keyword kw;
};
struct function_specifier : public node {
    AST_GET_TYPE;

    function_specifier(e_keyword kw)
        : kw(kw) {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("%s", indent, DBG_KEYWORD, keyword_to_string(kw));
    }

    e_keyword kw;
};
struct decl_specifier : public node {
    AST_GET_TYPE;

    decl_specifier(e_keyword kw)
        : kw(kw) {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("%s", indent, DBG_KEYWORD, keyword_to_string(kw));
    }

    e_keyword kw;
};
struct decl_specifier_seq : public node {
    AST_GET_TYPE;

    void push_back(ast_node&& n) {
        specifiers.push_back(std::move(n));
    }
    void dbg_print(int indent = 0) const override {
        int ind = indent;
        for (int i = 0; i < specifiers.size(); ++i) {
            specifiers[i].dbg_print(ind);
            if (i < specifiers.size() - 1) {
                printf(" ");
            }
            ind = 0;
        }
    }

    decl_flags flags;
    std::vector<ast_node> specifiers;
};
struct parameter_declaration : public node {
    AST_GET_TYPE;

    parameter_declaration(ast_node&& decl_spec, ast_node&& declarator)
        : decl_spec(std::move(decl_spec)), declarator(std::move(declarator)), initializer(ast_node::null()) {}
    parameter_declaration(ast_node&& decl_spec, ast_node&& declarator, ast_node&& initializer)
        : decl_spec(std::move(decl_spec)), declarator(std::move(declarator)), initializer(std::move(initializer)) {}

    void dbg_print(int indent = 0) const override {
        int ind = indent;
        if (decl_spec) {
            decl_spec.dbg_print(ind);
            ind = 0;
        }
        if (declarator) {
            dbg_printf_color_indent(" ", 0, DBG_WHITE);
            declarator.dbg_print(ind);
            ind = 0;
        }
        if (initializer) {
            dbg_printf_color_indent(" = ", 0, DBG_WHITE);
            initializer.dbg_print(ind);
            ind = 0;
        }
    }

    ast_node decl_spec;
    ast_node declarator;
    ast_node initializer;
};
struct parameter_declaration_list : public node {
    AST_GET_TYPE;

    void push_back(ast_node&& n) {
        params.push_back(std::move(n));
    }

    void dbg_print(int indent = 0) const override {
        int ind = indent;
        for (int i = 0; i < params.size(); ++i) {
            params[i].dbg_print(ind);
            if (i < params.size() - 1) {
                printf(", ");
            }
            ind = 0;
        }
    }

    std::vector<ast_node> params;
    bool is_variadic = false;
};
struct ref_qualifier : public node {
    AST_GET_TYPE;
    
    ref_qualifier(e_ptr_operator type)
        : type(type) {}

    void dbg_print(int indent = 0) const override {
        switch (type) {
        case e_ref:
            dbg_printf_color_indent("&", indent, DBG_RED | DBG_GREEN | DBG_BLUE | DBG_BG_BLUE);
            break;
        case e_rvref:
            dbg_printf_color_indent("&&", indent, DBG_RED | DBG_GREEN | DBG_BLUE | DBG_BG_BLUE);
            break;
        default: assert(false);
        }
    }

    e_ptr_operator type;
};
// parameters-and-qualifiers
struct parameters_and_qualifiers : public declarator_part {
    AST_GET_TYPE;

    parameters_and_qualifiers(ast_node&& param_decl_list, ast_node&& cv, ast_node&& ref, ast_node&& ex_spec)
        : param_decl_list(std::move(param_decl_list)), cv(std::move(cv)), ref(std::move(ref)), ex_spec(std::move(ex_spec)) {}

    void dbg_print(int indent = 0) const override {
        /*if (next) {
            dbg_printf_color_indent(" ", 0, DBG_WHITE);
            next.dbg_print(0);
        }*/
        dbg_printf_color_indent("()", indent, DBG_RED | DBG_GREEN | DBG_BLUE | DBG_BG_BLUE);
    }
    void dbg_print_declarator_part(int indent = 0) const override {
        dbg_printf_color_indent("(", indent, DBG_OPERATOR);
        if (param_decl_list) {
            param_decl_list.dbg_print(0);
        }
        dbg_printf_color_indent(")", 0, DBG_OPERATOR);
        if (cv) {
            dbg_printf_color_indent(" ", 0, DBG_WHITE);
            cv.dbg_print(0);
        }
        if (ref) {
            dbg_printf_color_indent(" ", 0, DBG_WHITE);
            ref.dbg_print(0);
        }
        if (ex_spec) {
            dbg_printf_color_indent(" ", 0, DBG_WHITE);
            ex_spec.dbg_print(0);
        }
    }

    ast_node param_decl_list;
    ast_node cv;
    ast_node ref;
    ast_node ex_spec;
};

struct type_id : public node {
    AST_GET_TYPE;

    type_id(ast_node&& type_spec_seq, ast_node&& declarator_)
        : type_spec_seq(std::move(type_spec_seq)), declarator_(std::move(declarator_)) {}

    void dbg_print(int indent = 0) const override {
        type_spec_seq.dbg_print(0);
        if(declarator_) {
            printf(" ");
            declarator_.dbg_print(0);
        }
    }

    ast_node type_spec_seq;
    ast_node declarator_;
};

struct type_id_list : public node {
    AST_GET_TYPE;
    
    void push_back(ast_node&& n) {
        list.push_back(std::move(n));
    }
    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("type_id_list:\n", indent, DBG_RED | DBG_GREEN | DBG_BLUE | DBG_BG_BLUE);
        for (int i = 0; i < list.size(); ++i) {
            list[i].dbg_print(indent + 1);
            if (i < list.size() - 1) {
                printf(", ");
            }
        }
    }

    std::vector<ast_node> list;

};


struct nested_name_specifier : public node {
    AST_GET_TYPE;

    nested_name_specifier()
        : n(nullptr), next(nullptr) {}
    nested_name_specifier(ast_node&& n)
        : n(std::move(n)), next(nullptr) {}
    nested_name_specifier(ast_node&& n, ast_node&& next)
        : n(std::move(n)), next(std::move(next)) {}

    void set_next(ast_node&& n) {
        next = std::move(n);
    }
    void push_next(ast_node&& n) {
        if (next.is_null()) {
            next = std::move(n);
            return;
        }

        if (!next.is<nested_name_specifier>()) {
            throw parse_exception("nested_name_specifier already has an ending", token());
        }

        next.as<nested_name_specifier>()->push_next(std::move(n));
    }

    void dbg_print(int indent = 0) const override {
        if(n) {
            n.dbg_print(indent);
        }
        dbg_printf_color_indent("::", indent, DBG_WHITE);
        next.dbg_print(0);
    }

    ast_node n;
    ast_node next;
};

struct dynamic_exception_spec : public node {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;

    dynamic_exception_spec(ast_node&& type_id_list_)
        : type_id_list_(std::move(type_id_list_)) {}

    ast_node type_id_list_;
};
struct noexcept_spec : public node {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;

    noexcept_spec()
        : const_expr_(nullptr) {}
    noexcept_spec(ast_node&& const_expr_)
        : const_expr_(std::move(const_expr_)) {}

    ast_node const_expr_;
};

struct class_name : public node {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
};
struct template_argument_list : public node {
    AST_GET_TYPE;

    template_argument_list() {}

    void push_back(ast_node&& n) {
        list.push_back(std::move(n));
    }

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("<", 0, DBG_WHITE);
        for (int i = 0; i < list.size(); ++i) {
            list[i].dbg_print(0);
            if (i < list.size() - 1) {
                dbg_printf_color_indent(", ", 0, DBG_WHITE);
            }
        }
        dbg_printf_color_indent(">", 0, DBG_WHITE);
    }

    std::vector<ast_node> list;
};
struct simple_template_id : public node {
    AST_GET_TYPE;

    simple_template_id(ast_node ident, ast_node&& args)
        : ident(std::move(ident)), template_argument_list(std::move(args)) {}

    void dbg_print(int indent = 0) const override {
        ident.dbg_print(indent);
        template_argument_list.dbg_print(0);
    }

    ast_node ident;
    ast_node template_argument_list;
};
struct template_id : public node {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    template_id(ast_node&& name, ast_node&& args)
        : name(std::move(name)), template_argument_list(std::move(args)) {}

    ast_node name;
    ast_node template_argument_list;
};
struct decltype_specifier : public node {
    AST_GET_TYPE;

    decltype_specifier()
        : expr(nullptr) {}
    decltype_specifier(ast_node&& expr)
        : expr(std::move(expr)) {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("decltype_specifier:\n", indent, DBG_RED | DBG_GREEN | DBG_BLUE | DBG_BG_BLUE);
        expr.dbg_print(indent + 1);
    }

    ast_node expr;
};

struct class_or_decltype : public node {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;

    std::unique_ptr<nested_name_specifier> nested_name_specifier;
    /*std::variant<
        ast_class_name,
        ast_simple_template_id,
        ast_decltype_specifier
    > type_name;*/
};

struct opaque_enum_decl : public node {
    AST_GET_TYPE;
    opaque_enum_decl(e_enum_key key, ast_node&& name, ast_node&& base)
        : key(key), name(std::move(name)), base(std::move(base)) {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent(enum_key_to_string(key), indent, DBG_KEYWORD);
        dbg_printf_color_indent(" ", 0, DBG_WHITE);
        name.dbg_print(0);
        if (base) {
            dbg_printf_color_indent(" : ", 0, DBG_WHITE);
            base.dbg_print(0);
        }
        dbg_printf_color_indent(";", 0, DBG_WHITE);
        dbg_printf_color_indent("/*opaque_enum_decl*/", 0, DBG_COMMENT);
    }

    e_enum_key key;
    ast_node name;
    ast_node base;
};
struct enum_head : public node {
    AST_GET_TYPE;
    enum_head(e_enum_key key, ast_node&& name, ast_node&& base)
        : key(key), name(std::move(name)), base(std::move(base)) {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent(enum_key_to_string(key), indent, DBG_KEYWORD);
        dbg_printf_color_indent(" ", 0, DBG_WHITE);
        if (name) {
            name.dbg_print(0);
        }
        if (base) {
            dbg_printf_color_indent(" : ", 0, DBG_WHITE);
            base.dbg_print(0);
        }
    }

    e_enum_key key;
    ast_node name;
    ast_node base;
};
struct enumerator_definition : public node {
    AST_GET_TYPE;
    enumerator_definition(ast_node&& ident, ast_node&& init)
        : ident(std::move(ident)), initializer(std::move(init)) {}

    void dbg_print(int indent = 0) const override {
        ident.dbg_print(indent);
        if (initializer) {
            dbg_printf_color_indent(" = ", 0, DBG_WHITE);
            initializer.dbg_print(0);
        }
    }

    ast_node ident;
    ast_node initializer;
};
struct enumerator_list : public node {
    AST_GET_TYPE;

    enumerator_list() {}

    void push_back(ast_node&& n) {
        list.push_back(std::move(n));
    }

    void dbg_print(int indent = 0) const override {
        for (int i = 0; i < list.size(); ++i) {
            list[i].dbg_print(indent);
            if (i < list.size() - 1) {
                dbg_printf_color_indent(",\n", 0, DBG_WHITE);
            }
        }
    }

    std::vector<ast_node> list;
};

struct enum_specifier : public node {
    AST_GET_TYPE;

    enum_specifier(ast_node&& head, ast_node&& enum_list)
        : head(std::move(head)), enum_list(std::move(enum_list)) {}

    void dbg_print(int indent = 0) const override {
        head.dbg_print(indent);
        dbg_printf_color_indent(" {\n", 0, DBG_WHITE);
        enum_list.dbg_print(indent + 1);
        dbg_printf_color_indent("\n", 0, DBG_WHITE);
        dbg_printf_color_indent("}", indent, DBG_WHITE);
        dbg_printf_color_indent("/*enum_specifier*/", 0, DBG_COMMENT);
    }

    ast_node head;
    ast_node enum_list;
};

struct base_specifier : public node {
    AST_GET_TYPE;

    base_specifier(ast_node&& base_type_spec, e_access_specifier access, bool is_virtual)
        : base_type_spec(std::move(base_type_spec)), access(access), is_virtual(is_virtual) {}

    void dbg_print(int indent = 0) const override {
        if(is_virtual) {
            dbg_printf_color_indent("virtual", 0, DBG_KEYWORD);
            dbg_printf_color_indent(" ", 0, DBG_WHITE);
        }
        if (access != e_access_unspecified) {
            dbg_printf_color_indent("%s", 0, DBG_KEYWORD, access_to_string(access));
            dbg_printf_color_indent(" ", 0, DBG_WHITE);
        }
        base_type_spec.dbg_print(0);
        if(is_pack_expansion) {
            dbg_printf_color_indent("...", 0, DBG_WHITE);
        }
    }

    ast_node base_type_spec;
    e_access_specifier access;
    bool is_virtual = false;
    bool is_pack_expansion = false;
};
struct base_specifier_list : public node {
    AST_GET_TYPE;

    void push_back(ast_node&& n) {
        list.push_back(std::move(n));
    }

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent(":", indent, DBG_WHITE);
        printf(" ");
        for (int i = 0; i < list.size(); ++i) {
            list[i].dbg_print(0);
            if (i < list.size() - 1) {
                dbg_printf_color_indent(", ", indent, DBG_WHITE);
            }
        }
    }

    std::vector<ast_node> list;
};

struct equal_initializer : public node {
    AST_GET_TYPE;
    equal_initializer(ast_node&& init_clause)
        : init_clause(std::move(init_clause)) {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("= ", indent, DBG_WHITE);
        init_clause.dbg_print(0);
    }

    ast_node init_clause;
};

struct bit_field : public node {
    AST_GET_TYPE;

    bit_field(ast_node&& declarator, ast_node&& const_expr)
        : declarator(std::move(declarator)), const_expr(std::move(const_expr)) {}

    void dbg_print(int indent = 0) const override {
        int ind = indent;
        if (declarator) {
            declarator.dbg_print(ind);
            dbg_printf_color_indent(" ", 0, DBG_WHITE);
            ind = 0;
        }
        dbg_printf_color_indent(": ", ind, DBG_WHITE);
        const_expr.dbg_print(0);
    }

    ast_node declarator;
    ast_node const_expr;
};
struct member_function_declarator : public node {
    AST_GET_TYPE;

    member_function_declarator(ast_node&& declarator, e_virt_flags virt_flags, bool is_pure)
        : declarator(std::move(declarator)), virt_flags(virt_flags), is_pure(is_pure) {}

    void dbg_print(int indent = 0) const override {
        declarator.dbg_print(indent);

        if(virt_flags & e_override) {
            dbg_printf_color_indent(" ", 0, DBG_WHITE);
            dbg_printf_color_indent("%s", 0, DBG_KEYWORD, virt_to_string(e_override));
        }
        if(virt_flags & e_final) {
            dbg_printf_color_indent(" ", 0, DBG_WHITE);
            dbg_printf_color_indent("%s", 0, DBG_KEYWORD, virt_to_string(e_final));
        }
        if (is_pure) {
            dbg_printf_color_indent(" ", 0, DBG_WHITE);
            dbg_printf_color_indent("= ", 0, DBG_WHITE);
            dbg_printf_color_indent("0", 0, DBG_LITERAL);
        }
    }

    ast_node declarator;
    e_virt_flags virt_flags;
    bool is_pure;
};
struct member_declarator : public node {
    AST_GET_TYPE;

    member_declarator(ast_node&& declarator, ast_node&& initializer)
        : declarator(std::move(declarator)), initializer(std::move(initializer)) {}

    void dbg_print(int indent = 0) const override {
        declarator.dbg_print(indent);
        if (initializer) {
            dbg_printf_color_indent(" ", 0, DBG_WHITE);
            initializer.dbg_print(0);
        }
    }

    ast_node declarator;
    ast_node initializer;
};

struct member_declarator_list : public node {
    AST_GET_TYPE;
    member_declarator_list() {}

    void push_back(ast_node n) {
        list.push_back(std::move(n));
    }
    void dbg_print(int indent = 0) const override {
        int ind = indent;
        for (int i = 0; i < list.size(); ++i) {
            list[i].dbg_print(ind);
            if(i < list.size() - 1) {
                dbg_printf_color_indent(", ", 0, DBG_WHITE);
            }
            ind = 0;
        }
    }

    std::vector<ast_node> list;
};

struct member_declaration : public node {
    AST_GET_TYPE;
    member_declaration(ast_node&& decl_spec, ast_node& declarator_list)
        : decl_spec(std::move(decl_spec)), declarator_list(std::move(declarator_list)) {}

    void dbg_print(int indent = 0) const override {
        int ind = indent;
        if (decl_spec) {
            decl_spec.dbg_print(ind);
            dbg_printf_color_indent(" ", 0, DBG_WHITE);
            ind = 0;
            dbg_printf_color_indent("/*decl-spec*/", 0, DBG_COMMENT);
        }
        if (declarator_list) {
            declarator_list.dbg_print(ind);
        }
        dbg_printf_color_indent(";", 0, DBG_WHITE);
        dbg_printf_color_indent("/*member-declaration*/", 0, DBG_COMMENT);
    }

    ast_node decl_spec;
    ast_node declarator_list;
};

struct mem_initializer : public node {
    AST_GET_TYPE;
    mem_initializer(ast_node&& id, ast_node&& initializer)
        : id(std::move(id)), initializer(std::move(initializer)) {}

    void dbg_print(int indent = 0) const override {
        id.dbg_print(indent);
        initializer.dbg_print(0);
    }

    ast_node id;
    ast_node initializer;
    bool is_pack_expansion = false;
};

struct mem_initializer_list : public node {
    AST_GET_TYPE;
    mem_initializer_list() {}

    void push_back(ast_node&& n) {
        list.push_back(std::move(n));
    }

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent(": ", indent, DBG_WHITE);
        for (int i = 0; i < list.size(); ++i) {
            list[i].dbg_print(0);
            if (i < int(list.size()) - 1) {
                dbg_printf_color_indent(", ", indent, DBG_WHITE);
            }
        }
    }

    std::vector<ast_node> list;
};

struct compound_statement : public node {
    AST_GET_TYPE;
    compound_statement() {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("{}", indent, DBG_WHITE);
    }
};

struct exception_declaration : public node {
    AST_GET_TYPE;
    exception_declaration(ast_node type_spec_seq, ast_node declarator_)
        : type_spec_seq(std::move(type_spec_seq)), declarator_(std::move(declarator_)) {}

    void dbg_print(int indent = 0) const override {
        type_spec_seq.dbg_print(indent);
        if (declarator_) {
            dbg_printf_color_indent(" ", 0, DBG_WHITE);
            declarator_.dbg_print(0);
        }
    }

    ast_node type_spec_seq;
    ast_node declarator_;
};

struct exception_handler : public node {
    AST_GET_TYPE;
    exception_handler(ast_node&& ex_decl, ast_node&& compound_stmt)
        : ex_decl(std::move(ex_decl)), compound_stmt(std::move(compound_stmt)) {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent(keyword_to_string(kw_catch), indent, DBG_KEYWORD);

        dbg_printf_color_indent(" (", 0, DBG_WHITE);
        ex_decl.dbg_print();
        dbg_printf_color_indent(") ", 0, DBG_WHITE);

        compound_stmt.dbg_print(0);
    }

    ast_node ex_decl;
    ast_node compound_stmt;
};

struct exception_handler_seq : public node {
    AST_GET_TYPE;
    exception_handler_seq() {}

    void push_back(ast_node&& n) {
        seq.push_back(std::move(n));
    }
    void dbg_print(int indent = 0) const override {
        int ind = indent;
        for (int i = 0; i < seq.size(); ++i) {
            if (i > 0) {
                dbg_printf_color_indent(" ", indent, DBG_WHITE);
            }
            seq[i].dbg_print(ind);
            ind = 0;
        }
    }

    std::vector<ast_node> seq;
};

struct pack_expansion : public node {
    AST_GET_TYPE;

    pack_expansion() {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("...", indent, DBG_WHITE);
    }
};

struct function_body : public node {
    AST_GET_TYPE;

    function_body(ast_node&& ctor_init, ast_node&& compound_stmt)
        : ctor_init(std::move(ctor_init)), compound_stmt(std::move(compound_stmt)) {}
    function_body(ast_node&& ctor_init, ast_node&& compound_stmt, ast_node&& ex_handlers)
        : ctor_init(std::move(ctor_init)), compound_stmt(std::move(compound_stmt)), ex_handlers(std::move(ex_handlers)) {}

    void dbg_print(int indent = 0) const override {
        int ind = indent;
        if (ctor_init) {
            dbg_printf_color_indent("\n", 0, DBG_WHITE);
            ctor_init.dbg_print(ind + 1);
        }
        if (compound_stmt) {
            if(ctor_init) {
                dbg_printf_color_indent("\n", 0, DBG_WHITE);
            }
            if (ex_handlers) {
                dbg_printf_color_indent(keyword_to_string(kw_try), 0, DBG_KEYWORD);
                dbg_printf_color_indent(" ", 0, DBG_WHITE);
            }
            compound_stmt.dbg_print(0);
        }
        if (ex_handlers) {
            dbg_printf_color_indent(" ", 0, DBG_WHITE);
            ex_handlers.dbg_print(0);
        }
    }

    ast_node ctor_init;
    ast_node compound_stmt;
    ast_node ex_handlers;
};
struct function_body_default : public node {
    AST_GET_TYPE;

    function_body_default() {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("= ", indent, DBG_WHITE);
        dbg_printf_color_indent("default", 0, DBG_KEYWORD);
        dbg_printf_color_indent(";", indent, DBG_WHITE);
    }
};
struct function_body_delete : public node {
    AST_GET_TYPE;

    function_body_delete() {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("= ", indent, DBG_WHITE);
        dbg_printf_color_indent("delete", 0, DBG_KEYWORD);
        dbg_printf_color_indent(";", indent, DBG_WHITE);
    }
};

struct function_definition : public node {
    AST_GET_TYPE;
    function_definition(ast_node&& decl_spec_seq, ast_node&& declarator_, ast_node&& body)
        : decl_spec_seq(std::move(decl_spec_seq)), declarator_(std::move(declarator_)), body(std::move(body)) {}

    void dbg_print(int indent = 0) const override {
        int ind = indent;
        if (decl_spec_seq) {
            decl_spec_seq.dbg_print(ind);
            ind = 0;
            dbg_printf_color_indent(" ", 0, DBG_WHITE);
        }
        declarator_.dbg_print(ind);
        dbg_printf_color_indent(" ", 0, DBG_WHITE);
        body.dbg_print(0);
    }

    ast_node decl_spec_seq;
    ast_node declarator_;
    ast_node body;
};

struct member_access_spec : public node {
    AST_GET_TYPE;
    member_access_spec(e_access_specifier access)
        : access(access) {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("%s", indent - 1, DBG_KEYWORD, access_to_string(access));
        dbg_printf_color_indent(":", 0, DBG_WHITE);
    }

    e_access_specifier access;
};

struct member_specification : public node {
    AST_GET_TYPE;

    void push_back(ast_node&& n) {
        list.push_back(std::move(n));
    }

    void dbg_print(int indent = 0) const override {
        for (int i = 0; i < list.size(); ++i) {
            list[i].dbg_print(indent);
            if(i < list.size() - 1) {
                dbg_printf_color_indent("\n", 0, DBG_WHITE);
            }
        }
    }

    std::vector<ast_node> list;
};
struct class_head : public node {
    AST_GET_TYPE;

    class_head(e_class_key key, ast_node&& class_head_name, ast_node&& base_clause)
        : key(key), class_head_name(std::move(class_head_name)), base_clause(std::move(base_clause)) {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("%s", indent, DBG_KEYWORD, class_key_to_string(key));
        printf(" ");
        class_head_name.dbg_print(0);
        printf(" ");
        if(base_clause) {
            base_clause.dbg_print(0);
            printf(" ");
        }
    }

    e_class_key key;
    ast_node class_head_name;
    ast_node base_clause;
};
struct class_specifier : public node {
    AST_GET_TYPE;

    class_specifier(ast_node&& class_head, ast_node&& member_spec_seq)
        : head(std::move(class_head)), member_spec_seq(std::move(member_spec_seq)) {}

    void dbg_print(int indent = 0) const override {
        head.dbg_print(indent);
        dbg_printf_color_indent("{", 0, DBG_WHITE);
        if(member_spec_seq) {
            dbg_printf_color_indent("\n", 0, DBG_WHITE);
            member_spec_seq.dbg_print(indent + 1);
            printf("\n");
            dbg_printf_color_indent("}", indent, DBG_WHITE);
        } else {
            dbg_printf_color_indent("}", 0, DBG_WHITE);
        }
        dbg_printf_color_indent("/*class_specifier*/", 0, DBG_COMMENT);
    }

    ast_node head;
    ast_node member_spec_seq;
};

struct declaration : public node {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;


};
struct declaration_seq : public node {
    AST_GET_TYPE;

    void push_back(ast_node&& n) {
        seq.push_back(std::move(n));
    }

    void dbg_print(int indent = 0) const override {
        for (int i = 0; i < seq.size(); ++i) {
            seq[i].dbg_print(indent);
            if (i < int(seq.size()) - 1) {
                dbg_printf_color_indent("\n", 0, DBG_WHITE);
            }
        }
    }

    std::vector<ast_node> seq;
};

struct init_declarator : public node {
    AST_GET_TYPE;

    init_declarator(ast_node&& declarator, ast_node&& initializer)
        : declarator(std::move(declarator)), initializer(std::move(initializer)) {}

    void dbg_print(int indent = 0) const override {
        declarator.dbg_print(indent);
        if (initializer) {
            dbg_printf_color_indent(" ", 0, DBG_WHITE);
            initializer.dbg_print(0);
        }
    }

    ast_node declarator;
    ast_node initializer;
};
struct init_declarator_list : public node {    
    AST_GET_TYPE;
    init_declarator_list() {}

    void push_back(ast_node n) {
        list.push_back(std::move(n));
    }
    void dbg_print(int indent = 0) const override {
        int ind = indent;
        for (int i = 0; i < list.size(); ++i) {
            list[i].dbg_print(ind);
            if(i < list.size() - 1) {
                dbg_printf_color_indent(", ", 0, DBG_WHITE);
            }
            ind = 0;
        }
    }

    std::vector<ast_node> list;
};

struct alias_declaration : public node {
    AST_GET_TYPE;
    alias_declaration(ast_node&& ident, ast_node&& type_id_)
        : ident(std::move(ident)), type_id_(std::move(type_id_)) {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent(keyword_to_string(kw_using), indent, DBG_KEYWORD);
        dbg_printf_color_indent(" ", 0, DBG_WHITE);
        ident.dbg_print(0);
        dbg_printf_color_indent(" = ", 0, DBG_WHITE);
        type_id_.dbg_print(0);
        dbg_printf_color_indent(";", 0, DBG_WHITE);
    }

    ast_node ident;
    ast_node type_id_;
};

struct simple_declaration : public node {
    AST_GET_TYPE;
    simple_declaration(ast_node&& decl_spec, ast_node&& declarator_list)
        : decl_spec(std::move(decl_spec)), declarator_list(std::move(declarator_list)) {}

    void dbg_print(int indent = 0) const override {
        int ind = indent;
        if (decl_spec) {
            decl_spec.dbg_print(ind);
            ind = 0;
        }
        if (declarator_list) {
            if(decl_spec) {
                dbg_printf_color_indent(" ", 0, DBG_WHITE);
            }
            declarator_list.dbg_print(ind);
        }
        dbg_printf_color_indent(";", 0, DBG_WHITE);
    }

    ast_node decl_spec;
    ast_node declarator_list;
};

struct static_assert_ : public node {
    AST_GET_TYPE;

    static_assert_(ast_node&& expr, ast_node&& strlit)
        : expr(std::move(expr)), strlit(std::move(strlit)) {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("static_assert", indent, DBG_KEYWORD);
        dbg_printf_color_indent("(", 0, DBG_WHITE);
        expr.dbg_print(0);
        if (strlit) {
            dbg_printf_color_indent(", ", 0, DBG_WHITE);
            strlit.dbg_print(0);
        }
        dbg_printf_color_indent(");", 0, DBG_WHITE);
    }
    ast_node expr;
    ast_node strlit;
};

struct namespace_definition : public node {
    AST_GET_TYPE;

    namespace_definition(ast_node&& ident, ast_node&& decl_seq, bool is_inline)
        : ident(std::move(ident)), decl_seq(std::move(decl_seq)), is_inline(is_inline) {}

    void dbg_print(int indent = 0) const override {
        int ind = indent;
        if (is_inline) {
            dbg_printf_color_indent(keyword_to_string(kw_inline), ind, DBG_KEYWORD);
            dbg_printf_color_indent(" ", 0, DBG_WHITE);
            ind = 0;
        }
        dbg_printf_color_indent("namespace", ind, DBG_KEYWORD);
        if (ident) {
            dbg_printf_color_indent(" ", 0, DBG_WHITE);
            ident.dbg_print(0);
        }
        dbg_printf_color_indent(" {", 0, DBG_WHITE);
        if (!decl_seq) {
            dbg_printf_color_indent("}", 0, DBG_WHITE);
            return;
        }
        dbg_printf_color_indent("\n", 0, DBG_WHITE);
        decl_seq.dbg_print(indent + 1);

        dbg_printf_color_indent("\n", 0, DBG_WHITE);
        dbg_printf_color_indent("}", indent, DBG_WHITE);
    }

    ast_node ident;
    ast_node decl_seq;
    bool is_inline = false;
};

struct type_parameter : public node {
    AST_GET_TYPE;
    type_parameter(ast_node&& ident, bool is_variadic)
        : ident(std::move(ident)), initializer(nullptr), is_variadic(is_variadic) {}
    type_parameter(ast_node&& ident, ast_node&& default_init, bool is_variadic)
        : ident(std::move(ident)), initializer(std::move(default_init)), is_variadic(is_variadic) {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent(keyword_to_string(kw_typename), indent, DBG_KEYWORD);
        if(is_variadic) {
            dbg_printf_color_indent("... ", 0, DBG_WHITE);
        } else {
            dbg_printf_color_indent(" ", 0, DBG_WHITE);
        }
        if(ident) ident.dbg_print(0);
        if (initializer) {
            dbg_printf_color_indent(" = ", 0, DBG_WHITE);
            initializer.dbg_print(0);
        }
    }

    ast_node ident;
    ast_node initializer;
    bool is_variadic = false;
};

struct template_parameter_list : public node {
    AST_GET_TYPE;
    template_parameter_list() {}

    void push_back(ast_node n) {
        list.push_back(std::move(n));
    }
    void dbg_print(int indent = 0) const override {
        int ind = indent;
        for (int i = 0; i < list.size(); ++i) {
            list[i].dbg_print(ind);
            if(i < list.size() - 1) {
                dbg_printf_color_indent(", ", 0, DBG_WHITE);
            }
            ind = 0;
        }
    }

    std::vector<ast_node> list;
};

struct template_declaration : public node {
    AST_GET_TYPE;
    template_declaration(ast_node&& param_list, ast_node&& decl)
        : param_list(std::move(param_list)), decl(std::move(decl)) {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent(keyword_to_string(kw_template), indent, DBG_KEYWORD);
        dbg_printf_color_indent("<", 0, DBG_WHITE);
        param_list.dbg_print(0);
        dbg_printf_color_indent(">\n", 0, DBG_WHITE);
        decl.dbg_print(indent);
    }

    ast_node param_list;
    ast_node decl;
};

struct translation_unit : public node {
    AST_GET_TYPE;

    void add_decl(ast_node&& n) {
        declarations.push_back(std::move(n));
    }

    std::vector<ast_node> declarations;

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("translation_unit:\n", indent, DBG_RED | DBG_GREEN | DBG_BLUE);
        for (int i = 0; i < declarations.size(); ++i) {
            declarations[i].dbg_print(indent + 1);
            printf("\n");
        }
    }
};


}

