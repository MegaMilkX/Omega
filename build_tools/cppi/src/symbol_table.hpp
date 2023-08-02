#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include <string>

#include "pp_state.hpp"

#include "parse_exception.hpp"
#include "parsing/type_id.hpp"


enum e_enum_key {
    ck_enum,
    ck_enum_class,
    ck_enum_struct
};
inline const char* enum_key_to_string(e_enum_key k) {
    switch (k) {
    case ck_enum: return "enum";
    case ck_enum_class: return "enum class";
    case ck_enum_struct: return "enum struct";
    default: return "";
    }
}


constexpr int LOOKUP_FLAG_OBJECT = 0x0001;
constexpr int LOOKUP_FLAG_FUNCTION = 0x0002;
constexpr int LOOKUP_FLAG_CLASS = 0x0004;
constexpr int LOOKUP_FLAG_NAMESPACE = 0x0008;
constexpr int LOOKUP_FLAG_ENUM = 0x0010;
constexpr int LOOKUP_FLAG_TYPEDEF = 0x0020;
constexpr int LOOKUP_FLAG_TEMPLATE = 0x0040;
constexpr int LOOKUP_FLAG_ENUMERATOR = 0x0080;

enum scope_type {
    scope_default,
    scope_class_pass_one,
    scope_class,
    scope_enum
};

class symbol_table;
class symbol {
public:
    virtual ~symbol() {}

    PP_FILE* file = 0;
    std::string name;
    std::string global_qualified_name;
    std::shared_ptr<symbol_table> nested_symbol_table;
    bool defined = false;

    template<typename T>
    bool is() const { return typeid(T) == get_type(); }
    virtual const type_info& get_type() const = 0;

    bool is_defined() const { return defined; }
    void set_defined(bool d) { defined = d; }

    virtual void dbg_print() const { printf("<not implemented>"); }
};
class symbol_object : public symbol {
    const type_info& get_type() const override { return typeid(*this); }
    void dbg_print() const override { 
        dbg_printf_color("data %s", DBG_RED | DBG_GREEN | DBG_BLUE, global_qualified_name.c_str());
        printf(" ");
        type_id_.dbg_print();
    }
public:
    type_id type_id_;
};
class symbol_func_overload : public symbol {
    const type_info& get_type() const override { return typeid(*this); }
    void dbg_print() const override {
        printf("%s", global_qualified_name.c_str());
        printf(" ");
        type_id_.dbg_print();
    }
public:
    type_id type_id_;
};
class symbol_function : public symbol {
    const type_info& get_type() const override { return typeid(*this); }
    void dbg_print() const override {
        dbg_printf_color("function %s", DBG_RED | DBG_GREEN | DBG_BLUE, global_qualified_name.c_str());
        for (auto& o : overloads) {
            printf("\n");
            ((symbol*)o.get())->dbg_print();
        }
    }
public:
    std::vector<std::shared_ptr<symbol_func_overload>> overloads;
    //type_id type_id_;

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
    }
};
class symbol_class : public symbol {
    const type_info& get_type() const override { return typeid(*this); }
    void dbg_print() const override { 
        dbg_printf_color("class %s", DBG_RED | DBG_GREEN | DBG_BLUE, global_qualified_name.c_str());
    }
public:
};
class symbol_enum : public symbol {
    const type_info& get_type() const override { return typeid(*this); }
    void dbg_print() const override { 
        dbg_printf_color("enum %s", DBG_RED | DBG_GREEN | DBG_BLUE, global_qualified_name.c_str());
    }
public:
    e_enum_key key;
};
class symbol_enumerator : public symbol {
    const type_info& get_type() const override { return typeid(*this); }
    void dbg_print() const override {
        dbg_printf_color("enumerator %s", DBG_GREEN | DBG_BLUE, global_qualified_name.c_str());
    }
};
class symbol_typedef : public symbol {
    const type_info& get_type() const override { return typeid(*this); }
    void dbg_print() const override {
        dbg_printf_color("alias %s ", DBG_RED | DBG_GREEN | DBG_BLUE, global_qualified_name.c_str());
        //printf(" is an alias for ");
        type_id_.dbg_print();
    }
public:
    type_id type_id_;
};
class symbol_template : public symbol {
    const type_info& get_type() const override { return typeid(*this); }
    void dbg_print() const override { 
        dbg_printf_color("template %s", DBG_RED | DBG_GREEN | DBG_BLUE, global_qualified_name.c_str());
    }
public:
};
// This one does not appear in the symbol table ever
class symbol_placeholder : public symbol {
    const type_info& get_type() const override { return typeid(*this); }
    void dbg_print() const override {
        dbg_printf_color("placeholder %s", DBG_RED | DBG_GREEN, global_qualified_name.c_str());
    }
public:
    std::vector<token> token_cache;
};

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

