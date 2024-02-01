#include "parser.hpp"


namespace animvm {


    bool parse_expr_helper(parse_state& ps, ast_node& node, bool(*left)(parse_state&, ast_node&), const std::vector<const char*> ops, bool(*right)(parse_state&, ast_node&)) {
        if (!left(ps, node)) {
            return false;
        }
        while (true) {
            bool fit = false;
            int op_i = -1;
            for (int i = 0; i < ops.size(); ++i) {
                if (ps.accept(ops[i])) {
                    fit = true;
                    op_i = i;
                    break;
                }
            }
            if (!fit) {
                break;
            }
            ast_node nright;
            if (!right(ps, nright)) {
                throw parse_exception(ps.tok(), "Expected an expression");
            }
            ast_node n;
            n.type = ast_type::ast_binary_op;
            n.binary_op = new ast_binary_op;
            n.binary_op->left = node;
            n.binary_op->right = nright;
            n.binary_op->op = ops[op_i];
            node = n;
        }
        return true;
    }

    bool parse_expression(parse_state& ps, ast_node& node);
    bool parse_primary_expr(parse_state& ps, ast_node& node) {
        if (ps.accept("(")) {
            if (!parse_expression(ps, node)) {
                throw parse_exception(ps.tok(), "Expected an expression");
            }
            if (!ps.accept(")")) {
                throw parse_exception(ps.tok(), "Expected a closing parenthesis");
            }
            return true;
        }

        token tok = ps.tok();
        if (ps.accept(e_identifier)) {
            node.type = ast_type::ast_identifier;
            node.identifier = new ast_identifier;
            node.identifier->name = tok.str;
            return true;
        }
        if(ps.accept(e_literal_integral)
            || ps.accept(e_literal_floating))
        {
            node.type = ast_type::ast_lit_numeric;
            node.lit_numeric = new ast_lit_numeric;
            if (tok.type == e_literal_integral) {
                node.lit_numeric->type = num_int;
                node.lit_numeric->_int = tok._int;
            } else if(tok.type == e_literal_floating) {
                node.lit_numeric->type = num_float;
                node.lit_numeric->_float = tok._float;
            } else {
                assert(false);
                return false;
            }
            return true;
        }
        return false;
    }
    bool parse_unary_op(parse_state& ps, ast_node& node) {
        token tok = ps.tok();
        if (ps.accept("!")
            || ps.accept("+")
            || ps.accept("-")
            || ps.accept("~"))
        {
            node.type = ast_type::ast_unary_op;
            node.unary_op = new ast_unary_op;
            node.unary_op->op = tok.str;
            return true;
        }
        return false;
    }
    bool parse_unary_expr(parse_state& ps, ast_node& node) {
        ps.push_rewind();
        if (parse_unary_op(ps, node)) {
            if (!parse_unary_expr(ps, node.unary_op->operand)) {
                ps.rewind();
                return false;
            }
            ps.pop_rewind();
            return true;
        } else if(parse_primary_expr(ps, node)) {
            ps.pop_rewind();
            return true;
        }
        ps.rewind();
        return false;
    }
    bool parse_multiplicative_expr(parse_state& ps, ast_node& node) {
        return parse_expr_helper(ps, node, &parse_unary_expr, { "*", "/", "%" }, &parse_unary_expr);
    }
    bool parse_additive_expr(parse_state& ps, ast_node& node) {
        return parse_expr_helper(ps, node, &parse_multiplicative_expr, { "+", "-" }, &parse_multiplicative_expr);
    }
    bool parse_shift_expr(parse_state& ps, ast_node& node) {
        return parse_expr_helper(ps, node, &parse_additive_expr, { "<<", ">>" }, &parse_additive_expr);
    }
    bool parse_relational_expr(parse_state& ps, ast_node& node) {
        return parse_expr_helper(ps, node, &parse_shift_expr, { "<", ">", "<=", ">=" }, &parse_shift_expr);
    }
    bool parse_equality_expr(parse_state& ps, ast_node& node) {
        return parse_expr_helper(ps, node, &parse_relational_expr, { "==", "!=" }, &parse_relational_expr);
    }
    bool parse_and_expr(parse_state& ps, ast_node& node) {
        return parse_expr_helper(ps, node, &parse_equality_expr, { "&" }, &parse_equality_expr);
    }
    bool parse_exclusive_or_expr(parse_state& ps, ast_node& node) {
        return parse_expr_helper(ps, node, &parse_and_expr, { "^" }, &parse_and_expr);
    }
    bool parse_inclusive_or_expr(parse_state& ps, ast_node& node) {
        return parse_expr_helper(ps, node, &parse_exclusive_or_expr, { "|" }, &parse_exclusive_or_expr);
    }
    bool parse_logical_and_expr(parse_state& ps, ast_node& node) {
        return parse_expr_helper(ps, node, &parse_inclusive_or_expr, { "&&" }, &parse_inclusive_or_expr);
    }
    bool parse_logical_or_expr(parse_state& ps, ast_node& node) {
        return parse_expr_helper(ps, node, &parse_logical_and_expr, { "||" }, &parse_logical_and_expr);
    }
    bool parse_assignment_expr(parse_state& ps, ast_node& node) {
        return parse_expr_helper(ps, node, &parse_logical_or_expr, { "=" }, &parse_logical_or_expr);
    }
    bool parse_expression(parse_state& ps, ast_node& node) {
        return parse_assignment_expr(ps, node);
    }

