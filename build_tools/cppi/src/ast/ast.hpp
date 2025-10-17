#pragma once

#include <assert.h>
#include <typeinfo>
#include <vector>
#include <memory>
#include <variant>
#include <deque>
#include "types.hpp"
#include "parse_exception.hpp"
#include "parsing/attribute.hpp"
#include "decl_spec_compat.hpp"
#include "expression/expression.hpp"
#include "expression/eval.hpp"
#include "template_argument.hpp"

class parse_state;

class symbol;
using symbol_ref = std::shared_ptr<symbol>;

class symbol_table;

class ast_node;

namespace ast {

    template<typename T>
    using uptr = std::unique_ptr<T>;

    struct node {
        virtual ~node() {}

        virtual const type_info& get_type() const = 0;

        template<typename T>
        bool is() const { return dynamic_cast<const T*>(this) != 0; }
        template<typename T>
        const T* as() const { return dynamic_cast<const T*>(this); }

        virtual node* clone() const { return nullptr; }

        virtual int precedence() const { return 0; }

        virtual bool is_dependent() const { return false; }
        virtual ast_node resolve_dependent(const template_argument* args, int arg_count) const;

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

    ast_node clone() const {
        if(!ptr) return ast_node::null();
        return ast_node(ptr->clone());
    }

    eval_result try_evaluate() const {
        const expression* expr = as<expression>();
        if (!expr) {
            return eval_result::not_a_constant_expr();
        }
        return expr->evaluate();
    }

    bool is_dependent() const {
        if(!ptr) return false;
        return ptr->is_dependent();
    }

    ast_node resolve_dependent(const template_argument* args, int count) const {
        if(!ptr) return ast_node::null();
        return ptr->resolve_dependent(args, count);
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

    inline ast_node node::resolve_dependent(const template_argument* args, int arg_count) const {
        return ast_node::null();
    }

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

#define AST_CLONE_AUTO(...) \
    node* clone() const override { \
        return new std::remove_cv<std::remove_pointer<decltype(this)>::type>::type(__VA_ARGS__); \
    } \


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
    AST_CLONE_AUTO();
};

struct attributes : public node {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_CLONE_AUTO(attribute_specifier(attribs));

    attributes(attribute_specifier&& attribs)
        : attribs(std::move(attribs)) {}

    attribute_specifier attribs;
};

struct literal : public node, public expression {
    AST_GET_TYPE;
    AST_PRECEDENCE(0);
    AST_CLONE_AUTO(tok);

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

    eval_result evaluate() const override {
        if (tok.type != tt_literal) {
            return eval_result::not_implemented();
        }

        switch (tok.lit_type) {
        case l_int32: return eval_value(e_int, (int64_t)tok.i32);
        case l_int64: return eval_value(e_long_int, (int64_t)tok.i64);
        case l_uint32: return eval_value(e_unsigned_int, (uint64_t)tok.ui32);
        case l_uint64: return eval_value(e_unsigned_long_int, (uint64_t)tok.ui64);
        case l_float: return eval_value(e_float, (long double)tok.f32);
        case l_double: return eval_value(e_double, (long double)tok.double_);
        case l_long_double: return eval_value(e_long_double, (long double)tok.long_double_);
        // TODO:
        default:
            return eval_result::not_implemented();
        }
    }

    // TODO: Interpret the token
    token tok;
};

struct user_defined_type_specifier : public node {
    AST_GET_TYPE;

    virtual symbol_ref resolve_to_symbol(parse_state& ps) const = 0;
};

struct nullptr_literal : public node, public expression {
    AST_GET_TYPE;
    AST_PRECEDENCE(0);
    AST_CLONE_AUTO(tok);

    // Token stored for location info that we could need
    // not used at the moment
    nullptr_literal(const token& tok)
        : tok(tok) {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("nullptr", indent, DBG_KEYWORD);
    }

    eval_result evaluate() const override {
        return eval_result::not_implemented();
    }

    token tok;
};

struct boolean_literal : public literal {
    AST_GET_TYPE;
    AST_PRECEDENCE(0);
    AST_CLONE_AUTO(tok);

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
    
    eval_result evaluate() const override {
        return eval_value(e_bool, (uint64_t)(value ? 1 : 0));
    }

    bool value;
};

struct identifier : public node, public expression {
    AST_GET_TYPE;
    AST_PRECEDENCE(0);
    AST_CLONE_AUTO(tok, sym);

    identifier(token tok)
        : tok(tok) {}
    identifier(token tok, symbol_ref sym)
        : tok(tok), sym(sym) {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("%s", indent, DBG_RED | DBG_GREEN | DBG_INTENSITY, tok.str.c_str());
    }

    token tok;
    symbol_ref sym;
};

struct this_ : public node, public expression {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_PRECEDENCE(0);
    AST_CLONE_AUTO();
};
struct size_of : public node, public expression {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_PRECEDENCE(3);
    AST_CLONE_AUTO(arg.clone(), is_vararg_count);

    size_of(ast_node&& arg, bool is_vararg_count = false)
        : arg(std::move(arg)), is_vararg_count(is_vararg_count) {}

    bool is_dependent() const override {
        return arg.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        // TODO: resolve to a resolved_const_expr
        return ast_node::make<ast::dummy>();
    }

    ast_node arg;
    bool is_vararg_count;
};
struct align_of : public node, public expression {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_PRECEDENCE(3);
    AST_CLONE_AUTO(arg.clone());

    align_of(ast_node&& arg)
        : arg(std::move(arg)) {}

    bool is_dependent() const override {
        return arg.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_arg = 
            arg.is_dependent()
            ? arg.resolve_dependent(args, arg_count)
            : arg.clone();
        return ast_node::make<ast::align_of>(std::move(resolved_arg));
    }

    ast_node arg;
};

struct pseudo_destructor_name : public node {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_CLONE_AUTO(type.clone());

    pseudo_destructor_name(ast_node&& type)
        : type(std::move(type)) {}


    ast_node type;
};

struct expr_noexcept : public node, public expression {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_PRECEDENCE(0);
    AST_CLONE_AUTO(expr.clone());

    expr_noexcept(ast_node&& expr)
        : expr(std::move(expr)) {}

    bool is_dependent() const override {
        return expr.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_expr = 
            expr.is_dependent()
            ? expr.resolve_dependent(args, arg_count)
            : expr.clone();
        return ast_node::make<ast::expr_noexcept>(std::move(resolved_expr));
    }

    ast_node expr;
};

struct expr_delete : public node, public expression {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_PRECEDENCE(3);
    AST_CLONE_AUTO(expr.clone(), is_array, is_scoped);

    expr_delete(ast_node&& expr, bool is_array, bool is_scoped)
        : expr(std::move(expr)), is_array(is_array), is_scoped(is_scoped) {}

    bool is_dependent() const override {
        return expr.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_expr =
            expr.is_dependent()
            ? expr.resolve_dependent(args, arg_count)
            : expr.clone();
        return ast_node::make<ast::expr_delete>(std::move(resolved_expr), is_array, is_scoped);
    }

    ast_node expr;
    bool is_array;
    bool is_scoped;
};

struct expr_primary : public node, public expression {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_PRECEDENCE(0);
    AST_CLONE_AUTO();

    bool is_dependent() const override {
        // TODO: ?
        return false;
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        // TODO: ?
        return ast_node::make<ast::expr_primary>();
    }

    // TODO:
};
struct expr_array_subscript : public node, public expression {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_PRECEDENCE(2);
    AST_CLONE_AUTO(operand.clone(), arg.clone());

    expr_array_subscript(ast_node&& operand, ast_node&& arg)
        : operand(std::move(operand)), arg(std::move(arg)) {}

    bool is_dependent() const override {
        return operand.is_dependent() || arg.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_operand =
            operand.is_dependent()
            ? operand.resolve_dependent(args, arg_count)
            : operand.clone();
        ast_node resolved_arg =
            arg.is_dependent()
            ? arg.resolve_dependent(args, arg_count)
            : arg.clone();
        return ast_node::make<ast::expr_array_subscript>(std::move(resolved_operand), std::move(resolved_arg));
    }

    ast_node operand;
    ast_node arg;
};
struct expr_fn_call : public node, public expression {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_PRECEDENCE(2);
    AST_CLONE_AUTO(operand.clone(), arg.clone());

    expr_fn_call(ast_node&& operand, ast_node&& arg)
        : operand(std::move(operand)), arg(std::move(arg)) {}
    
    bool is_dependent() const override {
        return operand.is_dependent() || arg.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_operand =
            operand.is_dependent()
            ? operand.resolve_dependent(args, arg_count)
            : operand.clone();
        ast_node resolved_arg =
            arg.is_dependent()
            ? arg.resolve_dependent(args, arg_count)
            : arg.clone();
        return ast_node::make<ast::expr_fn_call>(std::move(resolved_operand), std::move(resolved_arg));
    }

    ast_node operand;
    ast_node arg;
};
struct expr_member_access : public node, public expression {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_PRECEDENCE(2);
    AST_CLONE_AUTO(operand.clone(), arg.clone());

    expr_member_access(ast_node&& operand, ast_node&& arg)
        : operand(std::move(operand)), arg(std::move(arg)) {}
    
    bool is_dependent() const override {
        return operand.is_dependent() || arg.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_operand =
            operand.is_dependent()
            ? operand.resolve_dependent(args, arg_count)
            : operand.clone();
        ast_node resolved_arg =
            arg.is_dependent()
            ? arg.resolve_dependent(args, arg_count)
            : arg.clone();
        return ast_node::make<ast::expr_member_access>(std::move(resolved_operand), std::move(resolved_arg));
    }