class symbol_table {
    scope_type type = scope_default;
    std::shared_ptr<symbol_table> parent;
    std::shared_ptr<symbol> owner;
public:
    std::unordered_map<std::string, std::shared_ptr<symbol>> types;
    std::unordered_map<std::string, std::shared_ptr<symbol>> enums;
    std::unordered_map<std::string, std::shared_ptr<symbol>> enumerators;
    std::unordered_map<std::string, std::shared_ptr<symbol>> objects;
    std::unordered_map<std::string, std::shared_ptr<symbol>> functions;
    std::unordered_map<std::string, std::shared_ptr<symbol>> typedefs;
    std::unordered_map<std::string, std::shared_ptr<symbol>> templates;

    void set_type(scope_type type) { this->type = type; }
    scope_type get_type() const { return type; }

    void add_symbol(const std::shared_ptr<symbol>& sym) {
        if (sym->name.empty()) {
            sym->name = std::string("$unnamed_") + gen_random_string(16);
        }
        if (sym->is<symbol_class>()) {
            types[sym->name] = sym;
        } else if(sym->is<symbol_enum>()) {
            enums[sym->name] = sym;
        } else if(sym->is<symbol_enumerator>()) {
            enumerators[sym->name] = sym;
        } else if(sym->is<symbol_object>()) {
            objects[sym->name] = sym;
        } else if(sym->is<symbol_function>()) {
            functions[sym->name] = sym;
        } else if(sym->is<symbol_typedef>()) {
            typedefs[sym->name] = sym;
        } else if(sym->is<symbol_template>()) {
            templates[sym->name] = sym;
        } else {
            assert(false);
        }

        {
            std::string global_qualified_name;
            auto scope_owner_sym = owner.get();
            if (scope_owner_sym && !scope_owner_sym->global_qualified_name.empty() && parent) {
                global_qualified_name = scope_owner_sym->global_qualified_name + std::string("::") + sym->name;
            } else if(parent == nullptr) {
                global_qualified_name = sym->name;
            } else {
                global_qualified_name == "";
            }
            sym->global_qualified_name = global_qualified_name;
        }
    }
    // Get symbol within this scope
    std::shared_ptr<symbol> get_symbol(const std::string& local_name, int lookup_flags) const {
        if (lookup_flags & LOOKUP_FLAG_OBJECT) {
            auto it = objects.find(local_name);
            if (it != objects.end()) {
                return it->second;
            }
        }
        if (lookup_flags & LOOKUP_FLAG_FUNCTION) {
            auto it = functions.find(local_name);
            if (it != functions.end()) {
                return it->second;
            }
        }
        if (lookup_flags & LOOKUP_FLAG_TYPEDEF) {
            auto it = typedefs.find(local_name);
            if (it != typedefs.end()) {
                return it->second;
            }
        }
        if (lookup_flags & LOOKUP_FLAG_CLASS) {
            auto it = types.find(local_name);
            if (it != types.end()) {
                return it->second;
            }
        }
        if (lookup_flags & LOOKUP_FLAG_TEMPLATE) {
            auto it = templates.find(local_name);
            if (it != templates.end()) {
                return it->second;
            }
        }
        if (lookup_flags & LOOKUP_FLAG_ENUM) {
            auto it = enums.find(local_name);
            if (it != enums.end()) {
                return it->second;
            }
        }
        if (lookup_flags & LOOKUP_FLAG_ENUMERATOR) {
            auto it = enumerators.find(local_name);
            if (it != enumerators.end()) {
                return it->second;
            }
        }
        if (lookup_flags & LOOKUP_FLAG_NAMESPACE) {
            // TODO:
        }

        return 0;
    }

    // Get symbol within this scope or any enclosing scope
    std::shared_ptr<symbol> lookup(const std::string& name, int lookup_flags) const {
        const symbol_table* scope = this;
        while (scope) {
            auto sym = scope->get_symbol(name, lookup_flags);
            if (sym) {
                return sym;
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

    void set_enclosing(const std::shared_ptr<symbol_table>& scope) { parent = scope; }
    const std::shared_ptr<symbol_table>& get_enclosing() const { return parent; }
    void set_owner(const std::shared_ptr<symbol>& owner) { this->owner = owner; }
    const std::shared_ptr<symbol>& get_owner() const { return owner; }
    template<typename T>
    bool is_owner_type() const {
        if (!owner) {
            return false;
        }
        return owner->is<T>();
    }
};
