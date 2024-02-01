#pragma once

#include <vector>
#include "types.hpp"
#include "program.hpp"


namespace animvm {

    
    enum class ast_type {
        ast_none,
        ast_translation_unit,
        ast_expr_stmt,
        ast_lit_numeric,
        ast_identifier,
        ast_binary_op,
        ast_unary_op,
        ast_declaration,
        ast_type_specifier_seq,
        ast_declarator,
        ast_return_stmt,
        ast_host_event_stmt
    };

    struct ast_node_base {
        virtual ~ast_node_base() {}
    };

    struct ast_translation_unit;
    struct ast_expr_stmt;
    struct ast_binary_op;
    struct ast_unary_op;
    struct ast_identifier;
    struct ast_lit_numeric;
    struct ast_declaration;
    struct ast_type_specifier_seq;
    struct ast_declarator;
    struct ast_return_stmt;
    struct ast_host_event_stmt;
    struct ast_node {
        ast_type type = ast_type::ast_none;
        union {
            ast_node_base* _ptr_base;

            ast_translation_unit* translation_unit;
            ast_expr_stmt* expr_stmt;
            ast_binary_op* binary_op;
            ast_unary_op* unary_op;
            ast_identifier* identifier;
            ast_lit_numeric* lit_numeric;
            ast_declaration* declaration;
            ast_type_specifier_seq* type_specifier_seq;
            ast_declarator* declarator;
            ast_return_stmt* return_stmt;
            ast_host_event_stmt* host_event_stmt;
        };

        void free() {
            if (_ptr_base) {
                delete _ptr_base;
            }
        }

        ast_node()
            : _ptr_base(0) {}

        float eval();
        void make_program(vm_program& prog, bool lvalue = false);
    };
    struct ast_translation_unit : public ast_node_base {
        std::vector<ast_node> statements;
    };
    struct ast_expr_stmt : public ast_node_base {
        ast_node expr;
    };
    struct ast_binary_op : public ast_node_base {
        ast_node left;
        ast_node right;
        std::string op;
    };
    struct ast_unary_op : public ast_node_base {
        ast_node operand;
        std::string op;
    };
    struct ast_identifier : public ast_node_base {
        std::string name;
    };
    enum num_type {
        num_int,
        num_float
    };
    struct ast_lit_numeric : public ast_node_base {
        num_type type;
        union {
            float _float;
            int _int;
        };
    };
    struct ast_declaration : public ast_node_base {
        ast_node type_specifier_seq;
        ast_node declarator;
    };
    struct ast_type_specifier_seq : public ast_node_base {
        value_type type;
    };
    struct ast_declarator : public ast_node_base {
        std::string name;
    };
    struct ast_return_stmt : public ast_node_base {
        ast_node expr;
    };
    struct ast_host_event_stmt : public ast_node_base {
        std::string name;
    };

    inline float ast_node::eval() {
        switch (type) {
        case ast_type::ast_binary_op: {
            if (binary_op->op == "+") {
                return binary_op->left.eval() + binary_op->right.eval();
            } else if(binary_op->op == "-") {
                return binary_op->left.eval() - binary_op->right.eval();
            } else if(binary_op->op == "*") {
                return binary_op->left.eval() * binary_op->right.eval();
            } else if(binary_op->op == "/") {
                return binary_op->left.eval() / binary_op->right.eval();
            }
        }
        case ast_type::ast_lit_numeric: {
            if (lit_numeric->type == num_int) {
                return lit_numeric->_int;
            } else if(lit_numeric->type == num_float) {
                return lit_numeric->_float;
            }
        }
        }
        return .0f;
    }

