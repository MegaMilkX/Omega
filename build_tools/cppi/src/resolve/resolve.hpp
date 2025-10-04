#pragma once

#include "parse_state.hpp"
#include "ast/ast.hpp"
#include "symbol_table.hpp"

void resolve_namespace(parse_state& ps, ast::namespace_definition* nspace);
void resolve_declaration_seq(parse_state& ps, ast::declaration_seq* seq);
std::shared_ptr<symbol> resolve_class_name(parse_state& ps, ast::identifier* ident);
std::shared_ptr<symbol> resolve_class_name(parse_state& ps, ast::simple_template_id* stid);
std::shared_ptr<symbol> resolve_class_name(parse_state& ps, ast::nested_name_specifier* nns);
std::shared_ptr<symbol> resolve_class_name(parse_state& ps, ast_node& name);
void resolve_class_specifier(parse_state& ps, ast::class_specifier* spec);
void resolve_enum_specifier(parse_state& ps, ast::enum_specifier* spec);
void resolve_simple_declaration(parse_state& ps, ast::simple_declaration* decl);
void resolve_function_definition(parse_state& ps, ast::function_definition* def);
void resolve_choose(parse_state& ps, ast_node& n);
void resolve(parse_state& ps, ast_node& n);

