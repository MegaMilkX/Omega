#pragma once

#include <assert.h>
#include <string>
#include <unordered_map>
#include <memory>
#include "dbg.hpp"
#include "types.hpp"
#include "decl_spec_compat.hpp"


class symbol;
using symbol_ref = std::shared_ptr<symbol>;

class type_id_storage;
class type_id_graph_node;

class type_id_2 {
    friend type_id_storage;
    friend type_id_graph_node;

    const type_id_2* next = nullptr;
    mutable type_id_graph_node* lookup_node = nullptr;

    std::string name;
    mutable std::string source_name;
protected:
    const type_id_2* get_next() const {
        return next;
    }

public:
    type_id_2()
        : name("") {}
    type_id_2(const type_id_2* next, const std::string& name)
        : next(next), name(name) {}
    virtual ~type_id_2() {}

    const std::string& get_internal_name() const {
        return name;
    }

    const std::string& to_source_string() const {
        if (source_name.empty()) {
            build_source_name(source_name);
        }
        return source_name;
    }

    bool is_function() const;
    bool is_lvref() const;
    bool is_rvref() const;

    template<typename T>
    const T* as() const { return dynamic_cast<const T*>(this); }

    const type_id_2* get_return_type() const {
        if (!is_function()) {
            return nullptr;
        }
        return next;
    }

    virtual const type_id_2* strip_cv() const { return this; }

    type_id_graph_node* get_lookup_node() const { return lookup_node; }

    virtual void build_source_name(std::string& inout) const = 0;
};


class type_id_2_member_pointer : public type_id_2 {
    void build_source_name(std::string& inout) const override;
public:
    type_id_2_member_pointer(const type_id_2* next, const std::string& name, symbol_ref owner, bool is_const, bool is_volatile)
        : owner(owner)
        , is_const(is_const)
        , is_volatile(is_volatile)
        , type_id_2(next, name) {}

    const type_id_2* strip_cv() const override;

    symbol_ref owner;
    bool is_const;
    bool is_volatile;
};

class type_id_2_pointer : public type_id_2 {
    void build_source_name(std::string& inout) const override {
        std::string c_part = is_const ? std::string(" ") + cv_to_string(cv_const) : "";
        std::string v_part = is_volatile ? std::string(" ") + cv_to_string(cv_volatile) : "";

        if (get_next()->is_function()) {
            inout = "(*" + c_part + v_part + inout + ")";
        } else {
            inout = "*" + c_part + v_part + inout;
        }
        get_next()->build_source_name(inout);
    }
public:
    type_id_2_pointer(const type_id_2* next, const std::string& name, bool is_const, bool is_volatile)
        : is_const(is_const)
        , is_volatile(is_volatile)
        , type_id_2(next, name) {}

    const type_id_2* strip_cv() const override;

    bool is_const;
    bool is_volatile;
};

class type_id_2_ref : public type_id_2 {
    void build_source_name(std::string& inout) const override {
        if (get_next()->is_function()) {
            inout = "(&" + inout + ")";
        } else {
            inout = "&" + inout;
        }
        if (get_next()->is_lvref() || get_next()->is_rvref()) {
            inout = " " + inout;
        }
        get_next()->build_source_name(inout);
    }
public:
    type_id_2_ref(const type_id_2* next, const std::string& name)
        : type_id_2(next, name) {}

    const type_id_2* strip_cv() const override {
        // TODO: !!!
        return this;
    }
};

class type_id_2_rvref : public type_id_2 {
    void build_source_name(std::string& inout) const override {
        if (get_next()->is_function()) {
            inout = "(&&" + inout + ")";
        } else {
            inout = "&&" + inout;
        }
        if (get_next()->is_lvref() || get_next()->is_rvref()) {
            inout = " " + inout;
        }
        get_next()->build_source_name(inout);
    }
public:
    type_id_2_rvref(const type_id_2* next, const std::string& name)
        : type_id_2(next, name) {}

    const type_id_2* strip_cv() const override {
        // TODO: !!!
        return this;
    }
};

class type_id_2_array : public type_id_2 {
    size_t size = 0;
    
    void build_source_name(std::string& inout) const override {
        inout = inout + "[" + std::to_string(size) + "]";
        get_next()->build_source_name(inout);
    }
public:
    type_id_2_array(const type_id_2* next, const std::string& name, size_t size)
        : type_id_2(next, name), size(size) {}

};

