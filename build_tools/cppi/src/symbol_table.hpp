#pragma once

#include <unordered_map>
#include <map>
#include <vector>
#include <memory>
#include <string>
#include <sstream>

#include "preprocessor/pp_state.hpp"

#include "parsing/attribute.hpp"
#include "parse_exception.hpp"
#include "parsing/type_id.hpp"

#include "symbol.hpp"

#include "template_parameter.hpp"


/*
constexpr int LOOKUP_FLAG_OBJECT = 0x0001;
constexpr int LOOKUP_FLAG_FUNCTION = 0x0002;
constexpr int LOOKUP_FLAG_CLASS = 0x0004;
constexpr int LOOKUP_FLAG_NAMESPACE = 0x0008;
constexpr int LOOKUP_FLAG_ENUM = 0x0010;
constexpr int LOOKUP_FLAG_TYPEDEF = 0x0020;
constexpr int LOOKUP_FLAG_TEMPLATE = 0x0040;
constexpr int LOOKUP_FLAG_ENUMERATOR = 0x0080;
*/

constexpr int LOOKUP_FLAG_OBJECT = 0x1 << e_symbol_object;
constexpr int LOOKUP_FLAG_FUNCTION = 0x1 << e_symbol_function;
constexpr int LOOKUP_FLAG_CLASS = 0x1 << e_symbol_class;
constexpr int LOOKUP_FLAG_NAMESPACE = 0x1 << e_symbol_namespace;
constexpr int LOOKUP_FLAG_ENUM = 0x1 << e_symbol_enum;
constexpr int LOOKUP_FLAG_TYPEDEF = 0x1 << e_symbol_alias;
constexpr int LOOKUP_FLAG_TEMPLATE = 0x1 << e_symbol_template;
constexpr int LOOKUP_FLAG_TEMPLATE_PARAMETER = 0x1 << e_symbol_template_parameter;
constexpr int LOOKUP_FLAG_ENUMERATOR = 0x1 << e_symbol_enumerator;


inline const char* lookup_flag_to_string(int flag) {
    switch (flag) {
    case LOOKUP_FLAG_OBJECT: return "object";
    case LOOKUP_FLAG_FUNCTION: return "function";
    case LOOKUP_FLAG_CLASS: return "class";
    case LOOKUP_FLAG_NAMESPACE: return "namespace";
    case LOOKUP_FLAG_ENUM: return "enum";
    case LOOKUP_FLAG_TYPEDEF: return "typedef";
    case LOOKUP_FLAG_TEMPLATE: return "template";
    case LOOKUP_FLAG_ENUMERATOR: return "enumerator";
    default: return "UNKNOWN";
    }
}
inline std::string lookup_flags_to_string(int flags) {
    std::stringstream ss;
    for (int i = 0; i < 32; ++i) {
        if (flags & (1 << i)) {
            ss << lookup_flag_to_string(flags & (1 << i));
            ss << " ";
        }
    }
    return ss.str();
}

enum scope_type {
    scope_default,
    scope_class_pass_one,
    scope_class,
    scope_enum,
    scope_namespace,
    scope_template,
    scope_function_parameters
};
inline const char* scope_type_to_string(scope_type t) {
    switch (t) {
    case scope_default: return "scope_default";
    case scope_class_pass_one: return "scope_class_pass_one";
    case scope_class: return "scope_class";
    case scope_enum: return "scope_enum";
    case scope_namespace: return "scope_namespace";
    case scope_template: return "scope_template";
    case scope_function_parameters: return "scope_function_parameters";
    default: return "[UNKNOWN]";
    };
}