    inline void ast_node::make_program(vm_program& prog, bool lvalue) {
        switch (type) {
        case ast_type::ast_translation_unit: {
            for (int i = 0; i < translation_unit->statements.size(); ++i) {
                translation_unit->statements[i].make_program(prog);
            }
            return;
        }
        case ast_type::ast_expr_stmt: {
            expr_stmt->expr.make_program(prog);
            prog.instructions.push_back(POP);
            return;
        }
        case ast_type::ast_return_stmt: {
            return_stmt->expr.make_program(prog);
            prog.instructions.push_back(HALT); // Just halt for now since we don't have functions
            return;
        }
        case ast_type::ast_host_event_stmt: {
            vm_host_event e = prog.find_host_event(host_event_stmt->name);
            if (e.id == -1) {
                throw parse_exception(token{}, "Host event '%s' does not exist", host_event_stmt->name.c_str());
            }
            prog.instructions.push_back(HEVT);
            prog.instructions.push_back(e.id);
            return;
        }
        case ast_type::ast_declaration: {
            value_type t = declaration->type_specifier_seq.type_specifier_seq->type;
            std::string name = declaration->declarator.declarator->name;
            prog.decl_variable(t, name);
            return;
        }
        case ast_type::ast_binary_op: {
            if (binary_op->op == "=") {
                binary_op->left.make_program(prog, true);
                binary_op->right.make_program(prog);
                prog.instructions.push_back(DSTORE);
                if (!lvalue) {
                    prog.instructions.push_back(DLOAD);
                }
                return;
            }
            
            if (lvalue) {
                throw parse_exception(token{}, "left operand must be an assignable lvalue");
            }

            if (binary_op->op == "+") {
                binary_op->left.make_program(prog);
                binary_op->right.make_program(prog);
                prog.instructions.push_back(ADDF);
                return;
            } else if(binary_op->op == "-") {
                binary_op->left.make_program(prog);
                binary_op->right.make_program(prog);
                prog.instructions.push_back(SUBF);
                return;
            } else if(binary_op->op == "*") {
                binary_op->left.make_program(prog);
                binary_op->right.make_program(prog);
                prog.instructions.push_back(MULF);
                return;
            } else if(binary_op->op == "/") {
                binary_op->left.make_program(prog);
                binary_op->right.make_program(prog);
                prog.instructions.push_back(DIVF);
                return;
            } else if(binary_op->op == "==") {
                binary_op->left.make_program(prog);
                binary_op->right.make_program(prog);
                prog.instructions.push_back(EQ);
            } else if(binary_op->op == ">") {
                binary_op->right.make_program(prog);
                binary_op->left.make_program(prog);
                prog.instructions.push_back(LT);
            } else if(binary_op->op == "<") {
                binary_op->left.make_program(prog);
                binary_op->right.make_program(prog);
                prog.instructions.push_back(LT);
            } else if(binary_op->op == ">=") {
                binary_op->right.make_program(prog);
                binary_op->left.make_program(prog);
                prog.instructions.push_back(LTE);
            } else if(binary_op->op == "<=") {
                binary_op->left.make_program(prog);
                binary_op->right.make_program(prog);
                prog.instructions.push_back(LTE);
            }
        }
        case ast_type::ast_lit_numeric: {
            if (lvalue) {
                throw parse_exception(token{}, "left operand must be an assignable lvalue");
            }

            if (lit_numeric->type == num_int) {
                prog.instructions.push_back(PUSH);
                float flt = (float)lit_numeric->_int;
                prog.instructions.push_back(*(i32*)&flt);
                return;
            } else if(lit_numeric->type == num_float) {
                prog.instructions.push_back(PUSH);
                prog.instructions.push_back(*(i32*)&lit_numeric->_float);
                return;
            }
            return;
        }
        case ast_type::ast_identifier: {
            auto var = prog.find_variable(identifier->name);
            if (var.addr == -1) {
                throw parse_exception(token{}, "Variable '%s' does not exist", identifier->name.c_str());
            }
            if (lvalue) {
                prog.instructions.push_back(PUSH);
                prog.instructions.push_back(var.addr);
            } else {
                prog.instructions.push_back(DPUSH);
                prog.instructions.push_back(var.addr);
            }
            return;
        }
        default:
            throw parse_exception(token{}, "Unknown ast node");
        }
    }


}