class type_id_2_function : public type_id_2 {
    std::vector<const type_id_2*> params;
    bool is_const;
    bool is_volatile;

    void build_source_name(std::string& inout) const override {
        inout += "(";
        for (int i = 0; i < params.size(); ++i) {
            if(i) inout += ", ";
            inout += params[i]->to_source_string();
        }
        inout += ")";
        if (is_const) {
            inout += std::string(" ") + cv_to_string(cv_const);
        }
        if (is_volatile) {
            inout += std::string(" ") + cv_to_string(cv_volatile);
        }
        get_next()->build_source_name(inout);
    }
public:
    type_id_2_function(const type_id_2* next, const std::string& name, const type_id_2** params, int param_count, bool is_const, bool is_volatile)
        : type_id_2(next, name), params(params, params + param_count), is_const(is_const), is_volatile(is_volatile) {}

};

class type_id_2_fundamental : public type_id_2 {
    void build_source_name(std::string& inout) const override {
        std::vector<std::string> parts;
        parts.reserve(3);

        if(is_const) parts.push_back(cv_to_string(cv_const));
        if(is_volatile) parts.push_back(cv_to_string(cv_volatile));
        parts.push_back(fundamental_type_to_string(type));

        std::string str;
        for (int i = 0; i < parts.size(); ++i) {
            if(i) str += " ";
            str += parts[i];
        }

        inout = str + inout;
    }
public:
    type_id_2_fundamental(const std::string& name, e_fundamental_type t, bool is_const, bool is_volatile)
        : type(t)
        , is_const(is_const)
        , is_volatile(is_volatile)
        , type_id_2(nullptr, name)
    {}

    const type_id_2* strip_cv() const override;

    e_fundamental_type type;
    bool is_const;
    bool is_volatile;
};

class type_id_2_user_type : public type_id_2 {
    void build_source_name(std::string& inout) const override;
public:
    type_id_2_user_type(const std::string& name, symbol_ref sym, bool is_const, bool is_volatile)
        : sym(sym)
        , is_const(is_const)
        , is_volatile(is_volatile)
        , type_id_2(nullptr, name)
    {}

    const type_id_2* strip_cv() const override;

    symbol_ref sym;
    bool is_const;
    bool is_volatile;
};


inline bool type_id_2::is_function() const {
    return dynamic_cast<const type_id_2_function*>(this) != 0;
}
inline bool type_id_2::is_lvref() const {
    if (dynamic_cast<const type_id_2_ref*>(this)) {
        return true;
    }
    return false;
}
inline bool type_id_2::is_rvref() const {
    if (dynamic_cast<const type_id_2_rvref*>(this)) {
        return true;
    }
    return false;
}


struct type_id_graph_node {
    std::unique_ptr<type_id_2> type_id = nullptr;

    std::unordered_map<std::string, std::unique_ptr<type_id_graph_node>> nodes;

    type_id_graph_node* find_node(const std::string& sub_name) {
        auto it = nodes.find(sub_name);
        if (it == nodes.end()) {
            return nullptr;
        }
        return it->second.get();
    }

    type_id_graph_node* walk(const std::string& sub_name) {
        auto it = nodes.find(sub_name);
        if (it != nodes.end()) {
            return it->second.get();
        }
        type_id_graph_node* n = new type_id_graph_node;
        nodes.insert(std::make_pair(sub_name, std::unique_ptr<type_id_graph_node>(n)));
        return n;
    }

    void dbg_print_all() const {
        assert(type_id);
        dbg_printf_color("%s\n", DBG_WHITE, type_id->to_source_string().c_str());
        for (auto& kv : nodes) {
            kv.second->dbg_print_all();
        }
    }
};


class type_id_storage {
    std::unordered_map<std::string, std::unique_ptr<type_id_graph_node>> base_nodes;
    //std::unique_ptr<type_id_graph_node> root;

    type_id_storage()
        //: root(new type_id_graph_node) 
    {}

public:
    static type_id_storage* get() {
        static type_id_storage* this_ = new type_id_storage;
        return this_;
    }

    void dbg_print_all() const {
        for (auto& kv : base_nodes) {
            kv.second->dbg_print_all();
        }
    }

