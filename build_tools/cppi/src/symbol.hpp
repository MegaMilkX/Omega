#pragma once

#include <string>
#include <map>
#include <memory>
#include "attribute.hpp"
#include "token.hpp"
#include "types.hpp"
#include "template_parameter.hpp"
#include "parse_exception.hpp"
#include "ast/ast.hpp"


enum e_symbol_type {
    e_symbol_object,
    e_symbol_function,
    e_symbol_alias,
    e_symbol_class,
    e_symbol_template,
    e_symbol_template_parameter,
    e_symbol_enum,
    e_symbol_enumerator,
    e_symbol_namespace,

    e_symbol_function_overload,
    e_symbol_placeholder,
    e_symbol_unknown
};

class symbol_table;
class symbol {
protected:
    void copy_to(symbol* dest) const;
public:
    virtual ~symbol() {}

    // file is used to see which files to include
    // when generating reflection code
    // definition > declaration
    const PP_FILE* file = 0;
    std::string name;
    std::string global_qualified_name;
    std::string local_internal_name;
    std::string internal_name;
    std::shared_ptr<symbol_table> nested_symbol_table;
    attribute_specifier attrib_spec;
    // TODO: declared can be false if a rule created a symbol,
    // but otherwise didn't match and backtracked
    bool declared = true;
    bool defined = false;
    bool no_reflect = false;
    // TODO: remove this
    // used for template patterns
    bool no_lookup = false;

    template<typename T>
    bool is() const { return typeid(T) == get_type(); }
    template<typename T>
    const T* as() const { return is<T>() ? (const T*)this : nullptr; }
    template<typename T>
    T* as() { return is<T>() ? (T*)this : nullptr; }

    virtual const type_info& get_type() const = 0;
    virtual e_symbol_type get_type_enum() const = 0;

    bool is_declared() const { return declared; }
    void set_declared(bool d) { declared = d; }

    bool is_defined() const { return defined; }
    void set_defined(bool d) { defined = d; }

    const std::string& get_local_source_name() const { return name; }
    const std::string& get_source_name() const { return global_qualified_name; }
    const std::string& get_local_internal_name() const { return local_internal_name; }
    const std::string& get_internal_name() const { return internal_name; }
    std::string get_full_internal_name() const { return "_Z" + get_internal_name(); }

    virtual std::shared_ptr<symbol> clone() const = 0;

    virtual void dbg_print(int indent = 0) const { dbg_printf_color_indent("<symbol::dbg_print() not implemented>", indent, DBG_WHITE); }
};

using symbol_ref = std::shared_ptr<symbol>;

template<e_symbol_type SYM_TYPE>
class t_symbol : public symbol {
public:
    e_symbol_type get_type_enum() const override {
        return SYM_TYPE;
    }
};


class symbol_object : public t_symbol<e_symbol_object> {
    const type_info& get_type() const override { return typeid(*this); }
    void dbg_print(int indent = 0) const override { 
        dbg_printf_color_indent("data %s", indent, DBG_RED | DBG_GREEN | DBG_BLUE, global_qualified_name.c_str());
        printf(" ");
        //type_id_.dbg_print();
        std::string tid_name;
        tid.build_source_name(tid_name);
        dbg_printf_color(tid_name.c_str(), DBG_WHITE);
    }
public:
    TYPE_ID tid = nullptr;

    std::shared_ptr<symbol> clone() const override {
        std::shared_ptr<symbol_object> sym(new symbol_object);
        copy_to(sym.get());
        sym->tid = tid;
        return sym;
    }
};
class symbol_func_overload : public t_symbol<e_symbol_function_overload> {
    const type_info& get_type() const override { return typeid(*this); }
    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("%s", indent, DBG_WHITE, internal_name.c_str());
        printf(" ");
        //type_id_.dbg_print();
        std::string tid_name;
        tid.build_source_name(tid_name);
        dbg_printf_color(tid_name.c_str(), DBG_WHITE);
    }
