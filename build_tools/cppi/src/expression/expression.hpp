#pragma once

#include "eval_result.hpp"



struct expression {
    virtual ~expression() {}

    virtual eval_result evaluate() const {
        return eval_result::not_implemented();
    }
};