    type_id_graph_node* get_base_node(e_fundamental_type t, bool is_const = false, bool is_volatile = false) {
        std::string name = fundamental_type_to_normalized_string(t);
        if (is_const) {
            name = "K" + name;
        }
        if (is_volatile) {
            name = "V" + name;
        }
        type_id_graph_node* n = find_base_node(name.c_str());
        if (n) {
            return n;
        }
        n = new type_id_graph_node;
        base_nodes.insert(std::make_pair(std::string(name), std::unique_ptr<type_id_graph_node>(n)));
        n->type_id.reset(new type_id_2_fundamental(name, t, is_const, is_volatile));
        n->type_id->lookup_node = n;
        return n;
    }
    
    type_id_graph_node* get_base_node(symbol_ref sym, bool is_const = false, bool is_volatile = false);

    type_id_graph_node* get_base_node(const decl_flags& flags) {
        e_fundamental_type t = decl_flags_to_fundamental_type(flags);
        if (t == e_fundamental_invalid) {
            return nullptr;
        }
        return get_base_node(t, flags.has(e_cv_const), flags.has(e_cv_volatile));
    }
    type_id_graph_node* find_base_node(const char* name) {
        auto it = base_nodes.find(name);
        if (it == base_nodes.end()) {
            return nullptr;
        }
        return it->second.get();
    }

    type_id_graph_node* walk_to_member_ptr(type_id_graph_node* current, symbol_ref owner, bool is_const, bool is_volatile);

    type_id_graph_node* walk_to_pointer(type_id_graph_node* current, bool is_const, bool is_volatile) {
        std::string sub_name = "P";
        if (is_const) {
            sub_name = "K" + sub_name;
        }
        if (is_volatile) {
            sub_name = "V" + sub_name;
        }

        type_id_graph_node* n = current->walk(sub_name);
        if (n->type_id) {
            return n;
        }

        type_id_2_pointer* tid = new type_id_2_pointer(current->type_id.get(), sub_name + current->type_id->get_internal_name(), is_const, is_volatile);
        n->type_id.reset(tid);
        n->type_id->lookup_node = n;
        return n;
    }
    type_id_graph_node* walk_to_ref(type_id_graph_node* current) {
        std::string sub_name = "R";

        type_id_graph_node* n = current->walk(sub_name);
        if (n->type_id) {
            return n;
        }

        type_id_2_ref* tid = new type_id_2_ref(current->type_id.get(), sub_name + current->type_id->get_internal_name());
        n->type_id.reset(tid);
        n->type_id->lookup_node = n;
        return n;
    }
    type_id_graph_node* walk_to_rvref(type_id_graph_node* current) {
        std::string sub_name = "O";

        type_id_graph_node* n = current->walk(sub_name);
        if (n->type_id) {
            return n;
        }

        type_id_2_rvref* tid = new type_id_2_rvref(current->type_id.get(), sub_name + current->type_id->get_internal_name());
        n->type_id.reset(tid);
        n->type_id->lookup_node = n;
        return n;
    }
    type_id_graph_node* walk_to_array(type_id_graph_node* current, size_t array_size) {
        std::string sub_name = "A" + std::to_string(array_size) + "_";

        type_id_graph_node* n = current->walk(sub_name);        
        if (n->type_id) {
            return n;
        }

        type_id_2_array* tid = new type_id_2_array(current->type_id.get(), sub_name + current->type_id->get_internal_name(), array_size);
        n->type_id.reset(tid);
        n->type_id->lookup_node = n;
        return n;
    }
    type_id_graph_node* walk_to_function(type_id_graph_node* current, const type_id_2** params, int param_count, bool is_const, bool is_volatile) {
        std::string param_signature;
        if(param_count) {
            for (int i = 0; i < param_count; ++i) {
                const type_id_2* param = params[i];
                param_signature += param->get_internal_name();
            }
        } else {
            param_signature = fundamental_type_to_normalized_string(e_void);
        }
        
        std::string fn_name = "F" + current->type_id->get_internal_name() + param_signature + "E";
        if (is_const) {
            fn_name = "K" + fn_name;
        }
        if (is_volatile) {
            fn_name = "V" + fn_name;
        }

        // Would be more efficient to exclude return type from key,
        // but it's simpler this way
        type_id_graph_node* n = current->walk(fn_name);        
        if (n->type_id) {
            return n;
        }


        type_id_2_function* tid = new type_id_2_function(current->type_id.get(), fn_name, params, param_count, is_const, is_volatile);
        n->type_id.reset(tid);
        n->type_id->lookup_node = n;
        return n;
    }
};


