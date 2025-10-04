#pragma once

#include "types.hpp"
#include "common.hpp"
#include "parse_state.hpp"
#include "ast/ast.hpp"
#include "decl_spec_compat.hpp"
#include "parsing/attribute_specifier.hpp"

// =========================================
// Common
// =========================================

ast_node eat_identifier_2(parse_state& ps);

// =========================================
// Expression
// =========================================

ast_node eat_literal_2(parse_state& ps);
ast_node eat_this_2(parse_state& ps);

ast_node eat_primary_expression_2(parse_state& ps);

ast_node eat_unqualified_id_2(parse_state& ps);
ast_node eat_qualified_id_2(parse_state& ps);
ast_node eat_id_expression_2(parse_state& ps);

ast_node eat_nested_name_specifier_opener_syn(parse_state& ps);
ast_node eat_nested_name_specifier_syn(parse_state& ps);
ast_node eat_nested_name_specifier_opener_sem(parse_state& ps);
ast_node eat_nested_name_specifier_sem(parse_state& ps);
ast_node eat_nested_name_specifier_2(parse_state& ps);

ast_node eat_sizeof_expr_2(parse_state& ps);

ast_node eat_noexcept_expr_2(parse_state& ps);
ast_node eat_new_expr_2(parse_state& ps);
ast_node eat_cast_expression_2(parse_state& ps);
ast_node eat_delete_expr_2(parse_state& ps);

ast_node eat_pseudo_destructor_name_2(parse_state& ps);

ast_node eat_postfix_expr_suffix_2(parse_state& ps, ast_node&& wrapped);
ast_node eat_postfix_expr_2(parse_state& ps);
ast_node eat_unary_expression_2(parse_state& ps);
ast_node eat_cast_expression_2(parse_state& ps);
ast_node eat_pm_expression_2(parse_state& ps);
ast_node eat_multiplicative_expression_2(parse_state& ps);
ast_node eat_additive_expression_2(parse_state& ps);
ast_node eat_shift_expression_2(parse_state& ps);
ast_node eat_relational_expression_2(parse_state& ps);
ast_node eat_equality_expression_2(parse_state& ps);
ast_node eat_and_expression_2(parse_state& ps);
ast_node eat_exclusive_or_expression_2(parse_state& ps);
ast_node eat_inclusive_or_expression_2(parse_state& ps);
ast_node eat_logical_and_expression_2(parse_state& ps);
ast_node eat_logical_or_expression_2(parse_state& ps);
ast_node eat_conditional_expression_2(parse_state& ps);
ast_node eat_assignment_expression_2(parse_state& ps);
ast_node eat_expression_2(parse_state& ps);
ast_node eat_expression_list_2(parse_state& ps);
ast_node eat_constant_expression_2(parse_state& ps);

// =========================================
// Declarator
// =========================================

ast_node eat_initializer_clause_2(parse_state& ps);
ast_node eat_initializer_list_2(parse_state& ps);
ast_node eat_braced_init_list_2(parse_state& ps);
ast_node eat_cv_qualifier_2(parse_state& ps);
ast_node eat_cv_qualifier_seq_2(parse_state& ps);

ast_node eat_parameter_declaration_2(parse_state& ps);
ast_node eat_parameter_declaration_list_2(parse_state& ps);
ast_node eat_parameter_declaration_clause_2(parse_state& ps);
ast_node eat_ref_qualifier_2(parse_state& ps);

ast_node eat_parameters_and_qualifiers_2(parse_state& ps);
ast_node eat_ptr_declarator_core_2(parse_state& ps, ast_node& first, ast_node*& last, bool is_abstract);
ast_node eat_noptr_declarator_secondary_core_2(parse_state& ps, ast_node& first, ast_node*& last, bool is_abstract);
ast_node eat_noptr_declarator_paren_core_2(parse_state& ps, ast_node& first, ast_node*& last, bool is_abstract);
ast_node eat_noptr_declarator_core_2(parse_state& ps, ast_node& first, ast_node*& last, bool is_abstract);
ast_node eat_declarator_core_2(parse_state& ps, bool is_abstract);
ast_node eat_noptr_declarator_secondary_2(parse_state& ps, ast_node*& last);
ast_node eat_declarator_2(parse_state& ps);
ast_node eat_abstract_declarator_2(parse_state& ps);

ast_node eat_ptr_operator_2(parse_state& ps);

ast_node eat_declarator_id_2(parse_state& ps);
ast_node eat_type_id_2(parse_state& ps);

ast_node eat_brace_or_equal_initializer_2(parse_state& ps);

ast_node eat_function_body_2(parse_state& ps);

ast_node eat_initializer_2(parse_state& ps);
ast_node eat_init_declarator_2(parse_state& ps);
ast_node eat_init_declarator_list_2(parse_state& ps);

// =========================================
// Template
// =========================================

ast_node eat_typename_specifier_2(parse_state& ps);
ast_node eat_template_argument_2(parse_state& ps);
ast_node eat_template_argument_list_2(parse_state& ps);
void expect_closing_angle(parse_state& ps);
ast_node eat_simple_template_id_2(parse_state& ps);
ast_node eat_template_id_2(parse_state& ps);
ast_node eat_type_parameter_2(parse_state& ps);
ast_node eat_template_parameter_2(parse_state& ps);
ast_node eat_template_parameter_list_2(parse_state& ps);
ast_node eat_template_declaration_2(parse_state& ps);

// =========================================
// Declaration
// =========================================