    bool parse_type_specifier_seq(parse_state& ps, ast_node& node) {
        value_type type;
        if (ps.accept(kw_float)) {
            type = type_float;
        } else if(ps.accept(kw_int)) {
            type = type_int;
        } else if(ps.accept(kw_bool)) {
            type = type_bool;
        } else {
            return false;
        }

        node.type = ast_type::ast_type_specifier_seq;
        node.type_specifier_seq = new ast_type_specifier_seq;
        node.type_specifier_seq->type = type;

        return true;
    }
    bool parse_declarator(parse_state& ps, ast_node& node) {
        token tok = ps.tok();
        if (!ps.accept(e_identifier)) {
            return false;
        }
        node.type = ast_type::ast_declarator;
        node.declarator = new ast_declarator;
        node.declarator->name = tok.str;
        return true;
    }
    bool parse_declaration(parse_state& ps, ast_node& node) {
        ast_node type_specifier_seq;
        if (!parse_type_specifier_seq(ps, type_specifier_seq)) {
            return false;
        }

        ast_node declarator;
        if (!parse_declarator(ps, declarator)) {
            throw parse_exception(ps.tok(), "Expected a declarator");
        }

        ps.expect_throw(";");

        node.type = ast_type::ast_declaration;
        node.declaration = new ast_declaration;
        node.declaration->type_specifier_seq = type_specifier_seq;
        node.declaration->declarator = declarator;

        return true;
    }

    bool parse_expression_statement(parse_state& ps, ast_node& node) {
        ast_node expr;
        if (!parse_expression(ps, expr)) {
            return false;
        }
        ps.expect_throw(";");

        node.type = ast_type::ast_expr_stmt;
        node.expr_stmt = new ast_expr_stmt;
        node.expr_stmt->expr = expr;

        return true;
    }

    bool parse_return_statement(parse_state& ps, ast_node& node) {
        if (!ps.accept(kw_return)) {
            return false;
        }
        ast_node expr;
        if (!parse_expression(ps, expr)) {
            throw parse_exception(ps.tok(), "return: expected an expression");
        }

        ps.expect_throw(";");

        node.type = ast_type::ast_return_stmt;
        node.return_stmt = new ast_return_stmt;
        node.return_stmt->expr = expr;

        return true;
    }

    bool parse_host_event_statement(parse_state& ps, ast_node& node) {
        if (!ps.accept("@")) {
            return false;
        }
        token tok = ps.tok();
        if (!ps.accept(e_identifier)) {
            throw parse_exception(tok, "@: expected a host event identifier");
        }

        ps.expect_throw(";");

        node.type = ast_type::ast_host_event_stmt;
        node.host_event_stmt = new ast_host_event_stmt;
        node.host_event_stmt->name = tok.str;
    }

    bool parse_statement(parse_state& ps, ast_node& node) {
        return parse_declaration(ps, node)
            || parse_expression_statement(ps, node)
            || parse_return_statement(ps, node)
            || parse_host_event_statement(ps, node);
    }

    bool parse(const char* source, ast_node& node) {
        try {
            parse_state ps(source);

            node.free();
            node.type = ast_type::ast_translation_unit;
            node.translation_unit = new ast_translation_unit;
            while (!ps.is_eof()) {
                ps.push_rewind();

                node.translation_unit->statements.push_back(ast_node());
                parse_statement(ps, node.translation_unit->statements.back());

                if (ps.count_read() == 0) {
                    LOG_ERR("Failed to parse animvm program!");
                    return false;
                }
            }
        } catch(const parse_exception& ex) {
            LOG_ERR("anim expression error: " << ex.what());
            return false;
        }
        return true;
    }

}
