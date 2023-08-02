#pragma once

#include "parse_state.hpp"
#include "type_id.hpp"

enum TPL_PARAM {
    TPL_PARAM_TYPE,
    TPL_PARAM_PACK,
    TPL_PARAM_NON_TYPE,
    TPL_PARAM_TPL
};

struct tpl_parameter {
    std::string name;
    TPL_PARAM param_type;
    type_id default_type_arg;
};

struct tpl_parameter_list {
    std::vector<tpl_parameter> params;
    bool has_pack = false;
    bool has_default_args_ = false;

    void add(const tpl_parameter& param) {
        params.push_back(param);
        if (param.param_type == TPL_PARAM_PACK) {
            has_pack = true;
        }
        if (!param.default_type_arg.empty()) {
            has_default_args_ = true;
        }
    }
    bool has_pack_param() const {
        return has_pack;
    }
    bool has_default_args() const {
        return has_default_args_;
    }
};

bool eat_template_declaration(parse_state& ps);