ast_node eat_elaborated_type_specifier_class_syn(parse_state& ps);
ast_node eat_elaborated_type_specifier_enum_syn(parse_state& ps);
ast_node eat_elaborated_type_specifier_syn(parse_state& ps);
ast_node eat_elaborated_type_specifier_class_sem(parse_state& ps, decl_flags& flags);
ast_node eat_elaborated_type_specifier_enum_sem(parse_state& ps);
ast_node eat_elaborated_type_specifier_sem(parse_state& ps, decl_flags& flags);
ast_node eat_elaborated_type_specifier_2(parse_state& ps, decl_flags& flags);
ast_node eat_trailing_type_specifier_2(parse_state& ps, decl_flags& flags);
bool eat_enum_key_2(parse_state& ps, e_enum_key& key);
ast_node eat_enum_base_2(parse_state& ps);
// Unused?
ast_node eat_enum_head_2(parse_state& ps);
ast_node eat_enumerator_definition_2(parse_state& ps);
ast_node eat_enumerator_list_2(parse_state& ps);
ast_node eat_enum_specifier_2(parse_state& ps);

ast_node eat_type_specifier_2(parse_state& ps, decl_flags& flags);
ast_node eat_type_specifier_seq_2(parse_state& ps);

ast_node eat_storage_class_specifier_2(parse_state& ps, decl_flags& flags);
ast_node eat_function_specifier_2(parse_state& ps, decl_flags& flags);
ast_node eat_decl_specifier_2(parse_state& ps, decl_flags& flags);
ast_node eat_decl_specifier_seq_2(parse_state& ps);
ast_node eat_simple_type_specifier_2(parse_state& ps, decl_flags& flags);
ast_node eat_type_name_syn(parse_state& ps);
ast_node eat_type_name_sem(parse_state& ps);
ast_node eat_type_name_2(parse_state& ps);
ast_node eat_decltype_specifier_2(parse_state& ps);

ast_node eat_alias_declaration_2(parse_state& ps);
ast_node eat_simple_declaration_2(parse_state& ps);
ast_node eat_static_assert_declaration_2(parse_state& ps);
ast_node eat_opaque_enum_declaration_2(parse_state& ps);
ast_node eat_block_declaration_2(parse_state& ps);

ast_node eat_declaration_2(parse_state& ps);
bool eat_declaration_seq_limited_block_2(parse_state& ps, ast::declaration_seq* seq);
ast_node eat_declaration_seq_limited_2(parse_state& ps);
ast_node eat_declaration_seq_2(parse_state& ps);

ast_node eat_namespace_definition_2(parse_state& ps);

// =========================================
// Exception handling
// =========================================

ast_node eat_dynamic_exception_specification_2(parse_state& ps);
ast_node eat_noexcept_specification_2(parse_state& ps);
ast_node eat_exception_specification_2(parse_state& ps);
ast_node eat_type_id_list_2(parse_state& ps);

ast_node eat_exception_declaration_2(parse_state& ps);
ast_node eat_handler_2(parse_state& ps);
ast_node eat_handler_seq_2(parse_state& ps);
ast_node eat_function_try_block_2(parse_state& ps);

// =========================================
// Overloading
// =========================================

ast_node eat_operator_function_id_2(parse_state& ps);
ast_node eat_literal_operator_id_2(parse_state& ps);

// =========================================
// Special member functions
// =========================================

ast_node eat_conversion_declarator_2(parse_state& ps);
ast_node eat_conversion_type_id_2(parse_state& ps);
ast_node eat_conversion_function_id_2(parse_state& ps);

// =========================================
// Classes
// =========================================

bool eat_class_key_2(parse_state& ps, e_class_key& out_key);
ast_node eat_class_name_2(parse_state& ps);
ast_node eat_class_head_name_syn(parse_state& ps);
ast_node eat_class_head_name_sem(parse_state& ps);
ast_node eat_class_head_name_2(parse_state& ps);
bool eat_access_specifier_2(parse_state& ps, e_access_specifier& access);
ast_node eat_class_or_decltype_2(parse_state& ps);
ast_node eat_base_type_specifier_2(parse_state& ps);
ast_node eat_base_specifier_2(parse_state& ps);
ast_node eat_base_specifier_list_2(parse_state& ps);
ast_node eat_base_clause_2(parse_state& ps);
ast_node eat_class_head_2(parse_state& ps);
bool eat_virt_specifier_2(parse_state& ps, e_virt& virt);
e_virt_flags eat_virt_specifier_seq_2(parse_state& ps);
bool eat_pure_specifier_2(parse_state& ps);

ast_node eat_member_declarator_2(parse_state& ps);
ast_node eat_member_declarator_list_2(parse_state& ps);
ast_node eat_mem_initializer_id_2(parse_state& ps);
ast_node eat_mem_initializer_2(parse_state& ps);
ast_node eat_mem_initializer_list_2(parse_state& ps);
ast_node eat_ctor_initializer_2(parse_state& ps);

ast_node eat_member_declaration_2(parse_state& ps);
ast_node eat_member_specification_one_2(parse_state& ps);
ast_node eat_member_specification_2(parse_state& ps);
ast_node eat_member_specification_limited_one_2(parse_state& ps);
ast_node eat_member_specification_limited_2(parse_state& ps);
ast_node eat_class_specifier_2(parse_state& ps);

// =========================================
// Statements
// =========================================

ast_node eat_compound_statement_2(parse_state& ps);

// =========================================
// Attributes
// =========================================

ast_node eat_attribute_specifier_seq_2(parse_state& ps);

// =========================================
// Translation unit
// =========================================

ast_node eat_translation_unit_2(parse_state& ps);