inline std::string gen_random_string(int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::string str;
    str.resize(len);
    for (int i = 0; i < len; ++i) {
        str[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    return str;
}


using symbol_table_ref = std::shared_ptr<symbol_table>;

struct SYMBOL_ARRAY {
    using ARRAY_T = std::vector<symbol_ref>;

    uint32_t mask = 0;
    ARRAY_T symbols;

    void push_back(symbol_ref sym) {
        symbols.push_back(sym);
        mask |= 0x1 << sym->get_type_enum();
    }

    size_t size() const { return symbols.size(); }
    void resize(size_t sz) { symbols.resize(sz); }

    ARRAY_T::const_iterator begin() const { return symbols.begin(); }
    ARRAY_T::iterator begin() { return symbols.begin(); }
    ARRAY_T::const_iterator end() const { return symbols.end(); }
    ARRAY_T::iterator end() { return symbols.end(); }

    const symbol_ref& operator[](size_t i) const { return symbols[i]; }
    symbol_ref& operator[](size_t i) { return symbols[i]; }
};

using SYMBOL_MAP = std::unordered_map<std::string, SYMBOL_ARRAY>;

class symbol_table : public std::enable_shared_from_this<symbol_table> {
    scope_type type = scope_default;
    symbol_table_ref parent;
    symbol_ref owner;
    
    void make_qualified_name(symbol_ref sym) {
        std::string global_qualified_name;
        std::string internal_name;
        bool is_in_template_scope = get_type() == scope_template;
        auto scope_owner_sym = owner.get();

        if (scope_owner_sym
            && !scope_owner_sym->global_qualified_name.empty()
            && parent
            && !is_in_template_scope
        ) {
            global_qualified_name = scope_owner_sym->global_qualified_name + std::string("::") + sym->name;
            internal_name = scope_owner_sym->get_internal_name() + sym->local_internal_name;
            //internal_name = sym->name + "@" + scope_owner_sym->get_internal_name();
        } else if(parent == nullptr
            || is_in_template_scope
            ) {
            global_qualified_name = sym->name;
            internal_name = sym->local_internal_name;
            //internal_name = sym->name;
        } else {
            //assert(false);
            global_qualified_name = "";
            internal_name = "";
        }
        sym->global_qualified_name = global_qualified_name;
        sym->internal_name = internal_name;
    }

public:
    SYMBOL_MAP symbols;

    // Only relevant for scope_template
    // temporary storage for before the template symbol is known
    std::vector<template_parameter> template_parameters;


    virtual ~symbol_table() {}

    void clone_symbol_map(
        const SYMBOL_MAP& src,
        SYMBOL_MAP& dst
    ) const {
        dst.clear();
        for (auto& kv : src) {
            const auto& v_src = kv.second;
            auto& v_dst = dst[kv.first];
            v_dst.resize(v_src.size());
            for (int i = 0; i < v_src.size(); ++i) {
                v_dst[i] = v_src[i]->clone();
            }
        }
    }

    symbol_table_ref clone() const {
        symbol_table_ref symtable(new symbol_table);
        symtable->type = type;
        symtable->parent = parent;
        symtable->owner = owner;

        clone_symbol_map(symbols, symtable->symbols);
        return symtable;
    }

    symbol_table_ref navigate_to_namespace_scope() {
        if (!parent) {
            // Global scope, considered a namespace scope
            return shared_from_this();
        }
        if (!owner) {
            dbg_printf_color("null owner for a non-root symbol_table encountered", DBG_RED);
            assert(false);
            return nullptr;
        }
        if (owner->is<symbol_namespace>()) {
            return shared_from_this();
        }
        return parent->navigate_to_namespace_scope();
    }

    void print_all(int indent = 0) {
        for (auto& kv : symbols) {
            dbg_printf_color_indent("%s: \n", indent, DBG_IDENTIFIER, kv.first.c_str());
            const auto& vec = kv.second;
            for (int i = 0; i < vec.size(); ++i) {
                vec[i]->dbg_print(indent + 1);
                printf("\n");
                if(vec[i]->nested_symbol_table) {
                    vec[i]->nested_symbol_table->print_all(indent + 2);
                }
            }
        }
    }

    void set_type(scope_type type) { this->type = type; }
    scope_type get_type() const { return type; }

    // sort for correct lookup order
    // TODO: check if order is correct
    void sort_symbols(SYMBOL_ARRAY& vec) {
        std::sort(vec.begin(), vec.end(), [](const symbol_ref& a, const symbol_ref& b) {
            return a->get_type_enum() < b->get_type_enum();
        });
    }

    template<typename T>
    symbol_ref create_symbol(
        const std::string& source_name,
        const PP_FILE* file,
        bool no_reflect
    ) {
        std::string src_name = source_name;
        if (src_name.empty()) {
            src_name = std::string("$unnamed_") + gen_random_string(16);
            no_reflect = true;
        }
        std::string internal_name = std::to_string(src_name.size()) + src_name;
        return create_symbol<T>(src_name, internal_name, file, no_reflect);
    }

    template<typename T>
    symbol_ref create_symbol(
        const std::string& source_name,
        const std::string& internal_name,
        const PP_FILE* file,
        bool no_reflect
    ) {
        return create_symbol<T>(source_name, internal_name, source_name, file, no_reflect);
    }
    template<typename T>
    symbol_ref create_symbol(
        const std::string& source_name,
        const std::string& internal_name,
        const std::string& lookup_name,
        const PP_FILE* file,
        bool no_reflect
    ) {
        auto it = symbols.find(lookup_name);
        if (it == symbols.end()) {
            it = symbols.insert(std::make_pair(lookup_name, SYMBOL_ARRAY())).first;
        }

        // Try to find an undeclared symbol
        // that we can reuse
        symbol_ref sym = nullptr;
        auto& sym_vec = it->second;
        for (int i = 0; i < sym_vec.size(); ++i) {
            auto& old_sym = sym_vec[i];
            if (old_sym->is_declared()) {
                continue;
            }
            if (old_sym->get_type() == typeid(T)) {
                if (old_sym->is_defined()) {
                    // Names already should be looked up
                    // before creating, so if we hit this,
                    // it's an implementation error
                    assert(false);
                    return nullptr;
                }
                sym = old_sym;
            }
        }

        // If no undeclared placeholder - create new
        if (!sym) {
            sym = std::make_shared<T>();
            sym_vec.push_back(sym);
            sort_symbols(sym_vec);
        } else {
            dbg_printf_color("Symbol %s reused\n", DBG_RED | DBG_BLUE | DBG_INTENSITY, sym->global_qualified_name.c_str());
        }

        // etc.
        sym->file = file;
        sym->name = source_name;
        sym->local_internal_name = internal_name;
        sym->set_declared(true);
        sym->no_reflect = no_reflect ? no_reflect : sym->no_reflect;
        
        // Create a qualified name for this symbol
        make_qualified_name(sym);

        if (type == scope_template && !this->owner && sym->get_type_enum() != e_symbol_template_parameter) {
            auto enclosing_scope = get_enclosing();
            auto tpl_sym = enclosing_scope->create_symbol<symbol_template>(source_name, file, no_reflect);
            this->owner = tpl_sym;
            tpl_sym->nested_symbol_table = shared_from_this();
            
            tpl_sym->as<symbol_template>()->template_parameters = this->template_parameters;
            this->template_parameters.clear();

            tpl_sym->as<symbol_template>()->validate_parameters();

            ((symbol_template*)tpl_sym.get())->template_entity_sym = sym;
        }
        
        return sym;
    }

    void add_symbol(const symbol_ref& sym, bool no_reflect) {
        if (sym->name.empty()) {
            sym->name = std::string("$unnamed_") + gen_random_string(16);
        }
        
        auto it = symbols.find(sym->name);
        if (it == symbols.end()) {
            it = symbols.insert(std::make_pair(sym->name, SYMBOL_ARRAY())).first;
        }
        auto& sym_vec = it->second;
        sym_vec.push_back(sym);
        sort_symbols(sym_vec);

        sym->no_reflect = no_reflect;

        make_qualified_name(sym);
    }
    // Get symbol within this scope
    symbol_ref get_symbol(const std::string& local_name, int lookup_flags) const {
        //printf("! Looking for %s | %s\n", local_name.c_str(), lookup_flags_to_string(lookup_flags).c_str());

        auto it = symbols.find(local_name);
        if (it == symbols.end()) {
            return nullptr;
        }

        const auto& vec = it->second;
        for (int i = 0; i < vec.size(); ++i) {
            const auto& sym = vec[i];
            if (!sym->is_declared()) {
                continue;
            }
            if ((0x1 << sym->get_type_enum()) & lookup_flags) {
                return sym;
            }
        }

        //printf("! Lookup failed !\n");

        return nullptr;
    }

    // Get symbol within this scope or an inline namespace, or any enclosing scope
    symbol_ref lookup(const std::string& name, int lookup_flags) const {
        const symbol_table* scope = this;
        while (scope) {
            symbol_ref sym = scope->get_symbol(name, lookup_flags);
            if (sym) {
                return sym;
            }

            // NOTE: Namespaces can't be temporary symbols, so don't check those
             
            for (auto& kv : scope->symbols) {
                const auto& vec = kv.second;
                if ((vec.mask & LOOKUP_FLAG_NAMESPACE) == 0) {
                    continue;
                }

                for (int i = 0; i < vec.size(); ++i) {
                    symbol_ref sym = vec[i];
                    if (0x1 << sym->get_type_enum() != LOOKUP_FLAG_NAMESPACE) {
                        continue;
                    }

                    symbol_namespace* sym_ns = dynamic_cast<symbol_namespace*>(sym.get());
                    if (!sym_ns) {
                        assert(false);
                        continue;
                    }

                    if (!sym_ns->is_inline && !sym_ns->is_anon) {
                        continue;
                    }

                    // TODO: drill down through the inline namespaces
                    // TODO: right now we're looking only at the first one
                    sym = sym_ns->nested_symbol_table->get_symbol(name, lookup_flags);
                    if (sym) {
                        return sym;
                    }
                }
            }

            scope = scope->parent.get();
        }
        return 0;
    }
    bool is_parsing_class_first_pass() const {
        const symbol_table* scope = this;
        while (scope) {
            if (scope->get_type() == scope_class_pass_one) {
                return true;
            }
            scope = scope->parent.get();
        }
        return false;
    }

    void set_enclosing(const symbol_table_ref& scope) { parent = scope; }
    const symbol_table_ref& get_enclosing() const { return parent; }
    void set_owner(const symbol_ref& owner) { this->owner = owner; }
    const symbol_ref& get_owner() const { return owner; }
    template<typename T>
    bool is_owner_type() const {
        if (!owner) {
            return false;
        }
        return owner->is<T>();
    }

    const symbol_table_ref& get_parent_table() const { return this->parent; }
};