public:
    TYPE_ID tid = nullptr;

    std::shared_ptr<symbol_func_overload> clone_keep_type() const {
        std::shared_ptr<symbol_func_overload> sym(new symbol_func_overload);
        copy_to(sym.get());
        sym->tid = tid;
        return sym;
    }
    std::shared_ptr<symbol> clone() const override {
        return clone_keep_type();
    }
};
class symbol_function : public t_symbol<e_symbol_function> {
    const type_info& get_type() const override { return typeid(*this); }
    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("function %s", indent, DBG_RED | DBG_GREEN | DBG_BLUE, global_qualified_name.c_str());
        for (auto& o : overloads) {
            printf("\n");
            ((symbol*)o.get())->dbg_print(indent + 1);
        }
    }
public:
    std::vector<std::shared_ptr<symbol_func_overload>> overloads;

    std::shared_ptr<symbol_func_overload> get_or_create_overload(TYPE_ID tid) {
        for (int i = 0; i < overloads.size(); ++i) {
            if (overloads[i]->tid == tid) {
                return overloads[i];
            }

            // TODO:
            //const type_id_2_function* a = overloads[i]->tid->as<type_id_2_function>();
            //const type_id_2_function* b = tid->as<type_id_2_function>();
            // TODO: compare function parts
            // TODO: if same, error if return types are different
            // TODO: otherwise - return existing overload
        }

        std::shared_ptr<symbol_func_overload> sptr(new symbol_func_overload);
        sptr->name = name;
        sptr->defined = false;
        sptr->tid = tid;
        overloads.push_back(sptr);
        return sptr;
    }
    /*
    std::shared_ptr<symbol_func_overload> get_or_create_overload(const type_id& tid) {
        if (overloads.empty()) {
            std::shared_ptr<symbol_func_overload> sptr(new symbol_func_overload);
            sptr->name = name;
            sptr->defined = false;
            sptr->type_id_ = tid;
            overloads.push_back(sptr);
            return sptr;
        } else {
            for (int i = 0; i < overloads.size(); ++i) {
                // This is suboptimal, the 'function' part of the type-id is checked twice
                if (overloads[i]->type_id_.get_first()->equals(tid.get_first())) {
                    if (overloads[i]->type_id_ != tid) {
                        throw parse_exception("Function overload differs only by return type", "", name.c_str(), 0, 0);
                    }
                    return overloads[i];
                }
            }
            std::shared_ptr<symbol_func_overload> sptr(new symbol_func_overload);
            sptr->name = name;
            sptr->defined = false;
            sptr->type_id_ = tid;
            overloads.push_back(sptr);
            return sptr;
        }
    }*/

    std::shared_ptr<symbol> clone() const override {
        std::shared_ptr<symbol_function> sym(new symbol_function);
        copy_to(sym.get());
        for (auto& o : overloads) {
            sym->overloads.push_back(o->clone_keep_type());
        }
        return sym;
    }
};
class symbol_class : public t_symbol<e_symbol_class> {
    const type_info& get_type() const override { return typeid(*this); }
    void dbg_print(int indent = 0) const override { 
        dbg_printf_color_indent("class %s", indent, DBG_RED | DBG_GREEN | DBG_BLUE, global_qualified_name.c_str());
    }
public:
    std::vector<std::shared_ptr<symbol>> base_classes;

    std::shared_ptr<symbol> clone() const override {
        std::shared_ptr<symbol_class> sym(new symbol_class);
        copy_to(sym.get());
        for (auto& b : base_classes) {
            sym->base_classes.push_back(b->clone());
        }
        return sym;
    }
};
class symbol_enum : public t_symbol<e_symbol_enum> {
    const type_info& get_type() const override { return typeid(*this); }
    void dbg_print(int indent = 0) const override { 
        dbg_printf_color_indent("enum %s", indent, DBG_RED | DBG_GREEN | DBG_BLUE, global_qualified_name.c_str());
    }
public:
    e_enum_key key;
    uint64_t next_enumerator_value = 0;

    bool is_unscoped() const {
        return key == e_enum_key::ck_enum;
    }

    std::shared_ptr<symbol> clone() const override {
        std::shared_ptr<symbol_enum> sym(new symbol_enum);
        copy_to(sym.get());
        sym->key = key;
        sym->next_enumerator_value = next_enumerator_value;
        return sym;
    }
};
class symbol_enumerator : public t_symbol<e_symbol_enumerator> {
    const type_info& get_type() const override { return typeid(*this); }
    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("enumerator %s", indent, DBG_GREEN | DBG_BLUE, global_qualified_name.c_str());
    }