    ast_node operand;
    ast_node arg;
};
struct expr_ptr_member_access : public node, public expression {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_PRECEDENCE(2);
    AST_CLONE_AUTO(operand.clone(), arg.clone());

    expr_ptr_member_access(ast_node&& operand, ast_node&& arg)
        : operand(std::move(operand)), arg(std::move(arg)) {}
        
    bool is_dependent() const override {
        return operand.is_dependent() || arg.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_operand =
            operand.is_dependent()
            ? operand.resolve_dependent(args, arg_count)
            : operand.clone();
        ast_node resolved_arg =
            arg.is_dependent()
            ? arg.resolve_dependent(args, arg_count)
            : arg.clone();
        return ast_node::make<ast::expr_ptr_member_access>(std::move(resolved_operand), std::move(resolved_arg));
    }

    ast_node operand;
    ast_node arg;
};
struct expr_postfix : public node, public expression {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_PRECEDENCE(2);
    AST_CLONE_AUTO(operand.clone(), arg.clone());

    expr_postfix(ast_node&& operand, ast_node&& arg)
        : operand(std::move(operand)), arg(std::move(arg)) {}
    
    bool is_dependent() const override {
        return operand.is_dependent() || arg.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_operand =
            operand.is_dependent()
            ? operand.resolve_dependent(args, arg_count)
            : operand.clone();
        ast_node resolved_arg =
            arg.is_dependent()
            ? arg.resolve_dependent(args, arg_count)
            : arg.clone();
        return ast_node::make<ast::expr_postfix>(std::move(resolved_operand), std::move(resolved_arg));
    }

    ast_node operand;
    ast_node arg;
};
struct expr_unary : public node, public expression {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_PRECEDENCE(3);
    AST_CLONE_AUTO(op, expr.clone());

    expr_unary(e_unary_op op, ast_node&& expr)
        : op(op), expr(std::move(expr)) {}
    
    bool is_dependent() const override {
        return expr.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_expr =
            expr.is_dependent()
            ? expr.resolve_dependent(args, arg_count)
            : expr.clone();
        return ast_node::make<ast::expr_unary>(op, std::move(resolved_expr));
    }

    e_unary_op op;
    ast_node expr;
};

struct expr_deref : public expr_unary {
    AST_GET_TYPE;
    AST_CLONE_AUTO(expr.clone());

    expr_deref(ast_node&& expr)
        : expr_unary(e_unary_deref, std::move(expr)) {}
    
    eval_result evaluate() const override {
        eval_result e = expr.try_evaluate();
        if(!e) return e;

        // TODO: ?

        return eval_result::not_implemented();
    }
    
    bool is_dependent() const override {
        return expr.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_expr =
            expr.is_dependent()
            ? expr.resolve_dependent(args, arg_count)
            : expr.clone();
        return ast_node::make<ast::expr_deref>(std::move(resolved_expr));
    }

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("%s", indent, DBG_OPERATOR, unary_op_to_string(op));
        dbg_print_operand(0, this, expr);
    }
};

struct expr_amp : public expr_unary {
    AST_GET_TYPE;
    AST_CLONE_AUTO(expr.clone());

    expr_amp(ast_node&& expr)
        : expr_unary(e_unary_amp, std::move(expr)) {}
    
    eval_result evaluate() const override {
        eval_result e = expr.try_evaluate();
        if(!e) return e;

        // TODO: ?

        return eval_result::not_implemented();
    }
    
    bool is_dependent() const override {
        return expr.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_expr =
            expr.is_dependent()
            ? expr.resolve_dependent(args, arg_count)
            : expr.clone();
        return ast_node::make<ast::expr_amp>(std::move(resolved_expr));
    }

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("%s", indent, DBG_OPERATOR, unary_op_to_string(op));
        dbg_print_operand(0, this, expr);
    }
};

struct expr_plus : public expr_unary {
    AST_GET_TYPE;
    AST_CLONE_AUTO(expr.clone());

    expr_plus(ast_node&& expr)
        : expr_unary(e_unary_plus, std::move(expr)) {}
    
    eval_result evaluate() const override {
        eval_result e = expr.try_evaluate();
        if(!e) return e;

        eval_value r = e.value;
        if (!eval_promote_arithmetic(r)) {
            // TODO: exception?
            return eval_result::error();
        }
        return r;
    }
    
    bool is_dependent() const override {
        return expr.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_expr =
            expr.is_dependent()
            ? expr.resolve_dependent(args, arg_count)
            : expr.clone();
        return ast_node::make<ast::expr_plus>(std::move(resolved_expr));
    }

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("%s", indent, DBG_OPERATOR, unary_op_to_string(op));
        dbg_print_operand(0, this, expr);
    }
};

struct expr_minus : public expr_unary {
    AST_GET_TYPE;
    AST_CLONE_AUTO(expr.clone());

    expr_minus(ast_node&& expr)
        : expr_unary(e_unary_minus, std::move(expr)) {}
    
    eval_result evaluate() const override {
        eval_result e = expr.try_evaluate();
        if(!e) return e;

        eval_value r = e.value;
        if (!eval_promote_arithmetic(r)) {
            // TODO: exception?
            return eval_result::error();
        }

        return eval_negate(r);
    }
    
    bool is_dependent() const override {
        return expr.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_expr =
            expr.is_dependent()
            ? expr.resolve_dependent(args, arg_count)
            : expr.clone();
        return ast_node::make<ast::expr_minus>(std::move(resolved_expr));
    }

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("%s", indent, DBG_OPERATOR, unary_op_to_string(op));
        dbg_print_operand(0, this, expr);
    }
};

struct expr_lnot : public expr_unary {
    AST_GET_TYPE;
    AST_CLONE_AUTO(expr.clone());

    expr_lnot(ast_node&& expr)
        : expr_unary(e_unary_lnot, std::move(expr)) {}
    
    eval_result evaluate() const override {
        eval_result e = expr.try_evaluate();
        if(!e) return e;

        return eval_logical_not(e.value);
    }
    
    bool is_dependent() const override {
        return expr.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_expr =
            expr.is_dependent()
            ? expr.resolve_dependent(args, arg_count)
            : expr.clone();
        return ast_node::make<ast::expr_lnot>(std::move(resolved_expr));
    }

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("%s", indent, DBG_OPERATOR, unary_op_to_string(op));
        dbg_print_operand(0, this, expr);
    }
};

struct expr_not : public expr_unary {
    AST_GET_TYPE;
    AST_CLONE_AUTO(expr.clone());

    expr_not(ast_node&& expr)
        : expr_unary(e_unary_bnot, std::move(expr)) {}
    
    eval_result evaluate() const override {
        eval_result e = expr.try_evaluate();
        if(!e) return e;

        return eval_not(e.value);
    }
    
    bool is_dependent() const override {
        return expr.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_expr =
            expr.is_dependent()
            ? expr.resolve_dependent(args, arg_count)
            : expr.clone();
        return ast_node::make<ast::expr_not>(std::move(resolved_expr));
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
    AST_CLONE_AUTO(arg.clone());

    expr_typeid(ast_node&& arg)
        : arg(std::move(arg)) {}
    
    bool is_dependent() const override {
        return arg.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_arg =
            arg.is_dependent()
            ? arg.resolve_dependent(args, arg_count)
            : arg.clone();
        return ast_node::make<ast::expr_typeid>(std::move(resolved_arg));
    }

    ast_node arg;
};
struct expr_cast : public node, public expression {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_PRECEDENCE(3);
    AST_CLONE_AUTO(type_id_.clone(), expr_.clone(), kind);

    expr_cast(ast_node&& type_id_, ast_node&& expr_, e_cast kind = e_cast_cstyle)
        : type_id_(std::move(type_id_)), expr_(std::move(expr_)), kind(kind) {}
    
    bool is_dependent() const override {
        return type_id_.is_dependent() || expr_.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_type_id =
            type_id_.is_dependent()
            ? type_id_.resolve_dependent(args, arg_count)
            : type_id_.clone();
        ast_node resolved_expr =
            expr_.is_dependent()
            ? expr_.resolve_dependent(args, arg_count)
            : expr_.clone();
        return ast_node::make<ast::expr_cast>(std::move(resolved_type_id), std::move(resolved_expr), kind);
    }

    ast_node type_id_;
    ast_node expr_;
    e_cast kind;
};
struct expr_pm : public node, public expression {
    AST_GET_TYPE;
    AST_BIN_EXPR_DBG_PRINT_AUTO(expr_pm);
    AST_PRECEDENCE(4);
    AST_CLONE_AUTO(left.clone(), right.clone());

    expr_pm(ast_node&& left, ast_node&& right)
        : left(std::move(left)), right(std::move(right)) {}
    
    bool is_dependent() const override {
        return left.is_dependent() || right.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_left =
            left.is_dependent()
            ? left.resolve_dependent(args, arg_count)
            : left.clone();
        ast_node resolved_right =
            right.is_dependent()
            ? right.resolve_dependent(args, arg_count)
            : right.clone();
        return ast_node::make<ast::expr_pm>(std::move(resolved_left), std::move(resolved_right));
    }

    ast_node left;
    ast_node right;
};
struct expr_mul : public node, public expression {
    AST_GET_TYPE;
    //AST_BIN_EXPR_DBG_PRINT_AUTO(expr_mul);
    AST_PRECEDENCE(5);
    AST_CLONE_AUTO(op, left.clone(), right.clone());

