#pragma once

#include <assert.h>
#include "expression/eval_value.hpp"
#include "type_id/type_id.hpp"
#include "template_argument.hpp"

enum e_template_parameter_kind {
    e_template_parameter_type,
    e_template_parameter_non_type,
    e_template_parameter_template
};

class template_parameter {
    e_template_parameter_kind kind;
    const type_id_2* tid = nullptr;
    eval_value non_type_default_arg;
    bool is_pack_param = false;

    template_parameter(e_template_parameter_kind kind, const type_id_2* default_arg, bool is_pack)
        : kind(kind), tid(default_arg), is_pack_param(is_pack) {}
    template_parameter(e_template_parameter_kind kind, const type_id_2* tid, const eval_value& default_arg, bool is_pack)
        : kind(kind), tid(tid), non_type_default_arg(default_arg), is_pack_param(is_pack) {}

public:
    static template_parameter make_type(const type_id_2* default_arg, bool is_pack) {
        return template_parameter(e_template_parameter_type, default_arg, is_pack);
    }
    static template_parameter make_non_type(const type_id_2* tid, const eval_value& default_arg, bool is_pack) {
        return template_parameter(e_template_parameter_non_type, tid, default_arg, is_pack);
    }

    e_template_parameter_kind get_kind() const {
        return kind;
    }

    bool is_pack() const {
        return is_pack_param;
    }

    bool has_default_arg() const {
        if (kind == e_template_parameter_type) {
            return tid != nullptr;
        } else if (kind == e_template_parameter_non_type) {
            return !non_type_default_arg.is_empty();
        }
        return false;
    }

    void set_default_arg(const template_parameter& src) {
        assert(kind == src.kind);

        if (kind == e_template_parameter_type) {
            tid = src.tid;
        } else if (kind == e_template_parameter_non_type) {
            non_type_default_arg = src.non_type_default_arg;
        } else if (kind == e_template_parameter_template) {
            assert(false);
            // TODO:
        } else {
            assert(false);
        }
    }
    template_argument get_default_arg() const {
        assert(has_default_arg());
        if (kind == e_template_parameter_type) {
            return template_argument::make_type(tid);
        } else if (kind == e_template_parameter_non_type) {
            return template_argument::make_non_type(non_type_default_arg);
        } else if (kind == e_template_parameter_template) {
            assert(false);
            return template_argument::make_empty();
        }
        assert(false);
        return template_argument::make_empty();
    }
};