public:
    uint64_t value = 0;

    std::shared_ptr<symbol> clone() const override {
        std::shared_ptr<symbol_enumerator> sym(new symbol_enumerator);
        copy_to(sym.get());
        return sym;
    }
};
class symbol_typedef : public t_symbol<e_symbol_alias> {
    const type_info& get_type() const override { return typeid(*this); }
    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("alias %s ", indent, DBG_RED | DBG_GREEN | DBG_BLUE, global_qualified_name.c_str());
        //printf(" is an alias for ");
        //type_id_.dbg_print();
        std::string tid_name;
        tid.build_source_name(tid_name);
        dbg_printf_color(tid_name.c_str(), DBG_WHITE);
    }
public:
    TYPE_ID tid = nullptr;

    std::shared_ptr<symbol> clone() const override {
        std::shared_ptr<symbol_typedef> sym(new symbol_typedef);
        copy_to(sym.get());
        sym->tid = tid;
        return sym;
    }
};

class parse_state;
class symbol_template_parameter;
class symbol_template : public t_symbol<e_symbol_template> {
    const type_info& get_type() const override { return typeid(*this); }
    void dbg_print(int indent = 0) const override { 
        dbg_printf_color_indent("template %s", indent, DBG_RED | DBG_GREEN | DBG_BLUE, global_qualified_name.c_str());
    }
public:
    std::shared_ptr<symbol> template_entity_sym;
    std::vector<template_parameter> template_parameters;
    const ast::node* template_class_pattern = nullptr;
    //std::vector<std::shared_ptr<symbol_template_parameter>> parameters;

    void validate_parameters() {
        bool must_have_default_arg = false;
        for (int i = 0; i < template_parameters.size(); ++i) {
            bool has_default_arg = template_parameters[i].has_default_arg();
            if (must_have_default_arg) {
                if (!has_default_arg) {
                    throw std::exception("template parameters default argument error");
                }
            }
            if (has_default_arg) {
                must_have_default_arg = true;
            }
        }
    }

    std::shared_ptr<symbol> clone() const override {
        std::shared_ptr<symbol_template> sym(new symbol_template);
        copy_to(sym.get());
        // NOTE: Actually might not need to copy instantiations
        // New ones should be created when referenced
        //instantiation_root.copy_to(&sym->instantiation_root);
        return sym;
    }
};

class symbol_template_parameter : public t_symbol<e_symbol_template_parameter> {
    const type_info& get_type() const override { return typeid(*this); }
    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("template parameter %s", indent, DBG_WHITE, global_qualified_name.c_str());
    }
public:
    e_template_parameter_kind kind;
    int param_index = 0;

    std::shared_ptr<symbol> clone() const override {
        std::shared_ptr<symbol_template_parameter> sym(new symbol_template_parameter);
        copy_to(sym.get());
        // TODO: ?
        return sym;
    }
};

class symbol_namespace : public t_symbol<e_symbol_namespace> {
    const type_info& get_type() const override { return typeid(*this); }
    void dbg_print(int indent = 0) const override;
public:
    bool is_inline = false;
    bool is_anon = false;

    std::shared_ptr<symbol> clone() const override {
        std::shared_ptr<symbol_namespace> sym(new symbol_namespace);
        copy_to(sym.get());
        sym->is_inline = is_inline;
        sym->is_anon = is_anon;
        return sym;
    }
};
// This one does not appear in the symbol table ever
// ADDED LATER: Maybe it should?
class symbol_placeholder : public t_symbol<e_symbol_placeholder> {
    const type_info& get_type() const override { return typeid(*this); }
    void dbg_print(int indent = 0) const override {
        dbg_printf_color_indent("placeholder %s", indent, DBG_RED | DBG_GREEN, global_qualified_name.c_str());
    }
public:
    std::vector<token> token_cache;

    std::shared_ptr<symbol> clone() const override {
        std::shared_ptr<symbol_placeholder> sym(new symbol_placeholder);
        copy_to(sym.get());
        for (int i = 0; i < token_cache.size(); ++i) {
            sym->token_cache.push_back(token_cache[i]);
        }
        return sym;
    }
};