    expr_mul(e_operator op, ast_node&& left, ast_node&& right)
        : op(op), left(std::move(left)), right(std::move(right)) {}

    eval_result evaluate() const override {
        eval_result l = left.try_evaluate();
        eval_result r = right.try_evaluate();
        if(!l) return l;
        if(!r) return r;

        eval_convert_operands(l.value, r.value);

        switch (op) {
        case e_mul: return eval_multiply(l.value, r.value);
        case e_div: return eval_divide(l.value, r.value);
        case e_mod: return eval_modulo(l.value, r.value);
        default:
            assert(false);
            return eval_result::error();
        }
        return eval_result::error();
    }
    
    bool is_dependent() const override {
        return left.is_dependent() || right.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_left =
            left.is_dependent()
            ? left.resolve_dependent(args, arg_count)
            : left.clone();
        ast_node resolved_right =
            right.is_dependent()
            ? right.resolve_dependent(args, arg_count)
            : right.clone();
        return ast_node::make<ast::expr_mul>(op, std::move(resolved_left), std::move(resolved_right));
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
    AST_CLONE_AUTO(op, left.clone(), right.clone());

    expr_add(e_operator op, ast_node&& left, ast_node&& right)
        : op(op), left(std::move(left)), right(std::move(right)) {}

    eval_result evaluate() const override {
        eval_result l = left.try_evaluate();
        eval_result r = right.try_evaluate();
        if(!l) return l;
        if(!r) return r;

        eval_convert_operands(l.value, r.value);

        switch (op) {
        case e_add: return eval_add(l.value, r.value);
        case e_sub: return eval_sub(l.value, r.value);
        default:
            assert(false);
            return eval_result::error();
        }
        return eval_result::error();
    }
    
    bool is_dependent() const override {
        return left.is_dependent() || right.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_left =
            left.is_dependent()
            ? left.resolve_dependent(args, arg_count)
            : left.clone();
        ast_node resolved_right =
            right.is_dependent()
            ? right.resolve_dependent(args, arg_count)
            : right.clone();
        return ast_node::make<ast::expr_add>(op, std::move(resolved_left), std::move(resolved_right));
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
    AST_CLONE_AUTO(op, left.clone(), right.clone());

    expr_shift(e_operator op, ast_node&& left, ast_node&& right)
        : op(op), left(std::move(left)), right(std::move(right)) {}
    
    eval_result evaluate() const override {
        eval_result l = left.try_evaluate();
        eval_result r = right.try_evaluate();
        if(!l) return l;
        if(!r) return r;

        eval_convert_operands(l.value, r.value);

        switch (op) {
        case e_lshift: return eval_lshift(l.value, r.value);
        case e_rshift: return eval_rshift(l.value, r.value);
        default:
            assert(false);
            return eval_result::error();
        }
        return eval_result::error();
    }
    
    bool is_dependent() const override {
        return left.is_dependent() || right.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_left =
            left.is_dependent()
            ? left.resolve_dependent(args, arg_count)
            : left.clone();
        ast_node resolved_right =
            right.is_dependent()
            ? right.resolve_dependent(args, arg_count)
            : right.clone();
        return ast_node::make<ast::expr_shift>(op, std::move(resolved_left), std::move(resolved_right));
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
    AST_CLONE_AUTO(op, left.clone(), right.clone());

    expr_rel(e_operator op, ast_node&& left, ast_node&& right)
        : op(op), left(std::move(left)), right(std::move(right)) {}
    
    eval_result evaluate() const override {
        eval_result l = left.try_evaluate();
        eval_result r = right.try_evaluate();
        if(!l) return l;
        if(!r) return r;

        eval_convert_operands(l.value, r.value);

        switch (op) {
        case e_less: return eval_less(l.value, r.value);
        case e_more: return eval_greater(l.value, r.value);
        case e_lesseq: return eval_less_equal(l.value, r.value);
        case e_moreeq: return eval_greater_equal(l.value, r.value);
        default:
            assert(false);
            return eval_result::error();
        }
        return eval_result::error();
    }
    
    bool is_dependent() const override {
        return left.is_dependent() || right.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_left =
            left.is_dependent()
            ? left.resolve_dependent(args, arg_count)
            : left.clone();
        ast_node resolved_right =
            right.is_dependent()
            ? right.resolve_dependent(args, arg_count)
            : right.clone();
        return ast_node::make<ast::expr_rel>(op, std::move(resolved_left), std::move(resolved_right));
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
    AST_CLONE_AUTO(op, left.clone(), right.clone());

    expr_eq(e_operator op, ast_node&& left, ast_node&& right)
        : op(op), left(std::move(left)), right(std::move(right)) {}
    
    eval_result evaluate() const override {
        eval_result l = left.try_evaluate();
        eval_result r = right.try_evaluate();
        if(!l) return l;
        if(!r) return r;

        eval_convert_operands(l.value, r.value);

        switch (op) {
        case e_equal: return eval_equal(l.value, r.value);
        case e_notequal: return eval_not_equal(l.value, r.value);
        default:
            assert(false);
            return eval_result::error();
        }
        return eval_result::error();
    }
    
    bool is_dependent() const override {
        return left.is_dependent() || right.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_left =
            left.is_dependent()
            ? left.resolve_dependent(args, arg_count)
            : left.clone();
        ast_node resolved_right =
            right.is_dependent()
            ? right.resolve_dependent(args, arg_count)
            : right.clone();
        return ast_node::make<ast::expr_eq>(op, std::move(resolved_left), std::move(resolved_right));
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
    AST_CLONE_AUTO(left.clone(), right.clone());

    expr_and(ast_node&& left, ast_node&& right)
        : left(std::move(left)), right(std::move(right)) {}
    
    eval_result evaluate() const override {
        eval_result l = left.try_evaluate();
        eval_result r = right.try_evaluate();
        if(!l) return l;
        if(!r) return r;

        eval_convert_operands(l.value, r.value);

        return eval_and(l.value, r.value);
    }
    
    bool is_dependent() const override {
        return left.is_dependent() || right.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_left =
            left.is_dependent()
            ? left.resolve_dependent(args, arg_count)
            : left.clone();
        ast_node resolved_right =
            right.is_dependent()
            ? right.resolve_dependent(args, arg_count)
            : right.clone();
        return ast_node::make<ast::expr_and>(std::move(resolved_left), std::move(resolved_right));
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
    AST_CLONE_AUTO(left.clone(), right.clone());

    expr_exor(ast_node&& left, ast_node&& right)
        : left(std::move(left)), right(std::move(right)) {}
    
    eval_result evaluate() const override {
        eval_result l = left.try_evaluate();
        eval_result r = right.try_evaluate();
        if(!l) return l;
        if(!r) return r;

        eval_convert_operands(l.value, r.value);

        return eval_xor(l.value, r.value);
    }
    
    bool is_dependent() const override {
        return left.is_dependent() || right.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_left =
            left.is_dependent()
            ? left.resolve_dependent(args, arg_count)
            : left.clone();
        ast_node resolved_right =
            right.is_dependent()
            ? right.resolve_dependent(args, arg_count)
            : right.clone();
        return ast_node::make<ast::expr_exor>(std::move(resolved_left), std::move(resolved_right));
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
    AST_CLONE_AUTO(left.clone(), right.clone());

    expr_ior(ast_node&& left, ast_node&& right)
        : left(std::move(left)), right(std::move(right)) {}
    
    eval_result evaluate() const override {
        eval_result l = left.try_evaluate();
        eval_result r = right.try_evaluate();
        if(!l) return l;
        if(!r) return r;

        eval_convert_operands(l.value, r.value);

        return eval_inclusive_or(l.value, r.value);
    }
    
    bool is_dependent() const override {
        return left.is_dependent() || right.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_left =
            left.is_dependent()
            ? left.resolve_dependent(args, arg_count)
            : left.clone();
        ast_node resolved_right =
            right.is_dependent()
            ? right.resolve_dependent(args, arg_count)
            : right.clone();
        return ast_node::make<ast::expr_ior>(std::move(resolved_left), std::move(resolved_right));
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
    AST_CLONE_AUTO(left.clone(), right.clone());

    expr_land(ast_node&& left, ast_node&& right)
        : left(std::move(left)), right(std::move(right)) {}
    
    eval_result evaluate() const override {
        eval_result l = left.try_evaluate();
        eval_result r = right.try_evaluate();
        if(!l) return l;
        if(!r) return r;

        eval_convert_operands(l.value, r.value);

        return eval_logical_and(l.value, r.value);
    }
    
    bool is_dependent() const override {
        return left.is_dependent() || right.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_left =
            left.is_dependent()
            ? left.resolve_dependent(args, arg_count)
            : left.clone();
        ast_node resolved_right =
            right.is_dependent()
            ? right.resolve_dependent(args, arg_count)
            : right.clone();
        return ast_node::make<ast::expr_land>(std::move(resolved_left), std::move(resolved_right));
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
    AST_CLONE_AUTO(left.clone(), right.clone());

    expr_lor(ast_node&& left, ast_node&& right)
        : left(std::move(left)), right(std::move(right)) {}
    
    eval_result evaluate() const override {
        eval_result l = left.try_evaluate();
        eval_result r = right.try_evaluate();
        if(!l) return l;
        if(!r) return r;

        eval_convert_operands(l.value, r.value);

        return eval_logical_or(l.value, r.value);
    }
    
    bool is_dependent() const override {
        return left.is_dependent() || right.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_left =
            left.is_dependent()
            ? left.resolve_dependent(args, arg_count)
            : left.clone();
        ast_node resolved_right =
            right.is_dependent()
            ? right.resolve_dependent(args, arg_count)
            : right.clone();
        return ast_node::make<ast::expr_lor>(std::move(resolved_left), std::move(resolved_right));
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
    AST_CLONE_AUTO(cond.clone(), a.clone(), b.clone());
    
    expr_cond(ast_node&& cond, ast_node&& a, ast_node&& b)
        : cond(std::move(cond)), a(std::move(a)), b(std::move(b)) {}

    bool is_dependent() const override {
        return cond.is_dependent() || a.is_dependent() || b.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_cond =
            cond.is_dependent()
            ? cond.resolve_dependent(args, arg_count)
            : cond.clone();
        ast_node resolved_a =
            a.is_dependent()
            ? a.resolve_dependent(args, arg_count)
            : a.clone();
        ast_node resolved_b =
            b.is_dependent()
            ? b.resolve_dependent(args, arg_count)
            : b.clone();
        return ast_node::make<ast::expr_cond>(std::move(resolved_cond), std::move(resolved_a), std::move(resolved_b));
    }

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("expr_cond:\n", indent, DBG_RED | DBG_GREEN | DBG_BLUE | DBG_BG_BLUE);
        cond.dbg_print(indent + 1);
        printf("\n");
        a.dbg_print(indent + 1);
        printf("\n");
        b.dbg_print(indent + 1);
    }

    ast_node cond;
    ast_node a;
    ast_node b;
};
struct expr_assign : public node, public expression {
    AST_GET_TYPE;
    AST_BIN_EXPR_DBG_PRINT_AUTO(expr_assign);
    AST_CLONE_AUTO(left.clone(), right.clone());

    expr_assign(ast_node&& left, ast_node&& right)
        : left(std::move(left)), right(std::move(right)) {}
    
    bool is_dependent() const override {
        return left.is_dependent() || right.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_left =
            left.is_dependent()
            ? left.resolve_dependent(args, arg_count)
            : left.clone();
        ast_node resolved_right =
            right.is_dependent()
            ? right.resolve_dependent(args, arg_count)
            : right.clone();
        return ast_node::make<ast::expr_assign>(std::move(resolved_left), std::move(resolved_right));
    }

    ast_node left;
    ast_node right;
};

struct init_list : public node {
    AST_GET_TYPE;

    node* clone() const override {
        auto p = new init_list();
        for (auto& n : list) {
            p->add(n.clone());
        }
        return p;
    }

    init_list() {}

    bool is_dependent() const override {
        for (auto& n : list) {
            if(n.is_dependent()) return true;
        }
        return false;
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_list = ast_node::make<ast::init_list>();
        for (auto& n : list) {
            ast_node resolved_n =
                n.is_dependent()
                ? n.resolve_dependent(args, arg_count)
                : n.clone();
            resolved_list.as<ast::init_list>()->add(std::move(resolved_n));
        }
        return resolved_list;
    }

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
    AST_CLONE_AUTO(list.clone());

    braced_init_list(ast_node&& list)
        : list(std::move(list)) {}
    
    bool is_dependent() const override {
        return list.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_list =
            list.is_dependent()
            ? list.resolve_dependent(args, arg_count)
            : list.clone();
        return ast_node::make<ast::braced_init_list>(std::move(resolved_list));
    }

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("braced_init_list:\n", indent, DBG_RED | DBG_GREEN | DBG_BLUE | DBG_BG_BLUE);
        list.dbg_print(indent + 1);
    }

    ast_node list;
};

struct simple_type_specifier : public node {
    AST_GET_TYPE;
    AST_CLONE_AUTO(kw);

    simple_type_specifier(e_keyword kw)
        : kw(kw) {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("%s", indent, DBG_KEYWORD, keyword_to_string(kw));
    }

    e_keyword kw;
};

struct cv_qualifier : public node {
    AST_GET_TYPE;
    AST_CLONE_AUTO(cv);

    cv_qualifier(e_cv cv)
        : cv(cv) {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("%s", indent, DBG_KEYWORD, cv_to_string(cv));
    }

    e_cv cv;
};

struct cv_qualifier_seq : public node {
    AST_GET_TYPE;

    node* clone() const override {
        auto p = new cv_qualifier_seq();
        for (auto& n : seq) {
            p->push_back(n.clone());
        }
        return p;
    }

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

struct elaborated_type_specifier : public user_defined_type_specifier {
    AST_GET_TYPE;

    node* clone() const override {
        if (type == e_elaborated_class) {
            return new elaborated_type_specifier(class_key, name.clone(), sym);
        } else if (type == e_elaborated_enum) {
            return new elaborated_type_specifier(enum_key, name.clone(), sym);
        } else {
            assert(false);
            return nullptr;
        }
    }

    elaborated_type_specifier(e_class_key key, ast_node&& name, symbol_ref sym)
        : type(e_elaborated_class), class_key(key), name(std::move(name)), sym(sym) {}
    elaborated_type_specifier(e_enum_key key, ast_node&& name, symbol_ref sym)
        : type(e_elaborated_enum), enum_key(key), name(std::move(name)), sym(sym) {}

    symbol_ref resolve_to_symbol(parse_state& ps) const override {
        return sym;
    }

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
    symbol_ref sym;
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

struct parameters_and_qualifiers;
struct ptr_operator : public node {
    AST_GET_TYPE;
    AST_CLONE_AUTO(type, is_const, is_volatile);

    ptr_operator(e_ptr_operator type, bool is_const = false, bool is_volatile = false)
        : type(type), is_const(is_const), is_volatile(is_volatile) {}

    void dbg_print(int indent = 0) const override {
        switch (type) {
        case e_ptr:
            dbg_printf_color_indent("*", indent, DBG_OPERATOR);
            if(is_const) dbg_printf_color_indent((std::string(" ") + cv_to_string(cv_const)).c_str(), 0, DBG_KEYWORD);
            if(is_volatile) dbg_printf_color_indent((std::string(" ") + cv_to_string(cv_volatile)).c_str(), 0, DBG_KEYWORD);
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
    bool is_const;
    bool is_volatile;
};

struct type_specifier_seq : public node {
    AST_GET_TYPE;

    node* clone() const override {
        auto p = new type_specifier_seq();
        for (auto& n : seq) {
            p->push_back(n.clone());
        }
        p->flags = flags;
        return p;
    }

    type_specifier_seq() {}

    void push_back(ast_node&& n) {
        seq.push_back(std::move(n));
    }

    bool is_dependent() const override {
        for (auto& n : seq) {
            if(n.is_dependent()) return true;
        }
        return false;
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_seq = ast_node::make<ast::type_specifier_seq>();
        for (auto& n : seq) {
            ast_node resolved_n = 
                n.is_dependent()
                ? n.resolve_dependent(args, arg_count)
                : n.clone();
            resolved_seq.as<ast::type_specifier_seq>()->push_back(std::move(resolved_n));
        }
        resolved_seq.as<ast::type_specifier_seq>()->flags = flags;
        return resolved_seq;
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
    decl_flags flags;
};

struct array_declarator;
struct parameters_and_qualifiers;
struct identifier;
struct ptr_declarator;
struct declarator : public declarator_part {
    AST_GET_TYPE;
    AST_CLONE_AUTO(decl.clone(), next.clone());

    declarator()
        : decl(ast_node::null()) {}
    declarator(ast_node&& decl)
        : decl(std::move(decl)), declarator_part(ast_node::null()) {}
    declarator(ast_node&& decl, ast_node&& n)
        : decl(std::move(decl)), declarator_part(std::move(n)) {}

    bool is_abstract() const {
        return decl.is_null();
    }

    bool is_dependent() const {
        // TODO: unsure if the name itself really can be dependent
        return next.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        // TODO: unsure if the name itself really can be dependent
        ast_node resolved_next = 
            next.is_dependent()
            ? next.resolve_dependent(args, arg_count)
            : next.clone();
        return ast_node::make<ast::declarator>(decl.clone(), std::move(resolved_next));
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
            } else if (p->is<array_declarator>() || p->is<parameters_and_qualifiers>()) {
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
    AST_CLONE_AUTO(ptr_op.clone(), next.clone());

    ptr_declarator()
        : declarator_part(ast_node::null()) {}
    ptr_declarator(ast_node&& ptr_op, ast_node&& n)
        : declarator_part(std::move(n)), ptr_op(std::move(ptr_op)) {}

    bool is_dependent() const {
        return next.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_next =
            next.is_dependent()
            ? next.resolve_dependent(args, arg_count)
            : next.clone();
        return ast_node::make<ast::ptr_declarator>(ptr_op.clone(), std::move(resolved_next));
    }

    void dbg_print(int indent = 0) const override {
        ptr_op.dbg_print(indent);
    }
    void dbg_print_declarator_part(int indent = 0) const override {
        ptr_op.dbg_print(indent);
    }

    ast_node ptr_op;
};
// noptr-declarator
struct array_declarator : public declarator_part {
    AST_GET_TYPE;
    AST_CLONE_AUTO(const_expr.clone(), next.clone());

    array_declarator()
        : declarator_part(ast_node::null()) {}
    array_declarator(ast_node&& const_expr)
        : declarator_part(ast_node::null()), const_expr(std::move(const_expr)) {}
    array_declarator(ast_node&& const_expr, ast_node&& next)
        : declarator_part(std::move(next)), const_expr(std::move(const_expr)) {}
    
    bool is_dependent() const {
        return const_expr.is_dependent() || next.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_expr =
            const_expr.is_dependent()
            ? const_expr.resolve_dependent(args, arg_count)
            : const_expr.clone();
        ast_node resolved_next =
            next.is_dependent()
            ? next.resolve_dependent(args, arg_count)
            : next.clone();
        return ast_node::make<ast::array_declarator>(std::move(resolved_expr), std::move(resolved_next));
    }

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("[]", indent, DBG_RED | DBG_GREEN | DBG_BLUE | DBG_BG_BLUE);
    }
    void dbg_print_declarator_part(int indent = 0) const override {
        dbg_printf_color_indent("[]", indent, DBG_OPERATOR);
    }

    ast_node const_expr;
};

struct operator_function_id : public node {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_CLONE_AUTO();
};
struct conversion_declarator : public node {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_CLONE_AUTO();
};
struct conversion_type_id : public node {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_CLONE_AUTO();
};
struct conversion_function_id : public node {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_CLONE_AUTO();
};

struct storage_class_specifier : public node {
    AST_GET_TYPE;
    AST_CLONE_AUTO(kw);

    storage_class_specifier(e_keyword kw)
        : kw(kw) {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("%s", indent, DBG_KEYWORD, keyword_to_string(kw));
    }

    e_keyword kw;
};
struct function_specifier : public node {
    AST_GET_TYPE;
    AST_CLONE_AUTO(kw);

    function_specifier(e_keyword kw)
        : kw(kw) {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("%s", indent, DBG_KEYWORD, keyword_to_string(kw));
    }

    e_keyword kw;
};
struct decl_specifier : public node {
    AST_GET_TYPE;
    AST_CLONE_AUTO(kw);

    decl_specifier(e_keyword kw)
        : kw(kw) {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("%s", indent, DBG_KEYWORD, keyword_to_string(kw));
    }

    e_keyword kw;
};
struct decl_specifier_seq : public node {
    AST_GET_TYPE;

    node* clone() const override {
        auto p = new decl_specifier_seq();
        for (auto& n : specifiers) {
            p->push_back(n.clone());
        }
        p->flags = flags;
        return p;
    }

    void push_back(ast_node&& n) {
        specifiers.push_back(std::move(n));
    }

    bool is_dependent() const override {
        for (auto& n : specifiers) {
            if(n.is_dependent()) return true;
        }
        return false;
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_seq = ast_node::make<ast::decl_specifier_seq>();
        for (auto& n : specifiers) {
            ast_node resolved_n = 
                n.is_dependent()
                ? n.resolve_dependent(args, arg_count)
                : n.clone();
            resolved_seq.as<ast::decl_specifier_seq>()->push_back(std::move(resolved_n));
        }
        resolved_seq.as<ast::decl_specifier_seq>()->flags = flags;
        return resolved_seq;
    }

    size_t count() const {
        return specifiers.size();
    }

    template<typename T>
    bool contains() const {
        for (int i = 0; i < specifiers.size(); ++i) {
            if(specifiers[i].is<T>()) return true;
        }
        return false;
    }

    template<typename T>
    T* get_specifier() {
        for (int i = 0; i < specifiers.size(); ++i) {
            T* spec = specifiers[i].as<T>();
            if(spec) return spec;
        }
        return nullptr;
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
    AST_CLONE_AUTO(decl_spec.clone(), declarator.clone(), initializer.clone());

    parameter_declaration(ast_node&& decl_spec, ast_node&& declarator)
        : decl_spec(std::move(decl_spec)), declarator(std::move(declarator)), initializer(ast_node::null()) {}
    parameter_declaration(ast_node&& decl_spec, ast_node&& declarator, ast_node&& initializer)
        : decl_spec(std::move(decl_spec)), declarator(std::move(declarator)), initializer(std::move(initializer)) {}

    bool is_dependent() const {
        return decl_spec.is_dependent()
            || declarator.is_dependent()
            || initializer.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_decl_spec =
            decl_spec.is_dependent()
            ? decl_spec.resolve_dependent(args, arg_count)
            : decl_spec.clone();
        ast_node resolved_declarator =
            declarator.is_dependent()
            ? declarator.resolve_dependent(args, arg_count)
            : declarator.clone();
        ast_node resolved_initializer =
            initializer.is_dependent()
            ? initializer.resolve_dependent(args, arg_count)
            : initializer.clone();
        return ast_node::make<ast::parameter_declaration>(std::move(resolved_decl_spec), std::move(resolved_declarator), std::move(resolved_initializer));
    }

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

    node* clone() const override {
        auto p = new parameter_declaration_list();
        for (auto& n : params) {
            p->push_back(n.clone());
        }
        p->is_variadic = is_variadic;
        return p;
    }

    bool is_dependent() const {
        for (auto& n : params) {
            if(n.is_dependent()) return true;
        }
        return false;
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_list = ast_node::make<ast::parameter_declaration_list>();
        for (const auto& n : params) {
            ast_node resolved_n = 
                n.is_dependent()
                ? n.resolve_dependent(args, arg_count)
                : n.clone();
            resolved_list.as<ast::parameter_declaration_list>()->push_back(std::move(resolved_n));
        }
        resolved_list.as<ast::parameter_declaration_list>()->is_variadic = is_variadic;
        return resolved_list;
    }

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
    AST_CLONE_AUTO(type);
    
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
// TODO: trailing-return-type!
struct parameters_and_qualifiers : public declarator_part {
    AST_GET_TYPE;
    AST_CLONE_AUTO(param_decl_list.clone(), cv.clone(), ref.clone(), ex_spec.clone(), is_const, is_volatile, next.clone());

    parameters_and_qualifiers(
        ast_node&& param_decl_list,
        ast_node&& cv,
        ast_node&& ref,
        ast_node&& ex_spec,
        bool is_const,
        bool is_volatile,
        ast_node&& next = ast_node::null()
    )
        : param_decl_list(std::move(param_decl_list))
        , cv(std::move(cv))
        , ref(std::move(ref))
        , ex_spec(std::move(ex_spec))
        , is_const(is_const)
        , is_volatile(is_volatile)
        , declarator_part(std::move(next))
    {}

    bool is_dependent() const {
        return param_decl_list.is_dependent() || next.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_params = 
            param_decl_list.is_dependent()
            ? param_decl_list.resolve_dependent(args, arg_count)
            : param_decl_list.clone();
        ast_node resolved_next =
            next.is_dependent()
            ? next.resolve_dependent(args, arg_count)
            : next.clone();
        return ast_node::make<ast::parameters_and_qualifiers>(std::move(resolved_params), cv.clone(), ref.clone(), ex_spec.clone(), is_const, is_volatile, std::move(resolved_next));
    }

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
    bool is_const;
    bool is_volatile;
};

struct type_id : public node {
    AST_GET_TYPE;
    AST_CLONE_AUTO(type_spec_seq.clone(), declarator_.clone());

    type_id(ast_node&& type_spec_seq, ast_node&& declarator_)
        : type_spec_seq(std::move(type_spec_seq)), declarator_(std::move(declarator_)) {}

    bool is_dependent() const override {
        if(type_spec_seq.is_dependent()) return true;
        if(declarator_.is_dependent()) return true;
        return false;
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_type_spec_seq = 
            type_spec_seq.is_dependent()
            ? type_spec_seq.resolve_dependent(args, arg_count)
            : type_spec_seq.clone();
        ast_node resolved_declarator =
            declarator_.is_dependent()
            ? declarator_.resolve_dependent(args, arg_count)
            : declarator_.clone();
        return ast_node::make<ast::type_id>(std::move(resolved_type_spec_seq), std::move(resolved_declarator));
    }

    void dbg_print(int indent = 0) const override {
        type_spec_seq.dbg_print(0);
        if(declarator_) {
            printf(" ");
            declarator_.dbg_print(0);
        }
        //dbg_printf_color("/*type-id*/", DBG_COMMENT);
    }

    ast_node type_spec_seq;
    ast_node declarator_;
};

struct type_id_list : public node {
    AST_GET_TYPE;

    node* clone() const override {
        auto p = new type_id_list();
        for (auto& n : list) {
            p->push_back(n.clone());
        }
        return p;
    }

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


// Does not represent real syntax.
// Holds a resolved type_id substituted
// when instantiating templates.
// This is used instead of unwrapping
// type_id back into syntax, which
// wouldn't really work anyway with declarator parts,
// and would lead to weird syntax errors
struct resolved_type_id : public node {
    AST_GET_TYPE;
    
    resolved_type_id(const type_id_2* tid)
        : tid(tid) {}

    node* clone() const override {
        return new resolved_type_id(tid);
    }

    void dbg_print(int indent = 0) const override {
        // Losing colors here, would need special type_id_2::dbg_print() for that
        dbg_printf_color_indent("%s", indent, DBG_LITERAL, tid->to_source_string().c_str());
    }

    const type_id_2* tid;
};

struct resolved_const_expr : public node, public expression {
    AST_GET_TYPE;

    resolved_const_expr(const eval_value& value)
        : value(value) {}

    node* clone() const override {
        return new resolved_const_expr(value);
    }

    eval_result evaluate() const override {
        // TODO: might want to hold eval_result instead
        return value;
    }

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("%s", indent, DBG_LITERAL, value.normalized());
    }

    eval_value value;
};


struct dynamic_exception_spec : public node {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_CLONE_AUTO(type_id_list_.clone());

    dynamic_exception_spec(ast_node&& type_id_list_)
        : type_id_list_(std::move(type_id_list_)) {}

    ast_node type_id_list_;
};
struct noexcept_spec : public node {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_CLONE_AUTO(const_expr_.clone());

    noexcept_spec()
        : const_expr_(nullptr) {}
    noexcept_spec(ast_node&& const_expr_)
        : const_expr_(std::move(const_expr_)) {}

    ast_node const_expr_;
};

struct class_name : public user_defined_type_specifier {
    AST_GET_TYPE;
    AST_CLONE_AUTO(sym);

    class_name(symbol_ref sym)
        : sym(sym) {}

    symbol_ref resolve_to_symbol(parse_state& ps) const override {
        return sym;
    }

    bool is_dependent() const override;

    void dbg_print(int indent = 0) const override;

    symbol_ref sym;
};

struct enum_name : public user_defined_type_specifier {
    AST_GET_TYPE;
    AST_CLONE_AUTO(sym);

    enum_name(symbol_ref sym)
        : sym(sym) {}

    symbol_ref resolve_to_symbol(parse_state& ps) const override {
        return sym;
    }

    void dbg_print(int indent = 0) const override;

    symbol_ref sym;
};

struct enumerator_name : public node, public expression {
    AST_GET_TYPE;
    AST_CLONE_AUTO(sym);

    enumerator_name(symbol_ref sym)
        : sym(sym) {}

    eval_result evaluate() const override;

    void dbg_print(int indent = 0) const override;

    symbol_ref sym;
};

struct typedef_name : public user_defined_type_specifier {
    AST_GET_TYPE;
    AST_CLONE_AUTO(sym);

    typedef_name(symbol_ref sym)
        : sym(sym) {}

    symbol_ref resolve_to_symbol(parse_state& ps) const override {
        return sym;
    }

    void dbg_print(int indent = 0) const override;

    symbol_ref sym;
};

struct function_name : public node {
    AST_GET_TYPE;
    AST_CLONE_AUTO(sym);

    function_name(symbol_ref sym)
        : sym(sym) {}

    void dbg_print(int indent = 0) const override;

    symbol_ref sym;
};

struct object_name : public node {
    AST_GET_TYPE;
    AST_CLONE_AUTO(sym);

    object_name(symbol_ref sym)
        : sym(sym) {}

    void dbg_print(int indent = 0) const override;

    symbol_ref sym;
};

struct dependent_type_name : public node {
    AST_GET_TYPE;
    AST_CLONE_AUTO(sym);

    dependent_type_name(symbol_ref sym)
        : sym(sym) {}

    bool is_dependent() const override {
        return true;
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override;

    void dbg_print(int indent = 0) const override;

    symbol_ref sym;
};

struct dependent_non_type_name : public node {
    AST_GET_TYPE;
    AST_CLONE_AUTO(sym);

    dependent_non_type_name(symbol_ref sym)
        : sym(sym) {}

    bool is_dependent() const override {
        return true;
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override;

    void dbg_print(int indent = 0) const override;

    symbol_ref sym;
};

struct namespace_name : public node {
    AST_GET_TYPE;
    AST_CLONE_AUTO(sym);

    namespace_name(symbol_ref sym)
        : sym(sym) {}

    void dbg_print(int indent = 0) const override;

    symbol_ref sym;
};

/*
struct template_name : public user_defined_type_specifier {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;

    template_name(symbol_ref sym)
        : sym(sym) {}

    symbol_ref resolve_to_symbol(parse_state& ps) const override {
        return sym;
    }

    symbol_ref sym;
};*/

struct template_argument_list : public node {
    AST_GET_TYPE;

    node* clone() const override {
        auto p = new template_argument_list();
        for (auto& n : list) {
            p->push_back(n.clone());
        }
        return p;
    }

    template_argument_list() {}

    void push_back(ast_node&& n) {
        list.push_back(std::move(n));
    }

    bool is_dependent() const override {
        for (auto& n : list) {
            if(n.is_dependent()) return true;
        }
        return false;
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_list = ast_node::make<ast::template_argument_list>();
        for (auto& n : list) {
            ast_node resolved_arg = n.is_dependent()
                ? n.resolve_dependent(args, arg_count)
                : n.clone();
            resolved_list.as<ast::template_argument_list>()->push_back(std::move(resolved_arg));
        }
        return resolved_list;
    }

    void dbg_print(int indent = 0) const override {
        int ind = indent;
        for (int i = 0; i < list.size(); ++i) {
            list[i].dbg_print(ind);
            if (i < list.size() - 1) {
                dbg_printf_color_indent(", ", 0, DBG_WHITE);
            }
            ind = 0;
        }
    }

    std::vector<ast_node> list;
};
struct simple_template_id : public user_defined_type_specifier {
    AST_GET_TYPE;
    AST_CLONE_AUTO(ident.clone(), template_argument_list.clone());

    simple_template_id(ast_node&& ident, ast_node&& args)
        : ident(std::move(ident)), template_argument_list(std::move(args)) {}

    symbol_ref resolve_to_symbol(parse_state& ps) const override;

    bool is_dependent() const override {
        if(ident.is_dependent()) return true;
        if(template_argument_list.is_dependent()) return true;
        return false;
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_ident = ident.is_dependent()
            ? ident.resolve_dependent(args, arg_count)
            : ident.clone();
        ast_node resolved_args = template_argument_list.is_dependent()
            ? template_argument_list.resolve_dependent(args, arg_count)
            : template_argument_list.clone();
        return ast_node::make<ast::simple_template_id>(std::move(resolved_ident), std::move(resolved_args));
    }

    void dbg_print(int indent = 0) const override {
        ident.dbg_print(indent);

        dbg_printf_color_indent("<", 0, DBG_WHITE);
        if(template_argument_list) {
            template_argument_list.dbg_print(0);
        }
        dbg_printf_color_indent(">", 0, DBG_WHITE);
    }

    ast_node ident;
    ast_node template_argument_list;
    mutable symbol_ref sym_instance;
};
struct template_id : public node {
    AST_GET_TYPE;
    AST_DBG_PRINT_AUTO;
    AST_CLONE_AUTO(name.clone(), template_argument_list.clone());

    template_id(ast_node&& name, ast_node&& args)
        : name(std::move(name)), template_argument_list(std::move(args)) {}

    ast_node name;
    ast_node template_argument_list;
};


struct nested_name_specifier : public node {
    AST_GET_TYPE;
    AST_CLONE_AUTO(n.clone(), next.clone());

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

    std::shared_ptr<symbol_table> get_qualified_scope(parse_state& ps) const;
    const ast_node& get_qualified_name() const;

    bool is_dependent() const {
        if (n.is_dependent()) {
            return true;
        }
        if (next) {
            return next.is_dependent();
        }
        return false;
    }

    void dbg_print(int indent = 0) const override {
        if(n) {
            n.dbg_print(indent);
        }
        dbg_printf_color_indent("::", indent, DBG_WHITE);
        next.dbg_print(0);
    }

    std::shared_ptr<symbol_table> scope;
    ast_node n;
    ast_node next;
};


struct decltype_specifier : public node {
    AST_GET_TYPE;
    AST_CLONE_AUTO(expr.clone());

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

struct opaque_enum_decl : public node {
    AST_GET_TYPE;
    AST_CLONE_AUTO(key, name.clone(), base.clone());

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
    AST_CLONE_AUTO(key, name.clone(), base.clone());

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
    AST_CLONE_AUTO(ident.clone(), initializer.clone());

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

    node* clone() const override {
        auto p = new enumerator_list();
        for (auto& n : list) {
            p->push_back(n.clone());
        }
        return p;
    }

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

struct enum_specifier : public user_defined_type_specifier {
    AST_GET_TYPE;
    AST_CLONE_AUTO(head.clone(), enum_list.clone(), sym);

    enum_specifier(ast_node&& head, ast_node&& enum_list, symbol_ref sym)
        : head(std::move(head)), enum_list(std::move(enum_list)), sym(sym) {}

    symbol_ref resolve_to_symbol(parse_state& ps) const override {
        return sym;
    }

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
    symbol_ref sym;
};

struct base_specifier : public node {
    AST_GET_TYPE;
    AST_CLONE_AUTO(base_type_spec.clone(), access, is_virtual, is_pack_expansion);

    base_specifier(ast_node&& base_type_spec, e_access_specifier access, bool is_virtual)
        : base_type_spec(std::move(base_type_spec)), access(access), is_virtual(is_virtual) {}
    base_specifier(ast_node&& base_type_spec, e_access_specifier access, bool is_virtual, bool is_pack_expansion)
        : base_type_spec(std::move(base_type_spec)), access(access), is_virtual(is_virtual), is_pack_expansion(is_pack_expansion) {}

    bool is_dependent() const override {
        return base_type_spec.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_base_type_spec = base_type_spec.resolve_dependent(args, arg_count);
        // Any pack expansion can't result in a "pack expandable" node
        return ast_node::make<ast::base_specifier>(std::move(resolved_base_type_spec), access, is_virtual, false);
    }

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

    node* clone() const override {
        auto p = new base_specifier_list();
        for (auto& n : list) {
            p->push_back(n.clone());
        }
        return p;
    }

    void push_back(ast_node&& n) {
        list.push_back(std::move(n));
    }

    bool is_dependent() const override {
        for (auto& n : list) {
            if (n.is_dependent()) {
                return true;
            }
        }
        return false;
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        // eh
        int pack_arg_idx = -1;
        for (int i = 0; i < arg_count; ++i) {
            if (args[i].is_pack()) {
                pack_arg_idx = i;
                break;
            }
        }

        ast_node resolved_list = ast_node::make<ast::base_specifier_list>();
        for (auto& n : list) {
            if (!n.is_dependent()) {
                resolved_list.as<ast::base_specifier_list>()->push_back(n.clone());
                continue;
            }

            if (n.as<ast::base_specifier>()->is_pack_expansion) {
                //throw std::exception("TODO: base specifier pack expansion is not yet supported");
                std::vector<template_argument> args_copy(args, args + arg_count);
                if (pack_arg_idx < 0) {
                    throw std::exception("template arguments do not contain an argument pack, but a base_specifier is a pack expansion");
                }
                const auto& pack = args[pack_arg_idx].get_contained_pack();
                for (int i = 0; i < pack.size(); ++i) {
                    args_copy[pack_arg_idx] = pack[i];
                    ast_node resolved_base = n.resolve_dependent(args_copy.data(), args_copy.size());
                    resolved_base.as<ast::base_specifier>()->is_pack_expansion = false;
                    resolved_list.as<ast::base_specifier_list>()->push_back(std::move(resolved_base));
                }
            } else {
                ast_node resolved_base = n.resolve_dependent(args, arg_count);
                resolved_list.as<ast::base_specifier_list>()->push_back(std::move(resolved_base));
            }
        }
        return resolved_list;
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
    AST_CLONE_AUTO(init_clause.clone());

    equal_initializer(ast_node&& init_clause)
        : init_clause(std::move(init_clause)) {}
    
    bool is_dependent() const override {
        return init_clause.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_init_clause =
            init_clause.is_dependent()
            ? init_clause.resolve_dependent(args, arg_count)
            : init_clause.clone();
        return ast_node::make<ast::equal_initializer>(std::move(resolved_init_clause));
    }

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("= ", indent, DBG_WHITE);
        init_clause.dbg_print(0);
    }

    ast_node init_clause;
};

struct bit_field : public node {
    AST_GET_TYPE;
    AST_CLONE_AUTO(declarator.clone(), const_expr.clone());

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
    AST_CLONE_AUTO(declarator.clone(), virt_flags, is_pure);

    member_function_declarator(ast_node&& declarator, e_virt_flags virt_flags, bool is_pure)
        : declarator(std::move(declarator)), virt_flags(virt_flags), is_pure(is_pure) {}

    bool is_dependent() const override {
        return declarator.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_declarator =
            declarator.is_dependent()
            ? declarator.resolve_dependent(args, arg_count)
            : declarator.clone();
        return ast_node::make<ast::member_function_declarator>(std::move(resolved_declarator), virt_flags, is_pure);
    }

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
    AST_CLONE_AUTO(declarator.clone(), initializer.clone());

    member_declarator(ast_node&& declarator, ast_node&& initializer)
        : declarator(std::move(declarator)), initializer(std::move(initializer)) {}

    bool is_dependent() const override {
        return declarator.is_dependent() || initializer.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_declarator = 
            declarator.is_dependent()
            ? declarator.resolve_dependent(args, arg_count)
            : declarator.clone();
        ast_node resolved_initializer =
            initializer.is_dependent()
            ? initializer.resolve_dependent(args, arg_count)
            : initializer.clone();
        return ast_node::make<ast::member_declarator>(std::move(resolved_declarator), std::move(resolved_initializer));
    }
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

    node* clone() const override {
        auto p = new member_declarator_list();
        for (auto& n : list) {
            p->push_back(n.clone());
        }
        return p;
    }

    member_declarator_list() {}

    bool is_dependent() const override {
        for (auto& n : list) {
            if(n.is_dependent()) return true;
        }
        return false;
    }

    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_list = ast_node::make<ast::member_declarator_list>();
        for (auto& n : list) {
            ast_node resolved_n = 
                n.is_dependent()
                ? n.resolve_dependent(args, arg_count)
                : n.clone();
            resolved_list.as<ast::member_declarator_list>()->push_back(std::move(resolved_n));
        }
        return resolved_list;
    }

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
    AST_CLONE_AUTO(decl_spec.clone(), declarator_list.clone());

    member_declaration(ast_node&& decl_spec, ast_node&& declarator_list)
        : decl_spec(std::move(decl_spec)), declarator_list(std::move(declarator_list)) {}
    
    bool is_dependent() const override { 
        if(decl_spec.is_dependent()) return true;
        if(declarator_list.is_dependent()) return true;
        return false;
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_decl_spec = 
            decl_spec.is_dependent()
            ? decl_spec.resolve_dependent(args, arg_count)
            : decl_spec.clone();
        ast_node resolved_declarator_list =
            declarator_list.is_dependent()
            ? declarator_list.resolve_dependent(args, arg_count)
            : declarator_list.clone();
        return ast_node::make<ast::member_declaration>(std::move(resolved_decl_spec), std::move(resolved_declarator_list));
    }

    void dbg_print(int indent = 0) const override {
        int ind = indent;
        if (decl_spec) {
            decl_spec.dbg_print(ind);
            dbg_printf_color_indent(" ", 0, DBG_WHITE);
            ind = 0;
            //dbg_printf_color_indent("/*decl-spec*/", 0, DBG_COMMENT);
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
    AST_CLONE_AUTO(id.clone(), initializer.clone(), is_pack_expansion);

    mem_initializer(ast_node&& id, ast_node&& initializer)
        : id(std::move(id)), initializer(std::move(initializer)) {}
    mem_initializer(ast_node&& id, ast_node&& initializer, bool is_pack_expansion)
        : id(std::move(id)), initializer(std::move(initializer)), is_pack_expansion(is_pack_expansion) {}
    
    bool is_dependent() const override {
        return id.is_dependent() || initializer.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_id =
            id.is_dependent()
            ? id.resolve_dependent(args, arg_count)
            : id.clone();
        ast_node resolved_initializer =
            initializer.is_dependent()
            ? initializer.resolve_dependent(args, arg_count)
            : initializer.clone();

        // TODO: deal with pack expansion later
        return ast_node::make<ast::mem_initializer>(std::move(resolved_id), std::move(resolved_initializer), is_pack_expansion);
    }

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

    node* clone() const override {
        auto p = new mem_initializer_list();
        for (auto& n : list) {
            p->push_back(n.clone());
        }
        return p;
    }

    mem_initializer_list() {}

    void push_back(ast_node&& n) {
        list.push_back(std::move(n));
    }
    
    bool is_dependent() const override { 
        for (auto& n : list) {
            if(n.is_dependent()) return true;
        }
        return false; 
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_mem_init_list = ast_node::make<ast::mem_initializer_list>();
        for (auto& n : list) {
            ast_node resolved_n = 
                n.is_dependent()
                ? n.resolve_dependent(args, arg_count)
                : n.clone();
            resolved_mem_init_list.as<ast::mem_initializer_list>()->push_back(std::move(resolved_n));
        }
        return resolved_mem_init_list;
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
    AST_CLONE_AUTO();

    compound_statement() {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("{}", indent, DBG_WHITE);
    }
};

struct exception_declaration : public node {
    AST_GET_TYPE;
    AST_CLONE_AUTO(type_spec_seq.clone(), declarator_.clone());

    exception_declaration(ast_node type_spec_seq, ast_node declarator_)
        : type_spec_seq(std::move(type_spec_seq)), declarator_(std::move(declarator_)) {}
    
    bool is_dependent() const override {
        return type_spec_seq.is_dependent() || declarator_.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_type_spec_seq =
            type_spec_seq.is_dependent()
            ? type_spec_seq.resolve_dependent(args, arg_count)
            : type_spec_seq.clone();
        ast_node resolved_declarator_ =
            declarator_.is_dependent()
            ? declarator_.resolve_dependent(args, arg_count)
            : declarator_.clone();
        return ast_node::make<ast::exception_declaration>(std::move(resolved_type_spec_seq), std::move(resolved_declarator_));
    }

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
    AST_CLONE_AUTO(ex_decl.clone(), compound_stmt.clone());

    exception_handler(ast_node&& ex_decl, ast_node&& compound_stmt)
        : ex_decl(std::move(ex_decl)), compound_stmt(std::move(compound_stmt)) {}
    
    bool is_dependent() const override {
        return ex_decl.is_dependent() || compound_stmt.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_ex_decl =
            ex_decl.is_dependent()
            ? ex_decl.resolve_dependent(args, arg_count)
            : ex_decl.clone();
        ast_node resolved_compound_stmt =
            compound_stmt.is_dependent()
            ? compound_stmt.resolve_dependent(args, arg_count)
            : compound_stmt.clone();
        return ast_node::make<ast::exception_handler>(std::move(resolved_ex_decl), std::move(resolved_compound_stmt));
    }

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

    node* clone() const override {
        auto p = new exception_handler_seq();
        for (auto& n : seq) {
            p->push_back(n.clone());
        }
        return p;
    }

    exception_handler_seq() {}

    void push_back(ast_node&& n) {
        seq.push_back(std::move(n));
    }
    
    bool is_dependent() const override {
        for (auto& n : seq) {
            if(n.is_dependent()) return true;
        }
        return false;
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_exception_handler_seq = ast_node::make<ast::exception_handler_seq>();
        for (const auto& n : seq) {
            ast_node resolved_n = 
                n.is_dependent()
                ? n.resolve_dependent(args, arg_count)
                : n.clone();
            resolved_exception_handler_seq.as<ast::exception_handler_seq>()->push_back(std::move(resolved_n));
        }
        return resolved_exception_handler_seq;
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
    AST_CLONE_AUTO();

    pack_expansion() {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("...", indent, DBG_WHITE);
    }
};

struct function_body : public node {
    AST_GET_TYPE;
    AST_CLONE_AUTO(ctor_init.clone(), compound_stmt.clone(), ex_handlers.clone());

    function_body(ast_node&& ctor_init, ast_node&& compound_stmt)
        : ctor_init(std::move(ctor_init)), compound_stmt(std::move(compound_stmt)) {}
    function_body(ast_node&& ctor_init, ast_node&& compound_stmt, ast_node&& ex_handlers)
        : ctor_init(std::move(ctor_init)), compound_stmt(std::move(compound_stmt)), ex_handlers(std::move(ex_handlers)) {}
    
    bool is_dependent() const override {
        return ctor_init.is_dependent() || compound_stmt.is_dependent() || ex_handlers.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_ctor_init =
            ctor_init.is_dependent()
            ? ctor_init.resolve_dependent(args, arg_count)
            : ctor_init.clone();
        ast_node resolved_compound_stmt =
            compound_stmt.is_dependent()
            ? compound_stmt.resolve_dependent(args, arg_count)
            : compound_stmt.clone();
        ast_node resolved_ex_handlers = 
            ex_handlers.is_dependent()
            ? ex_handlers.resolve_dependent(args, arg_count)
            : ex_handlers.clone();
        return ast_node::make<ast::function_body>(std::move(resolved_ctor_init), std::move(resolved_compound_stmt), std::move(resolved_ex_handlers));
    }

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
    AST_CLONE_AUTO();

    function_body_default() {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("= ", indent, DBG_WHITE);
        dbg_printf_color_indent("default", 0, DBG_KEYWORD);
        dbg_printf_color_indent(";", indent, DBG_WHITE);
    }
};
struct function_body_delete : public node {
    AST_GET_TYPE;
    AST_CLONE_AUTO();

    function_body_delete() {}

    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("= ", indent, DBG_WHITE);
        dbg_printf_color_indent("delete", 0, DBG_KEYWORD);
        dbg_printf_color_indent(";", indent, DBG_WHITE);
    }
};

struct function_definition : public node {
    AST_GET_TYPE;
    AST_CLONE_AUTO(decl_spec_seq.clone(), declarator_.clone(), body.clone());

    function_definition(ast_node&& decl_spec_seq, ast_node&& declarator_, ast_node&& body)
        : decl_spec_seq(std::move(decl_spec_seq)), declarator_(std::move(declarator_)), body(std::move(body)) {}

    bool is_dependent() const override {
        return decl_spec_seq.is_dependent() || declarator_.is_dependent() || body.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_decl_spec_seq =
            decl_spec_seq.is_dependent()
            ? decl_spec_seq.resolve_dependent(args, arg_count)
            : decl_spec_seq.clone();
        ast_node resolved_declarator =
            declarator_.is_dependent()
            ? declarator_.resolve_dependent(args, arg_count)
            : declarator_.clone();
        ast_node resolved_body = 
            body.is_dependent()
            ? body.resolve_dependent(args, arg_count)
            : body.clone();
        return ast_node::make<ast::function_definition>(std::move(resolved_decl_spec_seq), std::move(resolved_declarator), std::move(resolved_body));
    }

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
    AST_CLONE_AUTO(access);

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

    node* clone() const override {
        auto p = new member_specification();
        for (auto& n : list) {
            p->push_back(n.clone());
        }
        return p;
    }

    bool is_dependent() const override { 
        for (auto& n : list) {
            if(n.is_dependent()) return true;
        }
        return false; 
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_mem_spec = ast_node::make<ast::member_specification>();
        for (auto& n : list) {
            ast_node resolved_n = 
                n.is_dependent()
                ? n.resolve_dependent(args, arg_count)
                : n.clone();
            resolved_mem_spec.as<ast::member_specification>()->push_back(std::move(resolved_n));
        }
        return resolved_mem_spec;
    }

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
    AST_CLONE_AUTO(key, class_head_name.clone(), base_clause.clone());

    class_head(e_class_key key, ast_node&& class_head_name, ast_node&& base_clause)
        : key(key), class_head_name(std::move(class_head_name)), base_clause(std::move(base_clause)) {}
    
    bool is_dependent() const override {
        return base_clause.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_base_clause =
            base_clause.is_dependent()
            ? base_clause.resolve_dependent(args, arg_count)
            : base_clause.clone();
        return ast_node::make<ast::class_head>(key, class_head_name.clone(), std::move(resolved_base_clause));
    }

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
struct class_specifier : public user_defined_type_specifier {
    AST_GET_TYPE;
    AST_CLONE_AUTO(head.clone(), member_spec_seq.clone(), sym);

    class_specifier(ast_node&& class_head, ast_node&& member_spec_seq, symbol_ref sym)
        : head(std::move(class_head)), member_spec_seq(std::move(member_spec_seq)), sym(sym) {}

    symbol_ref resolve_to_symbol(parse_state& ps) const override {
        return sym;
    }

    bool is_dependent() const override {
        return head.is_dependent() || member_spec_seq.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_head =
            head.is_dependent()
            ? head.resolve_dependent(args, arg_count)
            : head.clone();
        ast_node resolved_member_spec_seq =
            member_spec_seq.is_dependent()
            ? member_spec_seq.resolve_dependent(args, arg_count)
            : member_spec_seq.clone();
        return ast_node::make<ast::class_specifier>(std::move(resolved_head), std::move(resolved_member_spec_seq), nullptr);
    }

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
    symbol_ref sym;
};

struct declaration_seq : public node {
    AST_GET_TYPE;

    node* clone() const override {
        auto p = new declaration_seq();
        for (auto& n : seq) {
            p->push_back(n.clone());
        }
        return p;
    }

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
    AST_CLONE_AUTO(declarator.clone(), initializer.clone());

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

// TODO: implement resolve_dependent()
// if template variables are supported
struct init_declarator_list : public node {    
    AST_GET_TYPE;

    node* clone() const override {
        auto p = new init_declarator_list();
        for (auto& n : list) {
            p->push_back(n.clone());
        }
        return p;
    }

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
    AST_CLONE_AUTO(ident.clone(), type_id_.clone());

    alias_declaration(ast_node&& ident, ast_node&& type_id_)
        : ident(std::move(ident)), type_id_(std::move(type_id_)) {}
    
    bool is_dependent() const override {
        return type_id_.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_tid =
            type_id_.is_dependent()
            ? type_id_.resolve_dependent(args, arg_count)
            : type_id_.clone();
        return ast_node::make<ast::alias_declaration>(ident.clone(), std::move(resolved_tid));
    }

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
    AST_CLONE_AUTO(decl_spec.clone(), declarator_list.clone());

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
    AST_CLONE_AUTO(expr.clone(), strlit.clone());

    static_assert_(ast_node&& expr, ast_node&& strlit)
        : expr(std::move(expr)), strlit(std::move(strlit)) {}
    
    bool is_dependent() const override {
        return expr.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_expr =
            expr.is_dependent()
            ? expr.resolve_dependent(args, arg_count)
            : expr.clone();
        return ast_node::make<ast::static_assert_>(std::move(resolved_expr), strlit.clone());
    }

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
    AST_CLONE_AUTO(ident.clone(), decl_seq.clone(), is_inline);

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
    AST_CLONE_AUTO(ident.clone(), initializer.clone(), is_variadic);

    type_parameter(ast_node&& ident, bool is_variadic)
        : ident(std::move(ident)), initializer(nullptr), is_variadic(is_variadic) {}
    type_parameter(ast_node&& ident, ast_node&& default_init, bool is_variadic)
        : ident(std::move(ident)), initializer(std::move(default_init)), is_variadic(is_variadic) {}
            
    bool is_dependent() const override {
        return initializer.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_initializer =
            initializer.is_dependent()
            ? initializer.resolve_dependent(args, arg_count)
            : initializer.clone();
        return ast_node::make<ast::type_parameter>(ident.clone(), std::move(resolved_initializer), is_variadic);
    }

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

    node* clone() const override {
        auto p = new template_parameter_list();
        for (auto& n : list) {
            p->push_back(n.clone());
        }
        return p;
    }

    template_parameter_list() {}

    void push_back(ast_node n) {
        list.push_back(std::move(n));
    }
            
    bool is_dependent() const override {
        for (auto& n : list) {
            if(n.is_dependent()) return true;
        }
        return false;
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_tpl = ast_node::make<ast::template_parameter_list>();
        for (const auto& n : list) {
            ast_node resolved_n =
                n.is_dependent()
                ? n.resolve_dependent(args, arg_count)
                : n.clone();
            resolved_tpl.as<ast::template_parameter_list>()->push_back(std::move(resolved_n));
        }
        return resolved_tpl;
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
    AST_CLONE_AUTO(param_list.clone(), decl.clone());

    template_declaration(ast_node&& param_list, ast_node&& decl)
        : param_list(std::move(param_list)), decl(std::move(decl)) {}
        
    bool is_dependent() const override {
        return param_list.is_dependent() || decl.is_dependent();
    }
    ast_node resolve_dependent(const template_argument* args, int arg_count) const override {
        ast_node resolved_param_list =
            param_list.is_dependent()
            ? param_list.resolve_dependent(args, arg_count)
            : param_list.clone();
        ast_node resolved_decl =
            decl.is_dependent()
            ? decl.resolve_dependent(args, arg_count)
            : decl.clone();

        return ast_node::make<ast::template_declaration>(std::move(resolved_param_list), std::move(resolved_decl));
    }

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

    node* clone() const override {
        auto p = new translation_unit();
        for (auto& n : declarations) {
            p->add_decl(n.clone());
        }
        return p;
    }

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

