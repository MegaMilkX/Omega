#pragma once

#include <typeinfo>
#include <list>
#include <vector>
#include "parse_state.hpp"
#include "decl_specifier.hpp"


enum PTR_DECL_TYPE {
    PTR_DECL_PTR = 0x1,
    PTR_DECL_LVALUE_REF = 0x2,
    PTR_DECL_RVALUE_REF = 0x4
};
struct ptr_decl_part {
    PTR_DECL_TYPE type;
    int cv_flags;
};



struct ast_node {
    virtual const std::type_info& get_type() const { return typeid(decltype(*this)); }
    template<typename NODE_T>
    NODE_T* cast_to() const { if (get_type() == typeid(NODE_T)) return (NODE_T*)this; else return 0; }
    virtual ~ast_node() {}
};
template<typename T>
using uptr = std::unique_ptr<T>;

#include "declarator.hpp"

constexpr int DECL_TYPE_FUNCTION = 0x01;
constexpr int DECL_TYPE_ARRAY = 0x02;
constexpr int DECL_TYPE_PTR = 0x04;
constexpr int DECL_TYPE_LVALUE_REF = 0x08;
constexpr int DECL_TYPE_RVALUE_REF = 0x10;
constexpr int DECL_TYPE_CONST = 0x20;
constexpr int DECL_TYPE_VOLATILE = 0x40;

bool eat_braced_init_list(parse_state& ps);
bool eat_virt_specifier_seq(parse_state& ps);

bool eat_initializer(parse_state& ps);

bool eat_declarator(parse_state& ps, decl_spec& dspec, decl_type& dtype, declarator* d, std::shared_ptr<symbol>* out_sym, bool is_abstract = false);
bool eat_init_declarator(parse_state& ps, decl_spec& dspec, decl_type& dtype, declarator* d);

bool eat_init_declarator_list(parse_state& ps, decl_spec& dspec, decl_type& dtype, std::vector<declarator>* list);
bool eat_declarator_list(parse_state& ps, decl_spec& dspec, decl_type& dtype, std::vector<declarator>* list);

bool eat_abstract_declarator(parse_state& ps, decl_or_type_specifier_seq* dss, declarator* d);

bool eat_function_body(parse_state& ps, declarator* d);
bool eat_function_definition(parse_state& ps);