class TYPE_ID {
    const type_id_2* tid = nullptr;

    TYPE_ID(const type_id_2* tid)
        : tid(tid) {}
public:
    TYPE_ID() {}
    TYPE_ID(e_fundamental_type type, bool is_const, bool is_volatile)
        : tid(type_id_storage::get()->get_base_node(type, is_const, is_volatile)->type_id.get())
    {}
    TYPE_ID(symbol_ref sym, bool is_const, bool is_volatile)
        : tid(type_id_storage::get()->get_base_node(sym, is_const, is_volatile)->type_id.get())
    {}
    TYPE_ID(const decl_flags& flags)
        : tid(type_id_storage::get()->get_base_node(flags)->type_id.get())
    {}
    TYPE_ID(decltype(nullptr))
        : tid(nullptr) {}

    static TYPE_ID null() { return TYPE_ID(nullptr); }
    static TYPE_ID make_fundamental_type(e_fundamental_type type, bool is_const, bool is_volatile) {
        return TYPE_ID(type, is_const, is_volatile);
    }
    static TYPE_ID make_user_defined_type(symbol_ref sym, bool is_const, bool is_volatile) {
        return TYPE_ID(sym, is_const, is_volatile);
    }
    static TYPE_ID make_from_decl_flags(const decl_flags& flags) {
        return TYPE_ID(flags);
    }

    bool operator==(const TYPE_ID& other) const {
        return tid == other.tid;
    }
    bool operator!=(const TYPE_ID& other) const {
        return tid != other.tid;
    }

    bool is_null() const { return tid == nullptr; }

    void build_source_name(std::string& inout) const {
        if(!tid) return;
        return tid->build_source_name(inout);
    }

    std::string to_source_string() const {
        if(!tid) return "[NULL_TYPE_ID]";
        return tid->to_source_string();
    }

    const std::string& get_internal_name() const {
        if(!tid) return "[NULL_TYPE_ID]";
        return tid->get_internal_name();
    }

    TYPE_ID get_return_type() const {
        return TYPE_ID(tid->get_return_type());
    }

    symbol_ref get_type_symbol() const {
        auto udt = tid->as<type_id_2_user_type>();
        if (!udt) {
            return nullptr;
        }
        return udt->sym;
    }

    TYPE_ID strip_cv() const {
        if(!tid) return TYPE_ID(nullptr);
        return TYPE_ID(tid->strip_cv());
    }

    TYPE_ID make_member_ptr(symbol_ref owner, bool is_const, bool is_volatile) const {
        if(!tid) return TYPE_ID(nullptr);
        auto n = type_id_storage::get()->walk_to_member_ptr(tid->get_lookup_node(), owner, is_const, is_volatile);
        return TYPE_ID(n->type_id.get());
    }
    TYPE_ID make_ptr(bool is_const, bool is_volatile) const {
        if(!tid) return TYPE_ID(nullptr);
        auto n = type_id_storage::get()->walk_to_pointer(tid->get_lookup_node(), is_const, is_volatile);
        return TYPE_ID(n->type_id.get());
    }
    TYPE_ID make_ref() const {
        if(!tid) return TYPE_ID(nullptr);
        auto n = type_id_storage::get()->walk_to_ref(tid->get_lookup_node());
        return TYPE_ID(n->type_id.get());
    }
    TYPE_ID make_rvref() const {
        if(!tid) return TYPE_ID(nullptr);
        auto n = type_id_storage::get()->walk_to_rvref(tid->get_lookup_node());
        return TYPE_ID(n->type_id.get());
    }
    TYPE_ID make_array(size_t size) const {
        if(!tid) return TYPE_ID(nullptr);
        auto n = type_id_storage::get()->walk_to_array(tid->get_lookup_node(), size);
        return TYPE_ID(n->type_id.get());
    }
    TYPE_ID make_function(TYPE_ID* params, int param_count, bool is_const, bool is_volatile) const {
        if(!tid) return TYPE_ID(nullptr);
        
        std::vector<const type_id_2*> params_(param_count);
        for (int i = 0; i < param_count; ++i) {
            params_[i] = params[i].tid;
        }

        auto n = type_id_storage::get()->walk_to_function(
            tid->get_lookup_node(),
            params_.data(),
            param_count,
            is_const,
            is_volatile
        );
        return TYPE_ID(n->type_id.get());
    }
};

