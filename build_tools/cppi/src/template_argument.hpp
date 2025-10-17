#pragma once

#include "expression/eval_value.hpp"
#include "type_id/type_id.hpp"


enum e_template_argument_kind {
    e_template_argument_none,
    e_template_argument_type,
    e_template_argument_non_type,
    e_template_argument_template
};

class template_argument {
    e_template_argument_kind kind;
    const type_id_2* tid = nullptr;
    eval_value value;
    std::vector<template_argument> pack;
    bool m_is_pack = false;

    template_argument(e_template_argument_kind kind)
        : kind(kind) {}
    template_argument(e_template_argument_kind kind, const type_id_2* tid)
        : kind(kind), tid(tid) {}
    template_argument(e_template_argument_kind kind, const eval_value& val)
        : kind(kind), value(val) {}
    template_argument(e_template_argument_kind kind, const std::vector<template_argument>& pack)
        : kind(kind), pack(pack), m_is_pack(true) {}

public:
    static template_argument make_empty() {
        return template_argument(e_template_argument_none);
    }
    static template_argument make_type(const type_id_2* tid) {
        return template_argument(e_template_argument_type, tid);
    }
    static template_argument make_non_type(const eval_value& val) {
        return template_argument(e_template_argument_non_type, val);
    }
    static template_argument make_pack(e_template_argument_kind kind, const std::vector<template_argument>& pack) {
        return template_argument(kind, pack);
    }

    template_argument()
        : kind(e_template_argument_none) {}

    e_template_argument_kind get_kind() const {
        return kind;
    }

    bool is_empty() const {
        return kind == e_template_argument_none;
    }

    bool is_pack() const {
        return m_is_pack;
    }
    bool is_empty_pack() const {
        return m_is_pack && pack.empty();
    }

    const type_id_2* get_contained_type() const {
        if(kind != e_template_argument_type) {
            assert(false);
            return nullptr;
        }
        return tid;
    }
    eval_value get_contained_value() const {
        if(kind != e_template_argument_non_type) {
            assert(false);
            return eval_value();
        }
        return value;
    }
    const std::vector<template_argument>& get_contained_pack() const {
        if (!is_pack()) {
            assert(false);
        }
        return pack;
    }

    std::string to_source_name() const {
        if (is_pack()) {
            std::string str;
            for (int i = 0; i < pack.size(); ++i) {
                if(i) str += ", ";
                str += pack[i].to_source_name();
            }
            return str;
        }
        if (kind == e_template_argument_type) {
            return tid->to_source_string();
        } else if (kind == e_template_argument_non_type) {
            return value.normalized();
        } else if (kind == e_template_argument_template) {
            assert(false);
            return "?NOT_IMPLEMENTED?";
        } else {
            assert(false);
            return "_?";
        }
    }

    std::string to_internal_name() const {
        if (is_pack()) {
            std::string str = "J";
            for (int i = 0; i < pack.size(); ++i) {
                str += pack[i].to_internal_name();
            }
            str += "E";
            return str;
        }
        if (kind == e_template_argument_type) {
            return tid->get_internal_name();
        } else if (kind == e_template_argument_non_type) {
            // TODO: Handle non-fundamental types too
            std::string name;
            name += "L";
            name += fundamental_type_to_normalized_string(value.type);
            name += value.normalized();
            name += "E";
            return name;
        } else if (kind == e_template_argument_template) {
            assert(false);
            return "_?";
        } else {
            assert(false);
            return "_?";
        }
        /*
        std::string tpl_name = tpl_sym->name + "I";
        for (int i = 0; i < arg_list->list.size(); ++i) {
        const auto& arg = arg_list->list[i];

        if(const expression* expr = arg.as<expression>()) {
        // TODO: IMPORTANT! use parameter type, not argument type
        // TODO: check if result is valid
        // TODO: check if result converts to template parameter type
        // TODO: enum types should be handled here too
        eval_result res = expr->evaluate();
        tpl_name += "L";
        tpl_name += fundamental_type_to_normalized_string(res.value.type);
        tpl_name += res.value.normalized();
        tpl_name += "E";
        } else if(const ast::type_id* tid = arg.as<ast::type_id>()) {
        // TODO:
        type_id_2* type_id = resolve_type_id(ps, tid);
        dbg_printf_color("%s -> %s\n", DBG_RED, type_id->to_source_string().c_str(), type_id->get_internal_name().c_str());
        const std::string& name = type_id->get_internal_name();
        if (name.empty()) {
        assert(false);
        }
        tpl_name += name;
        } else {
        throw parse_exception("unexpected syntax in template argument", ps.get_latest_token());
        //assert(false);
        //tpl_name += "?";
        }
        }
        tpl_name += "E";
        */
    }
};